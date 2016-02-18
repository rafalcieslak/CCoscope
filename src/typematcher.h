// -*- mode: c++; fill-column: 80 -*-
#ifndef __TYPEMATCHER_H__
#define __TYPEMATCHER_H__

#include <string>
#include "types.h"

namespace ccoscope{

class OperatorEntry;
class CodegenContext;

class TypeMatcher{
public:
    TypeMatcher(const CodegenContext& c) : ctx(c) {}

    // Returns nullptr if no match or multiple matches, and writes corresponding
    // errors to the parent context.
    const OperatorEntry* MatchOperator(std::string name, Type t1, Type t2);

    // TODO: Implement function overloading
    // const FunctionEntry* MatchFunction(std::string name, std::list<Type> argtypes)
private:
    const CodegenContext& ctx;
};
}

#endif // __TYPEMATCHER_H__
