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
    CodegenContext(std::shared_ptr<Module> module)
        : TheModule(module),
          Builder(getGlobalContext())
    {
        BinOpCreator[std::make_tuple("ADD", DATATYPE_int, DATATYPE_int)] = 
            [this] (Value* LHS, Value* RHS) {
                return this->Builder.CreateAdd(LHS, RHS, "addtmp");
            };
            //std::bind(&IRBuilder<>::CreateAdd, &Builder, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
        BinOpCreator[std::make_tuple("SUB", DATATYPE_int, DATATYPE_int)] = 
            [this] (Value* LHS, Value* RHS) {
                return this->Builder.CreateSub(LHS, RHS, "subtmp");
            };
        BinOpCreator[std::make_tuple("MULT", DATATYPE_int, DATATYPE_int)] = 
            [this] (Value* LHS, Value* RHS) {
                return this->Builder.CreateMul(LHS, RHS, "multmp");
            };
        BinOpCreator[std::make_tuple("DIV", DATATYPE_int, DATATYPE_int)] = 
            [this] (Value* LHS, Value* RHS) {
                return this->Builder.CreateSDiv(LHS, RHS, "divtmp");
            };
        BinOpCreator[std::make_tuple("MOD", DATATYPE_int, DATATYPE_int)] = 
            [this] (Value* LHS, Value* RHS) {
                return this->Builder.CreateSRem(LHS, RHS, "modtmp");
            };
        
        BinOpCreator[std::make_tuple("EQUAL", DATATYPE_int, DATATYPE_int)] = 
            [this] (Value* LHS, Value* RHS) {
                return this->Builder.CreateICmpEQ(LHS, RHS, "cmptmp");
            };
        BinOpCreator[std::make_tuple("NEQUAL", DATATYPE_int, DATATYPE_int)] = 
            [this] (Value* LHS, Value* RHS) {
                return this->Builder.CreateICmpNE(LHS, RHS, "cmptmp");
            };
        
        BinOpCreator[std::make_tuple("GREATER", DATATYPE_int, DATATYPE_int)] = 
            [this] (Value* LHS, Value* RHS) {
                return this->Builder.CreateICmpUGT(LHS, RHS, "cmptmp");
            };
        BinOpCreator[std::make_tuple("GREATEREQ", DATATYPE_int, DATATYPE_int)] = 
            [this] (Value* LHS, Value* RHS) {
                return this->Builder.CreateICmpUGE(LHS, RHS, "cmptmp");
            };
        BinOpCreator[std::make_tuple("LESS", DATATYPE_int, DATATYPE_int)] = 
            [this] (Value* LHS, Value* RHS) {
                return this->Builder.CreateICmpULT(LHS, RHS, "cmptmp");
            };
        BinOpCreator[std::make_tuple("LESSEQ", DATATYPE_int, DATATYPE_int)] = 
            [this] (Value* LHS, Value* RHS) {
                return this->Builder.CreateICmpULE(LHS, RHS, "cmptmp");
            };
        
        BinOpCreator[std::make_tuple("LOGICAL_AND", DATATYPE_int, DATATYPE_int)] = 
            [this] (Value* LHS, Value* RHS) {
                return this->Builder.CreateAnd(LHS, RHS, "cmptmp");
            };
        BinOpCreator[std::make_tuple("LOGICAL_OR", DATATYPE_int, DATATYPE_int)] = 
            [this] (Value* LHS, Value* RHS) {
                return this->Builder.CreateOr(LHS, RHS, "cmptmp");
            };
    }

    std::shared_ptr<Module> TheModule;
    IRBuilder<> Builder;
    // std::map<std::string, Value*> CurrentFuncArgs; // Moved to VarsInScope
    Function* CurrentFunc;
    std::map<std::string, std::pair<AllocaInst*, datatype>> VarsInScope;
    
    std::map<std::tuple<std::string, datatype, datatype>, std::function<Value*(Value*, Value*)>> BinOpCreator;
    
    // For tracking in which loop we are currently in
    // .first -- headerBB, .second -- postBB
    std::list<std::pair<BasicBlock*, BasicBlock*>> LoopsBBHeaderPost;
    
    bool is_inside_loop () const { return !LoopsBBHeaderPost.empty(); }
    
    // Special function handles
    Function* func_printf;
};

#endif // __CODEGEN_CONTEXT_H__
