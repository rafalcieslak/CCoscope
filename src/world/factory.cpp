#include "codegencontext.h"
#include "utils.h"
#include <iostream>

#include "../common/common_types.h"

namespace ccoscope {

using namespace std;
using namespace llvm;

// ==---------------------------------------------------------------
// Factory methods for AST nodes

VariableOccExpr CodegenContext::makeVariableOcc(std::string name, fileloc pos) {
    return IntroduceE_(new VariableOccExprAST(*this, gid_++, name, pos));
}

VariableDeclExpr CodegenContext::makeVariableDecl(std::string name, Type type, fileloc pos) {
    return IntroduceE_(new VariableDeclExprAST(*this, gid_++, name, type, pos));
}

PrimitiveExpr<int> CodegenContext::makeInt(int value, fileloc pos) {
    return IntroduceE_(new PrimitiveExprAST<int>(*this, gid_++, value, pos));
}

PrimitiveExpr<double> CodegenContext::makeDouble(double value, fileloc pos) {
    return IntroduceE_(new PrimitiveExprAST<double>(*this, gid_++, value, pos));
}

PrimitiveExpr<bool> CodegenContext::makeBool(bool value, fileloc pos) {
    return IntroduceE_(new PrimitiveExprAST<bool>(*this, gid_++, value, pos));
}

ComplexValue CodegenContext::makeComplex(Expr re, Expr im, fileloc pos) {
    return IntroduceE_(new ComplexValueAST(*this, gid_++, re, im, pos));
}

StringValue CodegenContext::makeString(std::string s, fileloc pos) {
    return IntroduceE_(new StringValueAST(*this, gid_++, s, pos));
}

BinaryExpr CodegenContext::makeBinary(std::string Op, Expr LHS, Expr RHS, fileloc pos) {
    return IntroduceE_(new BinaryExprAST(*this, gid_++, Op, LHS, RHS, pos));
}

ReturnExpr CodegenContext::makeReturn(Expr expr, fileloc pos) {
    return IntroduceE_(new ReturnExprAST(*this, gid_++, expr, pos));
}

Block CodegenContext::makeBlock(const std::list<Expr>& s, fileloc pos) {
    return IntroduceE_(new BlockAST(*this, gid_++, s, pos));
}

CallExpr CodegenContext::makeCall(const std::string &Callee, std::vector<Expr> Args, fileloc pos) {
    return IntroduceE_(new CallExprAST(*this, gid_++, Callee, Args, pos));
}

IfExpr CodegenContext::makeIf(Expr Cond, Expr Then, Expr Else, fileloc pos) {
    return IntroduceE_(new IfExprAST(*this, gid_++, Cond, Then, Else, pos));
}

WhileExpr CodegenContext::makeWhile(Expr Cond, Expr Body, fileloc pos) {
    return IntroduceE_(new WhileExprAST(*this, gid_++, Cond, Body, pos));
}

ForExpr CodegenContext::makeFor(Expr Init, Expr Cond, std::list<Expr> Step, Expr Body, fileloc pos) {
    return IntroduceE_(new ForExprAST(*this, gid_++, Init, Cond, Step, Body, pos));
}

LoopControlStmt CodegenContext::makeLoopControlStmt(loopControl which, fileloc pos) {
    return IntroduceE_(new LoopControlStmtAST(*this, gid_++, which, pos));
}

Prototype CodegenContext::makePrototype(const std::string &Name,
        std::vector<std::pair<std::string, Type>> Args, Type ReturnType, fileloc pos)
{
    return IntroducePrototype_(new PrototypeAST(*this, gid_++, Name, Args, ReturnType, pos));
}

Function CodegenContext::makeFunction(Prototype Proto, Expr Body, fileloc pos) {
    return IntroduceFunction_(new FunctionAST(*this, gid_++, Proto, Body, pos));
}

Convert CodegenContext::makeConvert(Expr Expression, Type ResultingType, std::function<llvm::Value*(llvm::Value*)> Converter, fileloc pos) {
    return IntroduceE_(new ConvertAST(*this, gid_++, Expression, ResultingType, Converter, pos));
}

// ==---------------------------------------------------------------

// ==---------------------------------------------------------------
// Factory methods for Types

VoidType CodegenContext::getVoidTy() {
    return IntroduceT_(new VoidTypeAST(*this, gid_++));
}

IntegerType CodegenContext::getIntegerTy() {
    return IntroduceT_(new IntegerTypeAST(*this, gid_++));
}

DoubleType CodegenContext::getDoubleTy() {
    return IntroduceT_(new DoubleTypeAST(*this, gid_++));
}

BooleanType CodegenContext::getBooleanTy() {
    return IntroduceT_(new BooleanTypeAST(*this, gid_++));
}

ComplexType CodegenContext::getComplexTy() {
    return IntroduceT_(new ComplexTypeAST(*this, gid_++));
}

StringType CodegenContext::getStringTy() {
    return IntroduceT_(new StringTypeAST(*this, gid_++));
}

FunctionType CodegenContext::getFunctionTy(Type ret, std::vector<Type> args) {
    return IntroduceT_(new FunctionTypeAST(*this, gid_++, ret, args));
}

ReferenceType CodegenContext::getReferenceTy(Type of) {
    return IntroduceT_(new ReferenceTypeAST(*this, gid_++, of));
}

const ExprAST* CodegenContext::IntroduceExpr_(const ExprAST* node) const{
    /* This look ridiculously simple now, but in the future we can
     * make CSE optimization here
     */
    expressions_.insert(node);
    return node;
}

const TypeAST* CodegenContext::IntroduceType_(const TypeAST* node) const{
    auto it = types_.find(node);
    if(it != types_.end() && *it != node) {
        delete node;
        return *it;
    }

    types_.insert(node);
    return node;
}

const PrototypeAST* CodegenContext::IntroducePrototype_(const PrototypeAST* node) const{
    auto pit = prototypesMap_.find(node->getName());
    if(pit != prototypesMap_.end()) {
        delete node;
        return *(pit->second);
    }

    prototypes_.insert(node);
    prototypesMap_[node->getName()] = node;
    return node;
}

const FunctionAST* CodegenContext::IntroduceFunction_(const FunctionAST* node) const{
    definitions_.insert(node);
    return node;
}

}
