// -*- mode: c++; fill-column: 80 -*-
#ifndef __TYPEMATCHER_H__
#define __TYPEMATCHER_H__

#include "types.h"
#include <string>
#include <list>

namespace ccoscope{

class OperatorEntry;
class CodegenContext;

typedef std::function<llvm::Value*(CodegenContext&, llvm::Value*)> ConverterFunction;
typedef unsigned int ConversionCost;

struct Conversion{
    Type orig_type;
    Type target_type;
    ConversionCost cost;
    ConverterFunction converter;
};


class TypeMatcher{
public:
    TypeMatcher(const CodegenContext& c) : ctx(c) {}

    void InitImplicitConversions();


    struct Result{
        bool found; // false if no match was found at all
        Type return_type;
        // This function performs matched conversion AND applies the corresponding operator.
        typedef std::function<llvm::Value*(CodegenContext&, std::vector<llvm::Value*>)> ApplicationFunction;
        ApplicationFunction application_function;
    private:
        friend class TypeMatcher; // Only a TypeMatcher can construct results
        Result() : found(false) {}
        Result(const bool& f, const Type& r, ApplicationFunction af) :
            found(f), return_type(r), application_function(af) {}
    };

    // Returns a TypemMatcher::Result, and writes corresponding errors to the
    // parent context.
    const Result MatchOperator(std::string name, Type t1, Type t2) const;

    // TODO: Implement function overloading, very similar to operator
    // resolution, but needs some kind of name-mangling first
    // const Result MatchFunction(std::string name, std::vector<Type> argtypes) const;


private:
    // Reference to parent, used to get types.
    const CodegenContext& ctx;

    // Storage for implicit conversions lists (graph?)
    std::map<Type, std::list<Conversion>, TypeCmp> implicit_conversions;

    // For a given type, returns a list of possible conversions (including an
    // identity conversion)
    std::list<Conversion> Inflate(Type) const;
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
