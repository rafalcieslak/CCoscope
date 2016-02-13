#include "codegencontext.h"
#include "utils.h"
#include <iostream>

namespace ccoscope {

using namespace std;
using namespace llvm;

CodegenContext::CodegenContext()
    : Builder(getGlobalContext())
    , gid_(0)
{
    // Operators on integers

    BinOpCreator[std::make_tuple("ADD", getIntegerTy(), getIntegerTy())] =
        [this] (Value* LHS, Value* RHS) {
            return this->Builder.CreateAdd(LHS, RHS, "addtmp");
        };
    BinOpCreator[std::make_tuple("SUB", getIntegerTy(), getIntegerTy())] =
        [this] (Value* LHS, Value* RHS) {
            return this->Builder.CreateSub(LHS, RHS, "subtmp");
        };
    BinOpCreator[std::make_tuple("MULT", getIntegerTy(), getIntegerTy())] =
        [this] (Value* LHS, Value* RHS) {
            return this->Builder.CreateMul(LHS, RHS, "multmp");
        };
    BinOpCreator[std::make_tuple("DIV", getIntegerTy(), getIntegerTy())] =
        [this] (Value* LHS, Value* RHS) {
            return this->Builder.CreateSDiv(LHS, RHS, "divtmp");
        };
    BinOpCreator[std::make_tuple("MOD", getIntegerTy(), getIntegerTy())] =
        [this] (Value* LHS, Value* RHS) {
            return this->Builder.CreateSRem(LHS, RHS, "modtmp");
        };

    BinOpCreator[std::make_tuple("EQUAL", getIntegerTy(), getIntegerTy())] =
        [this] (Value* LHS, Value* RHS) {
            return this->Builder.CreateICmpEQ(LHS, RHS, "cmptmp");
        };
    BinOpCreator[std::make_tuple("NEQUAL", getIntegerTy(), getIntegerTy())] =
        [this] (Value* LHS, Value* RHS) {
            return this->Builder.CreateICmpNE(LHS, RHS, "cmptmp");
        };

    BinOpCreator[std::make_tuple("GREATER", getIntegerTy(), getIntegerTy())] =
        [this] (Value* LHS, Value* RHS) {
            return this->Builder.CreateICmpSGT(LHS, RHS, "cmptmp");
        };
    BinOpCreator[std::make_tuple("GREATEREQ", getIntegerTy(), getIntegerTy())] =
        [this] (Value* LHS, Value* RHS) {
            return this->Builder.CreateICmpSGE(LHS, RHS, "cmptmp");
        };
    BinOpCreator[std::make_tuple("LESS", getIntegerTy(), getIntegerTy())] =
        [this] (Value* LHS, Value* RHS) {
            return this->Builder.CreateICmpSLT(LHS, RHS, "cmptmp");
        };
    BinOpCreator[std::make_tuple("LESSEQ", getIntegerTy(), getIntegerTy())] =
        [this] (Value* LHS, Value* RHS) {
            return this->Builder.CreateICmpSLE(LHS, RHS, "cmptmp");
        };

    BinOpCreator[std::make_tuple("LOGICAL_AND", getIntegerTy(), getIntegerTy())] =
        [this] (Value* LHS, Value* RHS) {
            return this->Builder.CreateAnd(LHS, RHS, "cmptmp");
        };
    BinOpCreator[std::make_tuple("LOGICAL_OR", getIntegerTy(), getIntegerTy())] =
        [this] (Value* LHS, Value* RHS) {
            return this->Builder.CreateOr(LHS, RHS, "cmptmp");
        };

    // Operators on doubles

    BinOpCreator[std::make_tuple("ADD", getDoubleTy(), getDoubleTy())] =
        [this] (Value* LHS, Value* RHS) {
            return this->Builder.CreateFAdd(LHS, RHS, "faddtmp");
        };
    BinOpCreator[std::make_tuple("SUB", getDoubleTy(), getDoubleTy())] =
        [this] (Value* LHS, Value* RHS) {
            return this->Builder.CreateFSub(LHS, RHS, "fsubtmp");
        };
    BinOpCreator[std::make_tuple("MULT", getDoubleTy(), getDoubleTy())] =
        [this] (Value* LHS, Value* RHS) {
            return this->Builder.CreateFMul(LHS, RHS, "fmultmp");
        };
    BinOpCreator[std::make_tuple("DIV", getDoubleTy(), getDoubleTy())] =
        [this] (Value* LHS, Value* RHS) {
            return this->Builder.CreateFDiv(LHS, RHS, "fdivtmp");
        };
    BinOpCreator[std::make_tuple("MOD", getDoubleTy(), getDoubleTy())] =
        [this] (Value* LHS, Value* RHS) {
            return this->Builder.CreateFRem(LHS, RHS, "fmodtmp");
        };

    BinOpCreator[std::make_tuple("EQUAL", getDoubleTy(), getDoubleTy())] =
        [this] (Value* LHS, Value* RHS) {
            return this->Builder.CreateFCmpOEQ(LHS, RHS, "fcmptmp");
        };
    BinOpCreator[std::make_tuple("NEQUAL", getDoubleTy(), getDoubleTy())] =
        [this] (Value* LHS, Value* RHS) {
            return this->Builder.CreateFCmpONE(LHS, RHS, "fcmptmp");
        };

    BinOpCreator[std::make_tuple("GREATER", getDoubleTy(), getDoubleTy())] =
        [this] (Value* LHS, Value* RHS) {
            return this->Builder.CreateFCmpOGT(LHS, RHS, "fcmptmp");
        };
    BinOpCreator[std::make_tuple("GREATEREQ", getDoubleTy(), getDoubleTy())] =
        [this] (Value* LHS, Value* RHS) {
            return this->Builder.CreateFCmpOGE(LHS, RHS, "fcmptmp");
        };
    BinOpCreator[std::make_tuple("LESS", getDoubleTy(), getDoubleTy())] =
        [this] (Value* LHS, Value* RHS) {
            return this->Builder.CreateFCmpOLT(LHS, RHS, "fcmptmp");
        };
    BinOpCreator[std::make_tuple("LESSEQ", getDoubleTy(), getDoubleTy())] =
        [this] (Value* LHS, Value* RHS) {
            return this->Builder.CreateFCmpOLE(LHS, RHS, "fcmptmp");
        };    
}
/*
/// Deprecated
CodegenContext::CodegenContext(std::shared_ptr<Module> module, std::string fname)
    : CodegenContext()
    , TheModule(module)
    , Builder(getGlobalContext())
    , filename(fname)
{}*/

bool TTypeCmp::operator () (const std::tuple<std::string, Type, Type>& lhs,
                      const std::tuple<std::string, Type, Type>& rhs) const {
    return std::get<1>(lhs)->gid() < std::get<1>(rhs)->gid() ||
           (std::get<1>(lhs)->gid() == std::get<1>(rhs)->gid() &&
            std::get<2>(lhs)->gid() < std::get<2>(rhs)->gid()
           );
}

// ==---------------------------------------------------------------
// Factory methods for AST nodes

VariableExpr CodegenContext::makeVariable(std::string name) {
    return introduce_expr(new VariableExprAST(*this, gid_++, name));
}

PrimitiveExpr<int> CodegenContext::makeInt(int value) {
    return introduce_expr(new PrimitiveExprAST<int>(*this, gid_++, value));
}

PrimitiveExpr<double> CodegenContext::makeDouble(double value) {
    return introduce_expr(new PrimitiveExprAST<double>(*this, gid_++, value));
}

PrimitiveExpr<bool> CodegenContext::makeBool(bool value) {
    return introduce_expr(new PrimitiveExprAST<bool>(*this, gid_++, value));
}

BinaryExpr CodegenContext::makeBinary(std::string Op, Expr LHS, Expr RHS) {
    return introduce_expr(new BinaryExprAST(*this, gid_++, Op, LHS, RHS));
}

ReturnExpr CodegenContext::makeReturn(Expr expr) {
    return introduce_expr(new ReturnExprAST(*this, gid_++, expr));
}

Block CodegenContext::makeBlock(const std::vector<std::pair<std::string, CCType>> &vars, const std::list<Expr>& s) {
    return introduce_expr(new BlockAST(*this, gid_++, vars, s));
}

Assignment CodegenContext::makeAssignment(const std::string& Name, Expr expr) {
    return introduce_expr(new AssignmentAST(*this, gid_++, Name, expr));
}

CallExpr CodegenContext::makeCall(const std::string &Callee, std::vector<Expr> Args) {
    return introduce_expr(new CallExprAST(*this, gid_++, Callee, Args));
}

IfExpr CodegenContext::makeIf(Expr Cond, Expr Then, Expr Else) {
    return introduce_expr(new IfExprAST(*this, gid_++, Cond, Then, Else));
}

WhileExpr CodegenContext::makeWhile(Expr Cond, Expr Body) {
    return introduce_expr(new WhileExprAST(*this, gid_++, Cond, Body));
}

ForExpr CodegenContext::makeFor(Expr Init, Expr Cond, std::list<Expr> Step, Expr Body) {
    return introduce_expr(new ForExprAST(*this, gid_++, Init, Cond, Step, Body));
}

Keyword CodegenContext::makeKeyword(keyword which) {
    return introduce_expr(new KeywordAST(*this, gid_++, which));
}

Prototype CodegenContext::makePrototype(const std::string &Name, 
        std::vector<std::pair<std::string, CCType>> Args, CCType ReturnType)
{
    auto nprot = new PrototypeAST(*this, gid_++, Name, Args, ReturnType);
    prototypes.insert(nprot);
    return nprot;
}

Function CodegenContext::makeFunction(Prototype Proto, Expr Body) {
    auto nfun = new FunctionAST(*this, gid_++, Proto, Body);
    definitions.insert(nfun);
    return nfun;
}

// ==---------------------------------------------------------------

// ==---------------------------------------------------------------
// Factory methods for Types

VoidType getVoidTy() {
    return introduce_type(new VoidTypeAST(*this, gid_++));
}

IntegerType getIntegerTy() {
    return introduce_type(new IntegerTypeAST(*this, gid_++));
}

DoubleType getDoubleTy() {
    return introduce_type(new DoubleTypeAST(*this, gid_++));
}

BooleanType getBooleanTy() {
    return introduce_type(new BooleanTypeAST(*this, gid_++));
}

FunctionType getFunctionTy(Type ret, std::vector<Type> args) {
    return introduce_type(new FunctionTypeAST(*this, gid_++, ret, args));
}

ReferenceType getReferenceTy(Type of) {
    return introduce_type(new ReferenceTypeAST(*this, gid_++, of));
}

// ==---------------------------------------------------------------

void CodegenContext::SetModuleAndFile(std::shared_ptr<llvm::Module> module, std::string infile) {
    TheModule = module;
    filename = infile;
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

const ExprAST* CodegenContext::introduce_expr(const ExprAST* node) {
    /* This look ridiculously simple now, but in the future we can
     * make CSE optimization here
     */ 
    expressions.insert(node);
    return node;
}

}
