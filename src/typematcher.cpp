#include "typematcher.h"
#include "codegencontext.h"
#include <algorithm>
#include <queue>

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
    if(combinations.size() == 0)
        return Result(Result::NONE);

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
        Result::ConverterFunctions cfcs;
        for(size_t i = 0; i < best_match.size(); i++) {
            cfcs.push_back(
                [best_match, i](llvm::Value* v) -> llvm::Value* {
                    return best_match[i].converter(v);
                }
            );
        }
        return Result(Result::UNIQUE, mce, cfcs);
    }
    
    ctx.AddError("Internal error: Match was found, but it has no corresponding operator variant.");
    return Result(Result::NONE);
}

std::list<Conversion> TypeMatcher::ListTransitiveConversions(Type t) const{

    //                   ---  Dijkstra candidate distance ( conversioncost * -1)
    //                  /     --- Target type (search graph vertex)
    //                  |    /       ---- Candidate conversion function
    //                  |    |      /
    typedef std::tuple<int, Type, ConverterFunction> pq_elem;
    struct QCmp{
        bool operator()(const pq_elem &a,const pq_elem &b){
            if(std::get<0>(a) != std::get<0>(b)) return std::get<0>(a) < std::get<0>(b);
            if(std::get<1>(a) != std::get<1>(b)) return std::get<1>(a) < std::get<1>(b);
            return false; // has to be false, so that two different
                          // routes to the same element with the same
                          // cost are considered the same regardless of the conversion function
        }
    };
    std::priority_queue<pq_elem,std::vector<pq_elem>,QCmp> naiive_dijkstra_pq;
    std::map<Type, std::pair<int, ConverterFunction>> visited_vertices;

    // Initial vertex
    naiive_dijkstra_pq.push(pq_elem{0, t, [](llvm::Value* v){return v;} });

    while(!naiive_dijkstra_pq.empty()){
        auto current = naiive_dijkstra_pq.top();
        naiive_dijkstra_pq.pop();
        int current_mcost = std::get<0>(current);
        auto current_type = std::get<1>(current);
        auto current_convf = std::get<2>(current);

        if(visited_vertices.count(current_type) > 0) continue;

        // Store optimal distance to this vertex
        visited_vertices[current_type] = {current_mcost, current_convf};
        // Get a list of its neighbours
        std::list<Conversion> neigh = current_type->ListConversions();

        // Add all elements as search candidates
        for(Conversion conv : neigh){ // Cannot be a reference, a copy is needed to be captured in lambda
            naiive_dijkstra_pq.push(pq_elem{
                current_mcost - conv.cost,
                conv.target_type,
                [current_convf, conv](llvm::Value* v){
                    // Join path functions
                    return conv.converter(current_convf(v));
                }
            });
        }

    }

    std::list<Conversion> result;
    for(const auto &elem : visited_vertices){
        result.push_back(Conversion{elem.first, ConversionCost( - elem.second.first), elem.second.second});
    }
    return result;
}

std::vector<std::list<Conversion>> TypeMatcher::InflateTypes(std::vector<Type> v) const{
    std::vector<std::list<Conversion>> result(v.size());
    std::transform(v.begin(), v.end(), result.begin(), [&](Type t){return ListTransitiveConversions(t);});
    return result;
}

}
