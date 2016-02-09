#include "codegencontext.h"
#include "utils.h"
#include <iostream>

CodegenContext::CodegenContext(std::shared_ptr<Module> module, std::string fname)
    : TheModule(module),
      Builder(getGlobalContext()),
      filename(fname)
{
    // Operators on integers

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
            return this->Builder.CreateICmpSGT(LHS, RHS, "cmptmp");
        };
    BinOpCreator[std::make_tuple("GREATEREQ", DATATYPE_int, DATATYPE_int)] =
        [this] (Value* LHS, Value* RHS) {
            return this->Builder.CreateICmpSGE(LHS, RHS, "cmptmp");
        };
    BinOpCreator[std::make_tuple("LESS", DATATYPE_int, DATATYPE_int)] =
        [this] (Value* LHS, Value* RHS) {
            return this->Builder.CreateICmpSLT(LHS, RHS, "cmptmp");
        };
    BinOpCreator[std::make_tuple("LESSEQ", DATATYPE_int, DATATYPE_int)] =
        [this] (Value* LHS, Value* RHS) {
            return this->Builder.CreateICmpSLE(LHS, RHS, "cmptmp");
        };

    BinOpCreator[std::make_tuple("LOGICAL_AND", DATATYPE_int, DATATYPE_int)] =
        [this] (Value* LHS, Value* RHS) {
            return this->Builder.CreateAnd(LHS, RHS, "cmptmp");
        };
    BinOpCreator[std::make_tuple("LOGICAL_OR", DATATYPE_int, DATATYPE_int)] =
        [this] (Value* LHS, Value* RHS) {
            return this->Builder.CreateOr(LHS, RHS, "cmptmp");
        };

    // Operators on doubles

    BinOpCreator[std::make_tuple("ADD", DATATYPE_double, DATATYPE_double)] =
        [this] (Value* LHS, Value* RHS) {
            return this->Builder.CreateFAdd(LHS, RHS, "faddtmp");
        };
    BinOpCreator[std::make_tuple("SUB", DATATYPE_double, DATATYPE_double)] =
        [this] (Value* LHS, Value* RHS) {
            return this->Builder.CreateFSub(LHS, RHS, "fsubtmp");
        };
    BinOpCreator[std::make_tuple("MULT", DATATYPE_double, DATATYPE_double)] =
        [this] (Value* LHS, Value* RHS) {
            return this->Builder.CreateFMul(LHS, RHS, "fmultmp");
        };
    BinOpCreator[std::make_tuple("DIV", DATATYPE_double, DATATYPE_double)] =
        [this] (Value* LHS, Value* RHS) {
            return this->Builder.CreateFDiv(LHS, RHS, "fdivtmp");
        };
    BinOpCreator[std::make_tuple("MOD", DATATYPE_double, DATATYPE_double)] =
        [this] (Value* LHS, Value* RHS) {
            return this->Builder.CreateFRem(LHS, RHS, "fmodtmp");
        };

    BinOpCreator[std::make_tuple("EQUAL", DATATYPE_double, DATATYPE_double)] =
        [this] (Value* LHS, Value* RHS) {
            return this->Builder.CreateFCmpOEQ(LHS, RHS, "fcmptmp");
        };
    BinOpCreator[std::make_tuple("NEQUAL", DATATYPE_double, DATATYPE_double)] =
        [this] (Value* LHS, Value* RHS) {
            return this->Builder.CreateFCmpONE(LHS, RHS, "fcmptmp");
        };

    BinOpCreator[std::make_tuple("GREATER", DATATYPE_double, DATATYPE_double)] =
        [this] (Value* LHS, Value* RHS) {
            return this->Builder.CreateFCmpOGT(LHS, RHS, "fcmptmp");
        };
    BinOpCreator[std::make_tuple("GREATEREQ", DATATYPE_double, DATATYPE_double)] =
        [this] (Value* LHS, Value* RHS) {
            return this->Builder.CreateFCmpOGE(LHS, RHS, "fcmptmp");
        };
    BinOpCreator[std::make_tuple("LESS", DATATYPE_double, DATATYPE_double)] =
        [this] (Value* LHS, Value* RHS) {
            return this->Builder.CreateFCmpOLT(LHS, RHS, "fcmptmp");
        };
    BinOpCreator[std::make_tuple("LESSEQ", DATATYPE_double, DATATYPE_double)] =
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
