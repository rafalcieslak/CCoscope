#ifndef __CONVERSIONS_H__
#define __CONVERSIONS_H__

#include "types.h"

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

}

#endif // __CONVERSIONS_H__
