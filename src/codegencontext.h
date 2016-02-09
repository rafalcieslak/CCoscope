// -*- mode: c++; fill-column: 80 -*-
#ifndef __CODEGEN_CONTEXT_H__
#define __CODEGEN_CONTEXT_H__

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

#include <memory>
#include <list>

#include "typechecking.h"

using namespace llvm;

class CodegenContext
{
public:
    CodegenContext(std::shared_ptr<Module> module, std::string filename);

    std::shared_ptr<Module> TheModule;
    IRBuilder<> Builder;
    Function* CurrentFunc;
    std::map<std::string, std::pair<AllocaInst*, datatype>> VarsInScope;

    std::map<std::tuple<std::string, datatype, datatype>, std::function<Value*(Value*, Value*)>> BinOpCreator;

    // For tracking in which loop we are currently in
    // .first -- headerBB, .second -- postBB
    std::list<std::pair<BasicBlock*, BasicBlock*>> LoopsBBHeaderPost;

    bool is_inside_loop () const { return !LoopsBBHeaderPost.empty(); }

    // Special function handles
    Function* func_printf;

    // Stores an error-message. TODO: Add file positions storage.
    void AddError(std::string text);

    // Returns true iff no errors were stored.
    bool IsErrorFree();

    // Prints all stored errors to stdout.
    void DisplayErrors();

private:
    // Storage for error messages.
    std::list<std::pair<std::string, std::string>> errors;

    // The base input source file for this module
    std::string filename;
};

#endif // __CODEGEN_CONTEXT_H__
