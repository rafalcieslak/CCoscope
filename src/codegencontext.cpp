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
    , typematcher(*this)
    , gid_(0)
{
    // Operators on integers

#define INIT_OP(name) BinOpCreator[name] = std::list<MatchCandidateEntry>()

#define ADD_BASIC_OP(name, t1, t2, builderfunc, rettype, retname) \
    BinOpCreator[name].push_back(MatchCandidateEntry{{t1, t2},    \
       [this] (std::vector<Value*> v){                            \
            return this->Builder.builderfunc(v[0], v[1], retname);\
       }, rettype                                                 \
    })

    // We wouldn't need that if STL provided a `defaultdict`.
    INIT_OP("ADD");
    INIT_OP("SUB");
    INIT_OP("MULT");
    INIT_OP("DIV");
    INIT_OP("REM");
    INIT_OP("EQUAL");
    INIT_OP("NEQUAL");
    INIT_OP("GREATER");
    INIT_OP("GREATEREQ");
    INIT_OP("LESS");
    INIT_OP("LESSEQ");
    INIT_OP("LOGICAL_AND");
    INIT_OP("LOGICAL_OR");

    ADD_BASIC_OP("ADD",  getIntegerTy(), getIntegerTy(), CreateAdd,  getIntegerTy(), "addtmp");
    ADD_BASIC_OP("SUB",  getIntegerTy(), getIntegerTy(), CreateSub,  getIntegerTy(), "subtmp");
    ADD_BASIC_OP("MULT", getIntegerTy(), getIntegerTy(), CreateMul,  getIntegerTy(), "multmp");
    ADD_BASIC_OP("DIV",  getIntegerTy(), getIntegerTy(), CreateSDiv, getIntegerTy(), "divtmp");
    ADD_BASIC_OP("REM",  getIntegerTy(), getIntegerTy(), CreateSRem, getIntegerTy(), "modtmp");

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
    ADD_BASIC_OP("REM",  getDoubleTy(), getDoubleTy(), CreateFRem,  getDoubleTy(), "modtmp");

    ADD_BASIC_OP("EQUAL",    getDoubleTy(), getDoubleTy(), CreateFCmpOEQ, getBooleanTy(), "cmptmp");
    ADD_BASIC_OP("NEQUAL",   getDoubleTy(), getDoubleTy(), CreateFCmpONE, getBooleanTy(), "cmptmp");
    ADD_BASIC_OP("GREATER",  getDoubleTy(), getDoubleTy(), CreateFCmpOGT, getBooleanTy(), "cmptmp");
    ADD_BASIC_OP("GREATEREQ",getDoubleTy(), getDoubleTy(), CreateFCmpOGE, getBooleanTy(), "cmptmp");
    ADD_BASIC_OP("LESS",     getDoubleTy(), getDoubleTy(), CreateFCmpOLT, getBooleanTy(), "cmptmp");
    ADD_BASIC_OP("LESSEQ",   getDoubleTy(), getDoubleTy(), CreateFCmpOLE, getBooleanTy(), "cmptmp");

#define ADD_COMPLEX_OP(name, variadiccode, rettype, retname) \
    BinOpCreator[name].push_back(MatchCandidateEntry{{getComplexTy(), getComplexTy()},    \
       [this] (std::vector<Value*> v){                            \
            auto c1re = this->Builder.CreateExtractValue(v[0], {0}); \
            auto c1im = this->Builder.CreateExtractValue(v[0], {1}); \
            auto c2re = this->Builder.CreateExtractValue(v[1], {0}); \
            auto c2im = this->Builder.CreateExtractValue(v[1], {1}); \
            variadiccode \
            auto cmplx_t = getComplexTy()->toLLVMs(); \
            AllocaInst* alloca = CreateEntryBlockAlloca(CurrentFunc, "cmplxtmp", cmplx_t); \
            auto idx1 = this->Builder.CreateStructGEP(cmplx_t, alloca, 0); \
            this->Builder.CreateStore(reres, idx1); \
            auto idx2 = this->Builder.CreateStructGEP(cmplx_t, alloca, 1); \
            this->Builder.CreateStore(imres, idx2); \
            auto retsload = this->Builder.CreateLoad(alloca, "Cmplxloadret"); \
            return retsload; \
       }, rettype                                                 \
    })

    ADD_COMPLEX_OP("ADD",
            auto reres = this->Builder.CreateFAdd(c1re, c2re, "cmplxaddtmp");
            auto imres = this->Builder.CreateFAdd(c1im, c2im, "cmplxaddtmp");
     , getComplexTy(), "addtmp");

    ADD_COMPLEX_OP("SUB",
            auto reres = this->Builder.CreateFSub(c1re, c2re, "cmplxsubtmp");
            auto imres = this->Builder.CreateFSub(c1im, c2im, "cmplxsubtmp");
     , getComplexTy(), "subtmp");

    ADD_COMPLEX_OP("MULT",
            auto c1c2re = this->Builder.CreateFMul(c1re, c2re, "cmplxmultmp");
            auto c1c2im = this->Builder.CreateFMul(c1im, c2im, "cmplxmultmp");
            auto c1imc2re = this->Builder.CreateFMul(c1im, c2re, "cmplxmultmp");
            auto c1rec2im = this->Builder.CreateFMul(c1re, c2im, "cmplxmultmp");
            auto reres = this->Builder.CreateFSub(c1c2re, c1c2im, "cmplxsubtmp");
            auto imres = this->Builder.CreateFAdd(c1imc2re, c1rec2im, "cmplxaddtmp");
     , getComplexTy(), "multmp");

    ADD_COMPLEX_OP("DIV",
            auto c1c2re = this->Builder.CreateFMul(c1re, c2re, "cmplxmultmp");
            auto c1c2im = this->Builder.CreateFMul(c1im, c2im, "cmplxmultmp");
            auto c1imc2re = this->Builder.CreateFMul(c1im, c2re, "cmplxmultmp");
            auto c1rec2im = this->Builder.CreateFMul(c1re, c2im, "cmplxmultmp");
            auto c2rere = this->Builder.CreateFMul(c2re, c2re, "cmplxmultmp");
            auto c2imim = this->Builder.CreateFMul(c2im, c2im, "cmplxmultmp");
            auto squares = this->Builder.CreateFAdd(c2rere, c2imim, "cmplxaddtmp");
            auto left = this->Builder.CreateFAdd(c1c2re, c1c2im, "cmplxsubtmp");
            auto right = this->Builder.CreateFSub(c1imc2re, c1rec2im, "cmplxaddtmp");
            auto reres = this->Builder.CreateFDiv(left, squares, "cmplxdivtmp");
            auto imres = this->Builder.CreateFDiv(right, squares, "cmplxdivtmp");
     , getComplexTy(), "divtmp");

    BinOpCreator["EQUAL"].push_back(MatchCandidateEntry{{getComplexTy(), getComplexTy()},
       [this] (std::vector<Value*> v){
            auto c1re = this->Builder.CreateExtractValue(v[0], {0});
            auto c1im = this->Builder.CreateExtractValue(v[0], {1});
            auto c2re = this->Builder.CreateExtractValue(v[1], {0});
            auto c2im = this->Builder.CreateExtractValue(v[1], {1});
            auto c1c2re = this->Builder.CreateFCmpOEQ(c1re, c2re, "cmplxcmptmp");
            auto c1c2im = this->Builder.CreateFCmpOEQ(c1im, c2im, "cmplxcmptmp");
            return this->Builder.CreateAnd(c1c2re, c1c2im, "cmplxcmptmp");
       }, getBooleanTy()
    });
}

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

