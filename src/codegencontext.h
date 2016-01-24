// -*- mode: c++; fill-column: 80 -*-
#ifndef __CODEGEN_CONTEXT_H__
#define __CODEGEN_CONTEXT_H__

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

#include <memory>
#include <list>

using namespace llvm;

class CodegenContext{
public:
    CodegenContext(std::shared_ptr<Module> module)
        : TheModule(module),
          Builder(getGlobalContext())
    {}

    std::shared_ptr<Module> TheModule;
    IRBuilder<> Builder;
    // std::map<std::string, Value*> CurrentFuncArgs; // Moved to VarsInScope
    Function* CurrentFunc;
    std::map<std::string, AllocaInst*> VarsInScope;
    
    // For tracking in which loop we are currently in
    std::list<std::pair<BasicBlock*, BasicBlock*>> LoopsBBHeaderPost;

    // Special function handles
    Function* func_printf;
};

#endif // __CODEGEN_CONTEXT_H__
