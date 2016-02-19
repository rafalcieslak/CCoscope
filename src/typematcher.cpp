#include "typematcher.h"
#include "codegencontext.h"
#include <algorithm>

namespace ccoscope{

void TypeMatcher::InitImplicitConversions(){
    implicit_conversions[ctx.getIntegerTy()] = { // Conversions from int
        Conversion{
            ctx.getIntegerTy(),
            ctx.getDoubleTy(),                   // --- to double
            1,                                   // ----- costs 1
            [](CodegenContext & ctx, llvm::Value* v)->llvm::Value*{  // Note: The CodegenContext is passed again. We
                                                                     // canot reuse the parent context, because we
                                                                     // need a non-const context.
                return ctx.Builder.CreateSIToFP(v, llvm::Type::getDoubleTy(llvm::getGlobalContext()), "convtmp");
            }
        }
    };

    //TODO: Is there any other conversion we should use? I really
    // dislike implicit conversions from/to boolean types, and
    // implicit conversion double->int seems ugly. So until we have
    // more types, this is probably the only implicit conversion we
    // use...
}

// Helper function for calcluating a sum of conversion costs
static ConversionCost Summarize(const std::vector<Conversion>& v){
    ConversionCost sum = 0;
    for(const Conversion& c : v) sum += c.cost;
    return sum;
}

const TypeMatcher::Result TypeMatcher::MatchOperator(std::string name, Type t1, Type t2) const{
    // Currently the matcher does a first fit search, and it ignores possible implicit conversions.

    // Get the list of operator variants under this name
    auto opit = ctx.BinOpCreator.find(name);
    if(opit == ctx.BinOpCreator.end()) {
        ctx.AddError("Operator's '" + name + "' codegen is not implemented!");
        return Result();
    }
    const std::list<OperatorEntry>& operator_variants = opit->second;

    // Prepare type signature
    std::vector<Type> signature = {t1, t2};
    // Generate possible conversions for each input type
    std::vector<std::list<Conversion>> inflated = InflateTypes(signature);
    // Generate all combinations of conversions
    std::list<std::vector<Conversion>> combinations = CombinationWalker(inflated);
    // Filter out those that match some operator
    combinations.remove_if(
        [&](const std::vector<Conversion> v){
            Type target_1 = v[0].target_type;
            Type target_2 = v[1].target_type;
            // Search for a matching OperatorEntry. This is not O(1)! It could
            // be if operator_variants was a map instead of a list... but then
            // the BinOpCreator would be a
            //   map(string -> map( (type,type) -> (type,function) )
            // and that seems just mad. We might use that approach once there is A LOT of possible conversions.
            for(const OperatorEntry& oe : operator_variants){
                if(oe.t1 == target_1 && oe.t2 == target_2){
                    // Match! Do not remove this conversion combination
                    return false;
                }
            }
            // No match found. Remove this combination.
            return true;
        }
    );
    // If no combinations remain, fail.
    if(combinations.size() == 0){
        ctx.AddError("No matching operator '" + name + "' found to call with types: " + t1.deref()->name() + ", " + t2.deref()->name() + ".");
        return Result();
    }
    // Summarize conversion costs for each combination
    std::list<std::pair<ConversionCost, std::vector<Conversion>>> combinations_with_total_costs;
    for(const std::vector<Conversion> v : combinations)
        combinations_with_total_costs.push_back({ Summarize(v), v});
    // Find the minimal cost
    ConversionCost minimum = combinations_with_total_costs.front().first;
    for(const auto& p : combinations_with_total_costs)
        if(p.first < minimum)
            minimum = p.first;
    // Filter out combinations that have non-optimal cost
    combinations_with_total_costs.remove_if([&](auto p){
            return p.first != minimum;
        });
    // At this point, exactly one match should be left.
    if(combinations_with_total_costs.size() == 0){
        ctx.AddError("Internal error: the above operations should not have emptied the combination list.");
        return Result();
    }else if(combinations_with_total_costs.size() > 1){
        //TODO: Print type names
        //TODO: Use combinations_with_total_costs to inform the user about possible candidates
        ctx.AddError("Multiple candidates for operator '" + name + "' and types: " + t1.deref()->name() + ", " + t2.deref()->name() + ".");
        return Result();
    }
    // We have confirmed that there is indeed just one match. Let's unwrap it:
    std::vector<Conversion> best_match = combinations_with_total_costs.front().second;
    // Now find the operator variant for this match.
    for(const OperatorEntry& oe : operator_variants){
        if(oe.t1 != best_match[0].target_type || oe.t2 != best_match[1].target_type) continue;
#if DEBUG
        std::cout << "Operator '" + name  + "' variant used: " << oe.t1.deref()->name() << " " << oe.t2.deref()->name() << std::endl;
        std::cout << "Conversion 1 is from " << best_match[0].orig_type.deref()->name()
                  << " to " << best_match[0].target_type.deref()->name() << std::endl;
        std::cout << "Conversion 2 is from " << best_match[1].orig_type.deref()->name()
                  << " to " << best_match[1].target_type.deref()->name() << std::endl;
#endif

        // At this point, oe is the matching operator variant.
        // Finally, we can prepare the application function.
        ConverterFunction cf1 = best_match[0].converter;
        ConverterFunction cf2 = best_match[1].converter;
        CreatorFunc op_function = oe.creator_function;
        Result::ApplicationFunction af = [cf1, cf2,op_function](CodegenContext& ctx, std::vector<llvm::Value*> v) -> llvm::Value*{
            // Note: This function assumes that v.size() == 2. This is always
            // the case for binary operators, but it could be generalized for
            // different operators and overladed functions so that it would
            // support any-sized vector.
            // Conversions...
            llvm::Value* c1 = cf1(ctx, v[0]);
            llvm::Value* c2 = cf2(ctx, v[1]);
            // Operator...
            return op_function(c1, c2);
        };
        return Result(true, oe.return_type, af);
    }
    // assert(false)
    ctx.AddError("Internal error: Match was found, but it has no corresponding operator variant.");
    return Result();
}

std::list<Conversion> TypeMatcher::Inflate(Type t) const{
    // TODO: Support transitive implicit conversions!  It does not matter now,
    // as we only have a int->double conversion, but at somepoint we may want to
    // use multi-step conversions, like int->float->double. I believe the
    // support can be implemented by modifying only this function - this will
    // require walking a Dijkstra on the conversion graph, summing the costs on
    // our way, and joining the conversion functions. Should be pretty
    // straight-forward, but as we currently have no tools to test this, I'll
    // stay with this simple implementation that does not consider transitive
    // conversions.

    // std::cout << "Inflating a " << t.deref()->name() << std::endl;

    std::list<Conversion> result;
    // Identity conversion
    Conversion id{t,t,0,
            [](CodegenContext&, llvm::Value* v){return v;}
    };
    result.push_back(id);

    // Look up possible conversions from requested type
    auto it = implicit_conversions.find(t);
    if(it != implicit_conversions.end()){
        result.insert(result.end(), it->second.begin(), it->second.end());
    }

    return result;
}

std::vector<std::list<Conversion>> TypeMatcher::InflateTypes(std::vector<Type> v) const{
    std::vector<std::list<Conversion>> result(v.size());
    std::transform(v.begin(), v.end(), result.begin(), [&](Type t){return Inflate(t);});
    return result;
}

}
