#include "typematcher.h"
#include "codegencontext.h"
#include <algorithm>

namespace ccoscope{

// Helper function for calcluating a sum of conversion costs
static ConversionCost Summarize(const std::vector<Conversion>& v){
    ConversionCost sum = 0;
    for(const Conversion& c : v) sum += c.cost;
    return sum;
}

// Returns true IFF both vectors have the same type at their corresonding elements
static bool TestCandidateMatch(const MatchCandidateEntry& m, const std::vector<Conversion>& v) __attribute__((pure));
static bool TestCandidateMatch(const MatchCandidateEntry& m, const std::vector<Conversion>& v){
    const std::vector<Type>& matchtypes = m.input_types;
    if(matchtypes.size() != v.size()) return false;

    // TODO: Is it possible to do this with a ranged based form or an
    // adapter from std algorithms?
    for(unsigned int i = 0; i < matchtypes.size(); i++){
        if(matchtypes[i] != v[i].target_type)
            return false;
    }
    return true;
}

const TypeMatcher::Result TypeMatcher::Match(std::list<MatchCandidateEntry> candidates, std::vector<Type> input_signature) const{
    // Currently the matcher does a first fit search, and it ignores possible implicit conversions.

    // Generate possible conversions for each input type
    std::vector<std::list<Conversion>> inflated = InflateTypes(input_signature);
    // Generate all combinations of conversions
    std::list<std::vector<Conversion>> combinations = CombinationWalker(inflated);
    // Filter out those that match some operator
    combinations.remove_if(
        [&](const std::vector<Conversion> v){
            // Search for a matching OperatorEntry. This is not O(1)! It could
            // be if operator_variants was a map instead of a list... but then
            // the BinOpCreator would be a
            //   map(string -> map( (type,type) -> (type,function) )
            // and that seems just mad. We might use that approach once there is A LOT of possible conversions.
            for(const MatchCandidateEntry& mce : candidates){
                // Check if all targets match.
                if(TestCandidateMatch(mce, v)){
                    // Match! Do not remove this conversion combination
                    return false;
                }
            }
            // No match found. Remove this combination.
            return true;
        }
    );
    // If no combinations remain, fail.
    if(combinations.size() == 0) return Result(Result::NONE);

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
        std::cerr << "Internal error: the above operations should not have emptied the combination list." << std::endl;
        return Result(Result::NONE);
    }else if(combinations_with_total_costs.size() > 1){
        //TODO: Somehow embed the information about viable candidates in the result returned
        return Result(Result::MULTIPLE);
    }
    // We have confirmed that there is indeed just one match. Let's unwrap it:
    std::vector<Conversion> best_match = combinations_with_total_costs.front().second;
    // Now find the candidate for this match.
    for(const MatchCandidateEntry& mce : candidates){
        if(!TestCandidateMatch(mce, best_match)) continue;

        // At this point, oe is the matching operator variant.
        // Finally, we can prepare the application function.
        Result::BatchConverterFunction bcf = [best_match](CodegenContext& ctx, std::vector<llvm::Value*> v) -> std::vector<llvm::Value*>{
            std::vector<llvm::Value*> result;
            if(v.size() != best_match.size()) return result;
            result.resize(v.size(), nullptr);

            // Conversions...
            for(unsigned int i = 0; i < v.size(); i++){
                result[i] = best_match[i].converter( ctx, v[i] );
            }
            return result;
        };
        return Result(Result::UNIQUE, mce, bcf);
    }
    // assert(false)
    std::cout << "Internal error: Match was found, but it has no corresponding operator variant." << std::endl;
    return Result(Result::NONE);
}

std::list<Conversion> TypeMatcher::ListTransitiveConversions(Type t) const{
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

    std::list<Conversion> result = t.deref()->ListConversions();

    // Identity conversion
    Conversion id{t,0,
            [](CodegenContext&, llvm::Value* v){return v;}
    };
    result.push_front(id);

    return result;
}

std::vector<std::list<Conversion>> TypeMatcher::InflateTypes(std::vector<Type> v) const{
    std::vector<std::list<Conversion>> result(v.size());
    std::transform(v.begin(), v.end(), result.begin(), [&](Type t){return ListTransitiveConversions(t);});
    return result;
}

}