ComplexValue CodegenContext::makeComplex(Expr re, Expr im) {
    return introduceE(new ComplexValueAST(*this, gid_++, re, im));
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

VoidType CodegenContext::getVoidTy() const{
    return introduceT(new VoidTypeAST(*this, gid_++));
}

IntegerType CodegenContext::getIntegerTy() const{
    return introduceT(new IntegerTypeAST(*this, gid_++));
}

DoubleType CodegenContext::getDoubleTy() const{
    return introduceT(new DoubleTypeAST(*this, gid_++));
}

BooleanType CodegenContext::getBooleanTy() const{
    return introduceT(new BooleanTypeAST(*this, gid_++));
}

ComplexType CodegenContext::getComplexTy() const{
    return introduceT(new ComplexTypeAST(*this, gid_++));
}

FunctionType CodegenContext::getFunctionTy(Type ret, std::vector<Type> args) const{
    return introduceT(new FunctionTypeAST(*this, gid_++, ret, args));
}

ReferenceType CodegenContext::getReferenceTy(Type of) const{
    return introduceT(new ReferenceTypeAST(*this, gid_++, of));
}

// ==---------------------------------------------------------------

void CodegenContext::SetModuleAndFile(std::shared_ptr<llvm::Module> module, std::string infile) {
    TheModule = module;
    filename = infile;
    PrepareStdFunctionPrototypes();
}

void CodegenContext::AddError(std::string text) const{
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

const ExprAST* CodegenContext::introduce_expr(const ExprAST* node) const{
    /* This look ridiculously simple now, but in the future we can
     * make CSE optimization here
     */
    expressions.insert(node);
    return node;
}

const TypeAST* CodegenContext::introduce_type(const TypeAST* node) const{
    auto it = types.find(node);
    if(it != types.end() && *it != node) {
        delete node;
        return *it;
    }

    types.insert(node);
    return node;
}

const PrototypeAST* CodegenContext::introduce_prototype(const PrototypeAST* node) const{
    auto pit = prototypesMap.find(node->getName());
    if(pit != prototypesMap.end()) {
        delete node;
        return *(pit->second);
    }

    prototypes.insert(node);
    prototypesMap[node->getName()] = node;
    return node;
}

const FunctionAST* CodegenContext::introduce_function(const FunctionAST* node) const{
    definitions.insert(node);
    return node;
}

llvm::Function* CodegenContext::GetStdFunction(std::string name) const{
    auto it = stdlib_functions.find(name);
    if(it == stdlib_functions.end()) return nullptr;
    else return it->second;
}

void CodegenContext::PrepareStdFunctionPrototypes(){
    // Prepare prototypes of standard library functions.
    llvm::Function* f;
    llvm::FunctionType* ftype;
// ---
#define ADD_STDPROTO(name, typesig) do{                                 \
        ftype = TypeBuilder<typesig, false>::get(getGlobalContext());   \
        f = llvm::Function::Create(ftype, llvm::Function::ExternalLinkage, "__cco_" name, TheModule.get()); \
        stdlib_functions[name] = f;                                     \
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
