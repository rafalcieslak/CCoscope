#ifndef __CONVERSION_H__
#define __CONVERSION_H__

#include "../misc/proxy.h"

#include "llvm/IR/Value.h"

#include <functional>
#include <map>

namespace ccoscope{

class CodegenContext;

typedef std::function<llvm::Value*(llvm::Value*)> ConverterFunction;
typedef unsigned int ConversionCost;

class TypeAST; using Type = Proxy<TypeAST>;

struct Conversion{
    Type target_type;
    ConversionCost cost;
    ConverterFunction converter;
};

}

#endif // __CONVERSION_H__
