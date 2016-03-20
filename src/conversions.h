#ifndef __CONVERSIONS_H__
#define __CONVERSIONS_H__

#include "proxy.h"

#include "llvm/IR/Value.h"

#include <functional>
#include <map>

namespace ccoscope{

class CodegenContext;

typedef std::function<llvm::Value*(/*CodegenContext&,*/ llvm::Value*)> ConverterFunction;
typedef unsigned int ConversionCost;

class TypeAST; using Type = Proxy<TypeAST>;

struct Conversion{
    //Type orig_type;
    Type target_type;
    ConversionCost cost;
    ConverterFunction converter;
};

}

#endif // __CONVERSIONS_H__
