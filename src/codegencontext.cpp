#include "codegencontext.h"
#include "utils.h"
#include <iostream>

namespace ccoscope {

using namespace std;
using namespace llvm;

CodegenContext::CodegenContext()
    : typematcher(*this)
    , builder_(getGlobalContext())
    , gid_(0)
{
    // Operators on integers

#define INIT_OP(name) availableBinOps_[name] = std::list<MatchCandidateEntry>()

#define ADD_BASIC_OP(name, t1, t2, builderfunc, rettype, retname) \
    availableBinOps_[name].push_back(MatchCandidateEntry{{t1, t2}, rettype}); \
    binOpCreator_[{name, MatchCandidateEntry{{t1, t2}, rettype}}] = \
        [this] (std::vector<Value*> v) { \
            return this->builder_.builderfunc(v[0], v[1], retname);\
       }
       
#define ADD_ASSIGN_OP(t) \
    availableBinOps_["ASSIGN"].push_back(MatchCandidateEntry{{getReferenceTy(t), t}, getVoidTy()}); \
    binOpCreator_[{"ASSIGN", MatchCandidateEntry{{getReferenceTy(t), t}, getVoidTy()}}] = \
        [this] (std::vector<Value*> v) { \
            return this->builder_.CreateStore(v[1], v[0]); \
        }

    // We wouldn't need that if STL provided a `defaultdict`.
    INIT_OP("ADD");
    INIT_OP("SUB");
    INIT_OP("MULT");
    INIT_OP("DIV");
    INIT_OP("MOD");
    INIT_OP("EQUAL");
    INIT_OP("NEQUAL");
    INIT_OP("GREATER");
    INIT_OP("GREATEREQ");
    INIT_OP("LESS");
    INIT_OP("LESSEQ");
    INIT_OP("LOGICAL_AND");
    INIT_OP("LOGICAL_OR");
    INIT_OP("ASSIGN");
    
    ADD_ASSIGN_OP(getIntegerTy());
    ADD_ASSIGN_OP(getBooleanTy());
    ADD_ASSIGN_OP(getDoubleTy());
    ADD_ASSIGN_OP(getComplexTy());
    
    ADD_BASIC_OP("ADD",    getIntegerTy(), getIntegerTy(), CreateAdd,  getIntegerTy(), "addtmp");
    ADD_BASIC_OP("SUB",    getIntegerTy(), getIntegerTy(), CreateSub,  getIntegerTy(), "subtmp");
    ADD_BASIC_OP("MULT",   getIntegerTy(), getIntegerTy(), CreateMul,  getIntegerTy(), "multmp");
    ADD_BASIC_OP("DIV",    getIntegerTy(), getIntegerTy(), CreateSDiv, getIntegerTy(), "divtmp");
    ADD_BASIC_OP("MOD",    getIntegerTy(), getIntegerTy(), CreateSRem, getIntegerTy(), "modtmp");
    
    ADD_BASIC_OP("EQUAL",    getIntegerTy(), getIntegerTy(), CreateICmpEQ,  getBooleanTy(), "cmptmp");
    ADD_BASIC_OP("NEQUAL",   getIntegerTy(), getIntegerTy(), CreateICmpNE,  getBooleanTy(), "cmptmp");
    ADD_BASIC_OP("GREATER",  getIntegerTy(), getIntegerTy(), CreateICmpSGT, getBooleanTy(), "cmptmp");
    ADD_BASIC_OP("GREATEREQ",getIntegerTy(), getIntegerTy(), CreateICmpSGE, getBooleanTy(), "cmptmp");
    ADD_BASIC_OP("LESS",     getIntegerTy(), getIntegerTy(), CreateICmpSLT, getBooleanTy(), "cmptmp");
    ADD_BASIC_OP("LESSEQ",   getIntegerTy(), getIntegerTy(), CreateICmpSLE, getBooleanTy(), "cmptmp");
    
    ADD_BASIC_OP("LOGICAL_AND", getBooleanTy(), getBooleanTy(), CreateAnd,  getBooleanTy(), "andtmp");
    ADD_BASIC_OP("LOGICAL_OR" , getBooleanTy(), getBooleanTy(), CreateOr,   getBooleanTy(), "ortmp" );

    ADD_BASIC_OP("ADD",  getDoubleTy(), getDoubleTy(), CreateFAdd,  getDoubleTy(), "addtmp");
    ADD_BASIC_OP("SUB",  getDoubleTy(), getDoubleTy(), CreateFSub,  getDoubleTy(), "subtmp");
    ADD_BASIC_OP("MULT", getDoubleTy(), getDoubleTy(), CreateFMul,  getDoubleTy(), "multmp");
    ADD_BASIC_OP("DIV",  getDoubleTy(), getDoubleTy(), CreateFDiv,  getDoubleTy(), "divtmp");
    ADD_BASIC_OP("MOD",  getDoubleTy(), getDoubleTy(), CreateFRem,  getDoubleTy(), "modtmp");

    ADD_BASIC_OP("EQUAL",    getDoubleTy(), getDoubleTy(), CreateFCmpOEQ, getBooleanTy(), "cmptmp");
    ADD_BASIC_OP("NEQUAL",   getDoubleTy(), getDoubleTy(), CreateFCmpONE, getBooleanTy(), "cmptmp");
    ADD_BASIC_OP("GREATER",  getDoubleTy(), getDoubleTy(), CreateFCmpOGT, getBooleanTy(), "cmptmp");
    ADD_BASIC_OP("GREATEREQ",getDoubleTy(), getDoubleTy(), CreateFCmpOGE, getBooleanTy(), "cmptmp");
    ADD_BASIC_OP("LESS",     getDoubleTy(), getDoubleTy(), CreateFCmpOLT, getBooleanTy(), "cmptmp");
    ADD_BASIC_OP("LESSEQ",   getDoubleTy(), getDoubleTy(), CreateFCmpOLE, getBooleanTy(), "cmptmp");
    
#define ADD_COMPLEX_OP(name, variadiccode, rettype, retname) \
    availableBinOps_[name].push_back(MatchCandidateEntry{{getComplexTy(), getComplexTy()}, rettype}); \
    binOpCreator_[{name, MatchCandidateEntry{{getComplexTy(), getComplexTy()}, rettype}}] = \
       [this] (std::vector<Value*> v){   \
            auto c1re = this->builder_.CreateExtractValue(v[0], {0}); \
            auto c1im = this->builder_.CreateExtractValue(v[0], {1}); \
            auto c2re = this->builder_.CreateExtractValue(v[1], {0}); \
            auto c2im = this->builder_.CreateExtractValue(v[1], {1}); \
            variadiccode \
            auto cmplx_t = getComplexTy()->toLLVMs(); \
            AllocaInst* alloca = CreateEntryBlockAlloca(CurrentFunc(), "cmplxtmp", cmplx_t); \
            auto idx1 = this->builder_.CreateStructGEP(cmplx_t, alloca, 0); \
            this->builder_.CreateStore(reres, idx1); \
            auto idx2 = this->builder_.CreateStructGEP(cmplx_t, alloca, 1); \
            this->builder_.CreateStore(imres, idx2); \
            auto retsload = this->builder_.CreateLoad(alloca, "Cmplxloadret"); \
            return retsload; \
       }

    ADD_COMPLEX_OP("ADD",
            auto reres = this->builder_.CreateFAdd(c1re, c2re, "cmplxaddtmp");
            auto imres = this->builder_.CreateFAdd(c1im, c2im, "cmplxaddtmp");
     , getComplexTy(), "addtmp");

    ADD_COMPLEX_OP("SUB",
            auto reres = this->builder_.CreateFSub(c1re, c2re, "cmplxsubtmp");
            auto imres = this->builder_.CreateFSub(c1im, c2im, "cmplxsubtmp");
     , getComplexTy(), "subtmp");

    ADD_COMPLEX_OP("MULT",
            auto c1c2re = this->builder_.CreateFMul(c1re, c2re, "cmplxmultmp");
            auto c1c2im = this->builder_.CreateFMul(c1im, c2im, "cmplxmultmp");
            auto c1imc2re = this->builder_.CreateFMul(c1im, c2re, "cmplxmultmp");
            auto c1rec2im = this->builder_.CreateFMul(c1re, c2im, "cmplxmultmp");
            auto reres = this->builder_.CreateFSub(c1c2re, c1c2im, "cmplxsubtmp");
            auto imres = this->builder_.CreateFAdd(c1imc2re, c1rec2im, "cmplxaddtmp");
     , getComplexTy(), "multmp");

    ADD_COMPLEX_OP("DIV",
            auto c1c2re = this->builder_.CreateFMul(c1re, c2re, "cmplxmultmp");
            auto c1c2im = this->builder_.CreateFMul(c1im, c2im, "cmplxmultmp");
            auto c1imc2re = this->builder_.CreateFMul(c1im, c2re, "cmplxmultmp");
            auto c1rec2im = this->builder_.CreateFMul(c1re, c2im, "cmplxmultmp");
            auto c2rere = this->builder_.CreateFMul(c2re, c2re, "cmplxmultmp");
            auto c2imim = this->builder_.CreateFMul(c2im, c2im, "cmplxmultmp");
            auto squares = this->builder_.CreateFAdd(c2rere, c2imim, "cmplxaddtmp");
            auto left = this->builder_.CreateFAdd(c1c2re, c1c2im, "cmplxsubtmp");
            auto right = this->builder_.CreateFSub(c1imc2re, c1rec2im, "cmplxaddtmp");
            auto reres = this->builder_.CreateFDiv(left, squares, "cmplxdivtmp");
            auto imres = this->builder_.CreateFDiv(right, squares, "cmplxdivtmp");
     , getComplexTy(), "divtmp");
    
    availableBinOps_["EQUAL"].push_back(MatchCandidateEntry{{getComplexTy(), getComplexTy()}, getBooleanTy()});
    binOpCreator_[{"EQUAL", MatchCandidateEntry{{getComplexTy(), getComplexTy()}, getBooleanTy()}}] =
       [this] (std::vector<Value*> v){
            auto c1re = this->builder_.CreateExtractValue(v[0], {0});
            auto c1im = this->builder_.CreateExtractValue(v[0], {1});
            auto c2re = this->builder_.CreateExtractValue(v[1], {0});
            auto c2im = this->builder_.CreateExtractValue(v[1], {1});
            auto c1c2re = this->builder_.CreateFCmpOEQ(c1re, c2re, "cmplxcmptmp");
            auto c1c2im = this->builder_.CreateFCmpOEQ(c1im, c2im, "cmplxcmptmp");
            return this->builder_.CreateAnd(c1c2re, c1c2im, "cmplxcmptmp");
       };
}

CodegenContext::~CodegenContext() {
    for(auto& it : prototypes_)
        delete it;
    for(auto& it : definitions_)
        delete it;
    for(auto& it : expressions_)
        delete it;
    for(auto& it : types_)
        delete it;
}


// ==---------------------------------------------------------------
// Factory methods for AST nodes

VariableExpr CodegenContext::makeVariable(std::string name) {
    return IntroduceE_(new VariableExprAST(*this, gid_++, name));
}

PrimitiveExpr<int> CodegenContext::makeInt(int value) {
    return IntroduceE_(new PrimitiveExprAST<int>(*this, gid_++, value));
}

PrimitiveExpr<double> CodegenContext::makeDouble(double value) {
    return IntroduceE_(new PrimitiveExprAST<double>(*this, gid_++, value));
}

PrimitiveExpr<bool> CodegenContext::makeBool(bool value) {
    return IntroduceE_(new PrimitiveExprAST<bool>(*this, gid_++, value));
}

ComplexValue CodegenContext::makeComplex(Expr re, Expr im) {
    return IntroduceE_(new ComplexValueAST(*this, gid_++, re, im));
}

BinaryExpr CodegenContext::makeBinary(std::string Op, Expr LHS, Expr RHS) {
    return IntroduceE_(new BinaryExprAST(*this, gid_++, Op, LHS, RHS));
}

ReturnExpr CodegenContext::makeReturn(Expr expr) {
    return IntroduceE_(new ReturnExprAST(*this, gid_++, expr));
}

Block CodegenContext::makeBlock(const std::vector<std::pair<std::string, Type>> &vars,
                                const std::list<Expr>& s) {
    return IntroduceE_(new BlockAST(*this, gid_++, vars, s));
}

CallExpr CodegenContext::makeCall(const std::string &Callee, std::vector<Expr> Args) {
    return IntroduceE_(new CallExprAST(*this, gid_++, Callee, Args));
}

IfExpr CodegenContext::makeIf(Expr Cond, Expr Then, Expr Else) {
    return IntroduceE_(new IfExprAST(*this, gid_++, Cond, Then, Else));
}

WhileExpr CodegenContext::makeWhile(Expr Cond, Expr Body) {
    return IntroduceE_(new WhileExprAST(*this, gid_++, Cond, Body));
}

ForExpr CodegenContext::makeFor(Expr Init, Expr Cond, std::list<Expr> Step, Expr Body) {
    return IntroduceE_(new ForExprAST(*this, gid_++, Init, Cond, Step, Body));
}

LoopControlStmt CodegenContext::makeLoopControlStmt(loopControl which) {
    return IntroduceE_(new LoopControlStmtAST(*this, gid_++, which));
}

Prototype CodegenContext::makePrototype(const std::string &Name,
        std::vector<std::pair<std::string, Type>> Args, Type ReturnType)
{
    return IntroducePrototype_(new PrototypeAST(*this, gid_++, Name, Args, ReturnType));
}

Function CodegenContext::makeFunction(Prototype Proto, Expr Body) {
    return IntroduceFunction_(new FunctionAST(*this, gid_++, Proto, Body));
}

Convert CodegenContext::makeConvert(Expr Expression, Type ResultingType, std::function<llvm::Value*(llvm::Value*)> Converter) {
    return IntroduceE_(new ConvertAST(*this, gid_++, Expression, ResultingType, Converter));
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

FunctionType CodegenContext::getFunctionTy(Type ret, std::vector<Type> args) {
    return IntroduceT_(new FunctionTypeAST(*this, gid_++, ret, args));
}

ReferenceType CodegenContext::getReferenceTy(Type of) {
    return IntroduceT_(new ReferenceTypeAST(*this, gid_++, of));
}

// ==---------------------------------------------------------------

void CodegenContext::SetModuleAndFile(std::shared_ptr<llvm::Module> module, std::string infile) {
    theModule_ = module;
    filename_ = infile;
    PrepareStdFunctionPrototypes_();
}

void CodegenContext::AddError(std::string text) const{
    errors_.push_back(std::make_pair(CurrentFunc()->getName(), text));
}

bool CodegenContext::IsErrorFree(){
    return errors_.empty();
}

void CodegenContext::DisplayErrors(){
    for(const auto& e : errors_){
        std::cout << ColorStrings::Color(Color::White, true) << filename_ << ": " << ColorStrings::Color(Color::Red, true) << "ERROR" << ColorStrings::Reset();
        std::cout << " in function `" << e.first << "`: " << e.second << std::endl;
    }
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

llvm::Function* CodegenContext::GetStdFunction(std::string name) const{
    auto it = stdlib_functions_.find(name);
    if(it == stdlib_functions_.end()) return nullptr;
    else return it->second;
}

std::string CodegenContext::GetPrintFunctionName(Type type)  {
    if(type == getIntegerTy())
        return "print_int";
    if(type == getDoubleTy())
        return "print_double";
    if(type == getBooleanTy())
        return "print_bool";
    if(type == getComplexTy())
        return "print_complex";
    AddError("Asked to print non-printable value of type " + type->name());
    return "";
}

void CodegenContext::PrepareStdFunctionPrototypes_(){
    // Prepare prototypes of standard library functions.
    llvm::Function* f;
    llvm::FunctionType* ftype;
// ---
#define ADD_STDPROTO(name, typesig) do{                                 \
        ftype = TypeBuilder<typesig, false>::get(getGlobalContext());   \
        f = llvm::Function::Create(ftype, llvm::Function::ExternalLinkage, "__cco_" name, TheModule()); \
        stdlib_functions_[name] = f;                                     \
    }while(0)
// ---
    ADD_STDPROTO("print_int",void(int));
    ADD_STDPROTO("print_double",void(double));
    ADD_STDPROTO("print_bool",void(llvm::types::i<1>));
    ADD_STDPROTO("print_cstr", void(char*));
    ADD_STDPROTO("print_complex", void(double, double));

    auto complexproto = makePrototype(
        "newComplex", {{"Re", getDoubleTy()}, {"Im", getDoubleTy()}}, getComplexTy());
    makeFunction(complexproto, makeBlock({{"Cmplx", getComplexTy()}},
        {makeReturn(makeComplex(makeVariable("Re"), makeVariable("Im")))})
        );
}

}
