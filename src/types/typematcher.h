// -*- mode: c++; fill-column: 80 -*-
#ifndef __TYPEMATCHER_H__
#define __TYPEMATCHER_H__

#include "conversion.h"
#include "types.h"
#include <string>
#include <list>
#include <vector>
#include <iostream>

namespace ccoscope{

// We probably should think about how to refine the interface here
// because this mechanism is used both for finding conversion of "normal"
// types and for finding overloads for functions -- in the former case,
// `return_type` doesn't make much sense and we need to do weird stuff
// then like putting there arbitrary type...

struct MatchCandidateEntry;
inline std::ostream& operator<<(std::ostream& s, const MatchCandidateEntry& mce);

struct MatchCandidateEntry{
    MatchCandidateEntry() {}
    MatchCandidateEntry(std::vector<Type> inpt, Type rett)
        : input_types(inpt), return_type(rett)
    {}
    
    std::vector<Type> input_types;
    Type return_type;
    
    void operator = (const MatchCandidateEntry& other) {
        if(this != &other) {
            input_types = other.input_types;
            return_type = other.return_type;
        }
    }
    
    bool operator < (const MatchCandidateEntry& other) const {
        return input_types < other.input_types ||
              (input_types == other.input_types &&
               return_type < other.return_type);
    }
};

inline std::ostream& operator<<(std::ostream& s, const MatchCandidateEntry& mce){
    s << "{";
    for(auto& p : mce.input_types) {
        s << p->name() << ", ";
    }
    s << "; " << mce.return_type->name() << "}";
    return s;
}

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
        typedef std::vector<std::function<llvm::Value*(llvm::Value*)>> ConverterFunctions;
        ConverterFunctions converter_functions;

        Result(ResultType t = NONE) : type(t) {}
        Result(const ResultType& r, const MatchCandidateEntry& e, ConverterFunctions cfuncs) :
            type(r), match(e), converter_functions(cfuncs) {}
    };

    // Returns a TypeMatcher::Result and writes corresponding errors to the parent context.
    const Result Match(std::list<MatchCandidateEntry> candidates, std::vector<Type> input_signature) const;


private:
    // Reference to parent, used to get types. (rather -- writing errors)
    // TODO: Is this even used? -> yes, it is : )
    const CodegenContext& ctx;

    // For a given type, returns a list of possible conversions, including an
    // identity conversion and transitive conversions
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
