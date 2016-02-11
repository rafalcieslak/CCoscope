#include "codegencontext.h"
#include "utils.h"
#include <iostream>

using namespace std;
using namespace llvm;

CodegenContext::CodegenContext(std::shared_ptr<Module> module, std::string fname)
    : TheModule(module),
      Builder(getGlobalContext()),
      filename(fname)
{
    // Operators on integers

    BinOpCreator[std::make_tuple("ADD", CCIntegerType(), CCIntegerType())] =
        [this] (Value* LHS, Value* RHS) {
            return this->Builder.CreateAdd(LHS, RHS, "addtmp");
        };
        //std::bind(&IRBuilder<>::CreateAdd, &Builder, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    BinOpCreator[std::make_tuple("SUB", CCIntegerType(), CCIntegerType())] =
        [this] (Value* LHS, Value* RHS) {
            return this->Builder.CreateSub(LHS, RHS, "subtmp");
        };
    BinOpCreator[std::make_tuple("MULT", CCIntegerType(), CCIntegerType())] =
        [this] (Value* LHS, Value* RHS) {
            return this->Builder.CreateMul(LHS, RHS, "multmp");
        };
    BinOpCreator[std::make_tuple("DIV", CCIntegerType(), CCIntegerType())] =
        [this] (Value* LHS, Value* RHS) {
            return this->Builder.CreateSDiv(LHS, RHS, "divtmp");
        };
    BinOpCreator[std::make_tuple("MOD", CCIntegerType(), CCIntegerType())] =
        [this] (Value* LHS, Value* RHS) {
            return this->Builder.CreateSRem(LHS, RHS, "modtmp");
        };

    BinOpCreator[std::make_tuple("EQUAL", CCIntegerType(), CCIntegerType())] =
        [this] (Value* LHS, Value* RHS) {
            return this->Builder.CreateICmpEQ(LHS, RHS, "cmptmp");
        };
    BinOpCreator[std::make_tuple("NEQUAL", CCIntegerType(), CCIntegerType())] =
        [this] (Value* LHS, Value* RHS) {
            return this->Builder.CreateICmpNE(LHS, RHS, "cmptmp");
        };

    BinOpCreator[std::make_tuple("GREATER", CCIntegerType(), CCIntegerType())] =
        [this] (Value* LHS, Value* RHS) {
            return this->Builder.CreateICmpSGT(LHS, RHS, "cmptmp");
        };
    BinOpCreator[std::make_tuple("GREATEREQ", CCIntegerType(), CCIntegerType())] =
        [this] (Value* LHS, Value* RHS) {
            return this->Builder.CreateICmpSGE(LHS, RHS, "cmptmp");
        };
    BinOpCreator[std::make_tuple("LESS", CCIntegerType(), CCIntegerType())] =
        [this] (Value* LHS, Value* RHS) {
            return this->Builder.CreateICmpSLT(LHS, RHS, "cmptmp");
        };
    BinOpCreator[std::make_tuple("LESSEQ", CCIntegerType(), CCIntegerType())] =
        [this] (Value* LHS, Value* RHS) {
            return this->Builder.CreateICmpSLE(LHS, RHS, "cmptmp");
        };

    BinOpCreator[std::make_tuple("LOGICAL_AND", CCIntegerType(), CCIntegerType())] =
        [this] (Value* LHS, Value* RHS) {
            return this->Builder.CreateAnd(LHS, RHS, "cmptmp");
        };
    BinOpCreator[std::make_tuple("LOGICAL_OR", CCIntegerType(), CCIntegerType())] =
        [this] (Value* LHS, Value* RHS) {
            return this->Builder.CreateOr(LHS, RHS, "cmptmp");
        };

    // Operators on doubles

    BinOpCreator[std::make_tuple("ADD", CCDoubleType(), CCDoubleType())] =
        [this] (Value* LHS, Value* RHS) {
            return this->Builder.CreateFAdd(LHS, RHS, "faddtmp");
        };
    BinOpCreator[std::make_tuple("SUB", CCDoubleType(), CCDoubleType())] =
        [this] (Value* LHS, Value* RHS) {
            return this->Builder.CreateFSub(LHS, RHS, "fsubtmp");
        };
    BinOpCreator[std::make_tuple("MULT", CCDoubleType(), CCDoubleType())] =
        [this] (Value* LHS, Value* RHS) {
            return this->Builder.CreateFMul(LHS, RHS, "fmultmp");
        };
    BinOpCreator[std::make_tuple("DIV", CCDoubleType(), CCDoubleType())] =
        [this] (Value* LHS, Value* RHS) {
            return this->Builder.CreateFDiv(LHS, RHS, "fdivtmp");
        };
    BinOpCreator[std::make_tuple("MOD", CCDoubleType(), CCDoubleType())] =
        [this] (Value* LHS, Value* RHS) {
            return this->Builder.CreateFRem(LHS, RHS, "fmodtmp");
        };

    BinOpCreator[std::make_tuple("EQUAL", CCDoubleType(), CCDoubleType())] =
        [this] (Value* LHS, Value* RHS) {
            return this->Builder.CreateFCmpOEQ(LHS, RHS, "fcmptmp");
        };
    BinOpCreator[std::make_tuple("NEQUAL", CCDoubleType(), CCDoubleType())] =
        [this] (Value* LHS, Value* RHS) {
            return this->Builder.CreateFCmpONE(LHS, RHS, "fcmptmp");
        };

    BinOpCreator[std::make_tuple("GREATER", CCDoubleType(), CCDoubleType())] =
        [this] (Value* LHS, Value* RHS) {
            return this->Builder.CreateFCmpOGT(LHS, RHS, "fcmptmp");
        };
    BinOpCreator[std::make_tuple("GREATEREQ", CCDoubleType(), CCDoubleType())] =
        [this] (Value* LHS, Value* RHS) {
            return this->Builder.CreateFCmpOGE(LHS, RHS, "fcmptmp");
        };
    BinOpCreator[std::make_tuple("LESS", CCDoubleType(), CCDoubleType())] =
        [this] (Value* LHS, Value* RHS) {
            return this->Builder.CreateFCmpOLT(LHS, RHS, "fcmptmp");
        };
    BinOpCreator[std::make_tuple("LESSEQ", CCDoubleType(), CCDoubleType())] =
        [this] (Value* LHS, Value* RHS) {
            return this->Builder.CreateFCmpOLE(LHS, RHS, "fcmptmp");
        };
}

void CodegenContext::AddError(std::string text){
    errors.push_back(std::make_pair(CurrentFunc->getName(), text));
}

bool CodegenContext::IsErrorFree(){
    return errors.empty();
}

void CodegenContext::DisplayErrors(){
    for(const auto& e : errors){
        std::cout << ColorStrings::Color(Color::White, true) << filename << ": " << ColorStrings::Color(Color::Red, true) << "ERROR" << ColorStrings::Reset();
        std::cout << " in function `" << e.first << "`: " << e.second << std::endl;
    }
}
