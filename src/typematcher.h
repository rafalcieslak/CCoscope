// -*- mode: c++; fill-column: 80 -*-
#ifndef __TYPEMATCHER_H__
#define __TYPEMATCHER_H__

#include "conversions.h"
#include "types.h"
#include <string>
#include <list>
#include <vector>

namespace ccoscope{

struct MatchCandidateEntry{
    std::vector<Type> input_types;
    std::function<llvm::Value*(std::vector<llvm::Value*>)> associated_function;
    Type return_type;
};

class TypeMatcher{
public:
    TypeMatcher(const CodegenContext& c) : ctx(c) {}

    struct Result{
        typedef enum{
            NONE,
            UNIQUE,
            MULTIPLE
        } ResultType;
        ResultType type; // false if no match was found at all
        MatchCandidateEntry match;
        // This function performs matched conversion
        typedef std::function<std::vector<llvm::Value*>(CodegenContext&, std::vector<llvm::Value*> input)> BatchConverterFunction;
        BatchConverterFunction converter_function;

        Result(ResultType t = NONE) : type(t) {}
        Result(const ResultType& r, const MatchCandidateEntry& e, BatchConverterFunction func) :
            type(r), match(e), converter_function(func) {}
    };

    // Returns a TypeMatcher::Result, and writes corresponding errors to the
    // parent context.
    const Result Match(std::list<MatchCandidateEntry> candidates, std::vector<Type> input_signature) const;


private:
    // Reference to parent, used to get types.
    // TODO: Is this even used?
    const CodegenContext& ctx;

    // For a given type, returns a list of possible conversions, including an
    // identity conversion and an transitive conversions
    std::list<Conversion> ListTransitiveConversions(Type) const;
    // Replaces each type on the list with all possible conversions
    std::vector<std::list<Conversion>> InflateTypes(std::vector<Type>) const;
};
}

// Generates a list of possible combinations, each one is a vector whose nth
//  element is picked from nth list.  See input/output types, this is the only
//  reasonable pure function on these types.  On one hand, this is very
//  straight-forward combination generator. On the other, this is one of the
//  most wicked templates I've ever written. Note: This would be a one-liner in
//  any functional language.
template <typename T>
std::list<std::vector<T>> CombinationWalker(const std::vector<std::list<T>>& in){
    typedef typename std::list<T>::const_iterator list_it;
    std::list<std::vector<T>> result;
    unsigned int combination_size = in.size();
    std::vector<list_it> iterators(combination_size);
    // Initialize iterators with begins
    std::transform(in.begin(), in.end(), iterators.begin(), [](const std::list<T>& l){return l.begin();});
    // Generate combinations
    while(true){
        // Multidimentional dereference
        std::vector<T> current_combination(combination_size);
        std::transform(iterators.begin(), iterators.end(), current_combination.begin(), [](list_it it){return *it;});
        //std::cout << "Inserting a combination" << std::endl;
        result.push_back(current_combination);

        // Increment combination.
        for(unsigned int i = 0; i <= combination_size; i++){
            if(i == combination_size){
                // If incrementing the last iterator has overflown, it means we've found all possible combinations.
                return result;
            }
            iterators[i]++;
            if(iterators[i] == in[i].end()){
                // Carry
                iterators[i] = in[i].begin();
                continue;
            }else{
                // Done.
                break;
            }

        }
    }
}

#endif // __TYPEMATCHER_H__
