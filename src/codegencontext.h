// -*- mode: c++; fill-column: 80 -*-
#ifndef __CODEGEN_CONTEXT_H__
#define __CODEGEN_CONTEXT_H__

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

#include <memory>

using namespace llvm;

class CodegenContext{
public:
    CodegenContext(std::shared_ptr<Module> module)
        : TheModule(module),
          Builder(getGlobalContext())
    {}

    std::shared_ptr<Module> TheModule;
    IRBuilder<> Builder;
    std::map<std::string, Value*> CurrentFuncArgs;
    Function* CurrentFunc;
};

#endif // __CODEGEN_CONTEXT_H__
