#include "codegencontext.h"
#include "utils.h"
#include <iostream>

namespace ccoscope {

using namespace std;
using namespace llvm;

// Creates a global i8 string. Useful for printing values.
llvm::Constant* CreateI8String(char const* str, CodegenContext& ctx) {
  auto strVal = ctx.Builder.CreateGlobalStringPtr(str);
  return llvm::cast<llvm::Constant>(strVal);
}

CodegenContext::CodegenContext()
    : Builder(getGlobalContext())
    , gid_(0)
{
    // Operators on integers

    BinOpCreator[std::make_tuple("ADD", getIntegerTy(), getIntegerTy())] =
        [this] (Value* LHS, Value* RHS) {
            return std::make_pair(this->Builder.CreateAdd(LHS, RHS, "addtmp"), getIntegerTy());
        };
    BinOpCreator[std::make_tuple("SUB", getIntegerTy(), getIntegerTy())] =
        [this] (Value* LHS, Value* RHS) {
            return std::make_pair(this->Builder.CreateSub(LHS, RHS, "subtmp"), getIntegerTy());
        };
    BinOpCreator[std::make_tuple("MULT", getIntegerTy(), getIntegerTy())] =
        [this] (Value* LHS, Value* RHS) {
            return std::make_pair(this->Builder.CreateMul(LHS, RHS, "multmp"), getIntegerTy());
        };
    BinOpCreator[std::make_tuple("DIV", getIntegerTy(), getIntegerTy())] =
        [this] (Value* LHS, Value* RHS) {
            return std::make_pair(this->Builder.CreateSDiv(LHS, RHS, "divtmp"), getIntegerTy());
        };
    BinOpCreator[std::make_tuple("MOD", getIntegerTy(), getIntegerTy())] =
        [this] (Value* LHS, Value* RHS) {
            return std::make_pair(this->Builder.CreateSRem(LHS, RHS, "modtmp"), getIntegerTy());
        };

    BinOpCreator[std::make_tuple("EQUAL", getIntegerTy(), getIntegerTy())] =
        [this] (Value* LHS, Value* RHS) {
            return std::make_pair(this->Builder.CreateICmpEQ(LHS, RHS, "cmptmp"), getBooleanTy());
        };
    BinOpCreator[std::make_tuple("NEQUAL", getIntegerTy(), getIntegerTy())] =
        [this] (Value* LHS, Value* RHS) {
            return std::make_pair(this->Builder.CreateICmpNE(LHS, RHS, "cmptmp"), getBooleanTy());
        };

    BinOpCreator[std::make_tuple("GREATER", getIntegerTy(), getIntegerTy())] =
        [this] (Value* LHS, Value* RHS) {
            return std::make_pair(this->Builder.CreateICmpSGT(LHS, RHS, "cmptmp"), getBooleanTy());
        };
    BinOpCreator[std::make_tuple("GREATEREQ", getIntegerTy(), getIntegerTy())] =
        [this] (Value* LHS, Value* RHS) {
            return std::make_pair(this->Builder.CreateICmpSGE(LHS, RHS, "cmptmp"), getBooleanTy());
        };
    BinOpCreator[std::make_tuple("LESS", getIntegerTy(), getIntegerTy())] =
        [this] (Value* LHS, Value* RHS) {
            return std::make_pair(this->Builder.CreateICmpSLT(LHS, RHS, "cmptmp"), getBooleanTy());
        };
    BinOpCreator[std::make_tuple("LESSEQ", getIntegerTy(), getIntegerTy())] =
        [this] (Value* LHS, Value* RHS) {
            return std::make_pair(this->Builder.CreateICmpSLE(LHS, RHS, "cmptmp"), getBooleanTy());
        };

    BinOpCreator[std::make_tuple("LOGICAL_AND", getBooleanTy(), getBooleanTy())] =
        [this] (Value* LHS, Value* RHS) {
            return std::make_pair(this->Builder.CreateAnd(LHS, RHS, "cmptmp"), getBooleanTy());
        };
    BinOpCreator[std::make_tuple("LOGICAL_OR", getBooleanTy(), getBooleanTy())] =
        [this] (Value* LHS, Value* RHS) {
            return std::make_pair(this->Builder.CreateOr(LHS, RHS, "cmptmp"), getBooleanTy());
        };

    // Operators on doubles

    BinOpCreator[std::make_tuple("ADD", getDoubleTy(), getDoubleTy())] =
        [this] (Value* LHS, Value* RHS) {
            return std::make_pair(this->Builder.CreateFAdd(LHS, RHS, "faddtmp"), getDoubleTy());
        };
    BinOpCreator[std::make_tuple("SUB", getDoubleTy(), getDoubleTy())] =
        [this] (Value* LHS, Value* RHS) {
            return std::make_pair(this->Builder.CreateFSub(LHS, RHS, "fsubtmp"), getDoubleTy());
        };
    BinOpCreator[std::make_tuple("MULT", getDoubleTy(), getDoubleTy())] =
        [this] (Value* LHS, Value* RHS) {
            return std::make_pair(this->Builder.CreateFMul(LHS, RHS, "fmultmp"), getDoubleTy());
        };
    BinOpCreator[std::make_tuple("DIV", getDoubleTy(), getDoubleTy())] =
        [this] (Value* LHS, Value* RHS) {
            return std::make_pair(this->Builder.CreateFDiv(LHS, RHS, "fdivtmp"), getDoubleTy());
        };
    BinOpCreator[std::make_tuple("MOD", getDoubleTy(), getDoubleTy())] =
        [this] (Value* LHS, Value* RHS) {
            return std::make_pair(this->Builder.CreateFRem(LHS, RHS, "fmodtmp"), getDoubleTy());
        };

    BinOpCreator[std::make_tuple("EQUAL", getDoubleTy(), getDoubleTy())] =
        [this] (Value* LHS, Value* RHS) {
            return std::make_pair(this->Builder.CreateFCmpOEQ(LHS, RHS, "fcmptmp"), getBooleanTy());
        };
    BinOpCreator[std::make_tuple("NEQUAL", getDoubleTy(), getDoubleTy())] =
        [this] (Value* LHS, Value* RHS) {
            return std::make_pair(this->Builder.CreateFCmpONE(LHS, RHS, "fcmptmp"), getBooleanTy());
        };

    BinOpCreator[std::make_tuple("GREATER", getDoubleTy(), getDoubleTy())] =
        [this] (Value* LHS, Value* RHS) {
            return std::make_pair(this->Builder.CreateFCmpOGT(LHS, RHS, "fcmptmp"), getBooleanTy());
        };
    BinOpCreator[std::make_tuple("GREATEREQ", getDoubleTy(), getDoubleTy())] =
        [this] (Value* LHS, Value* RHS) {
            return std::make_pair(this->Builder.CreateFCmpOGE(LHS, RHS, "fcmptmp"), getBooleanTy());
        };
    BinOpCreator[std::make_tuple("LESS", getDoubleTy(), getDoubleTy())] =
        [this] (Value* LHS, Value* RHS) {
            return std::make_pair(this->Builder.CreateFCmpOLT(LHS, RHS, "fcmptmp"), getBooleanTy());
        };
    BinOpCreator[std::make_tuple("LESSEQ", getDoubleTy(), getDoubleTy())] =
        [this] (Value* LHS, Value* RHS) {
            return std::make_pair(this->Builder.CreateFCmpOLE(LHS, RHS, "fcmptmp"), getBooleanTy());
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

CodegenContext::~CodegenContext() {
    for(auto& it : prototypes)
        delete it;
    for(auto& it : definitions)
        delete it;
    for(auto& it : expressions)
        delete it;
    for(auto& it : types)
        delete it;
}

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
    return introduceE(new VariableExprAST(*this, gid_++, name));
}

PrimitiveExpr<int> CodegenContext::makeInt(int value) {
    return introduceE(new PrimitiveExprAST<int>(*this, gid_++, value));
}

PrimitiveExpr<double> CodegenContext::makeDouble(double value) {
    return introduceE(new PrimitiveExprAST<double>(*this, gid_++, value));
}

PrimitiveExpr<bool> CodegenContext::makeBool(bool value) {
    return introduceE(new PrimitiveExprAST<bool>(*this, gid_++, value));
}

BinaryExpr CodegenContext::makeBinary(std::string Op, Expr LHS, Expr RHS) {
    return introduceE(new BinaryExprAST(*this, gid_++, Op, LHS, RHS));
}

ReturnExpr CodegenContext::makeReturn(Expr expr) {
    return introduceE(new ReturnExprAST(*this, gid_++, expr));
}

Block CodegenContext::makeBlock(const std::vector<std::pair<std::string, Type>> &vars, 
                                const std::list<Expr>& s) {
    return introduceE(new BlockAST(*this, gid_++, vars, s));
}

Assignment CodegenContext::makeAssignment(const std::string& Name, Expr expr) {
    return introduceE(new AssignmentAST(*this, gid_++, Name, expr));
}

CallExpr CodegenContext::makeCall(const std::string &Callee, std::vector<Expr> Args) {
    return introduceE(new CallExprAST(*this, gid_++, Callee, Args));
}

IfExpr CodegenContext::makeIf(Expr Cond, Expr Then, Expr Else) {
    return introduceE(new IfExprAST(*this, gid_++, Cond, Then, Else));
}

WhileExpr CodegenContext::makeWhile(Expr Cond, Expr Body) {
    return introduceE(new WhileExprAST(*this, gid_++, Cond, Body));
}

ForExpr CodegenContext::makeFor(Expr Init, Expr Cond, std::list<Expr> Step, Expr Body) {
    return introduceE(new ForExprAST(*this, gid_++, Init, Cond, Step, Body));
}

Keyword CodegenContext::makeKeyword(keyword which) {
    return introduceE(new KeywordAST(*this, gid_++, which));
}

Prototype CodegenContext::makePrototype(const std::string &Name, 
        std::vector<std::pair<std::string, Type>> Args, Type ReturnType)
{
    return introduce_prototype(new PrototypeAST(*this, gid_++, Name, Args, ReturnType));
}

Function CodegenContext::makeFunction(Prototype Proto, Expr Body) {
    return introduce_function(new FunctionAST(*this, gid_++, Proto, Body));
}

// ==---------------------------------------------------------------

// ==---------------------------------------------------------------
// Factory methods for Types

VoidType CodegenContext::getVoidTy() {
    return introduceT(new VoidTypeAST(*this, gid_++));
}

IntegerType CodegenContext::getIntegerTy() {
    return introduceT(new IntegerTypeAST(*this, gid_++));
}

DoubleType CodegenContext::getDoubleTy() {
    return introduceT(new DoubleTypeAST(*this, gid_++));
}

BooleanType CodegenContext::getBooleanTy() {
    return introduceT(new BooleanTypeAST(*this, gid_++));
}

FunctionType CodegenContext::getFunctionTy(Type ret, std::vector<Type> args) {
    return introduceT(new FunctionTypeAST(*this, gid_++, ret, args));
}

ReferenceType CodegenContext::getReferenceTy(Type of) {
    return introduceT(new ReferenceTypeAST(*this, gid_++, of));
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

const TypeAST* CodegenContext::introduce_expr(const TypeAST* node) {
    /* This look ridiculously simple now, but in the future we can
     * make CSE optimization here
     */ 
    types.insert(node);
    return node;
}

const PrototypeAST* introduce_prototype(const PrototypeAST* node) {
    if(auto pit = prototypesMap.find(node->Name) != prototypesMap.end()) {
        delete node;
        return pit->second;
    }

    prototypes.insert(node);
    prototypesMap[node->Name] = node;
    return node;
}

const FunctionAST* introduce_function(const FunctionAST* node) {
    definitions.insert(node);
    return node;
}

}
