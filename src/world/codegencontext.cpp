#include "codegencontext.h"
#include "utils.h"
#include <iostream>

#include "../common/common_types.h"

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

    // This macro is useful for creating operators that use a libcco
    // function. NOTE: There are no checks whether the requested
    // stdfunction has a registered prototype, or whether the
    // arguments are of correct type. As long as the prototypes and
    // match types are correctly implemented, there will be no runtime
    // errors.
#define ADD_STD_OP(name, t1, t2, opfunc, rettype, retname)              \
    availableBinOps_[name].push_back(MatchCandidateEntry{{t1, t2}, rettype}); \
    binOpCreator_[{name, MatchCandidateEntry{{t1, t2}, rettype}}] =    \
        [this] (std::vector<Value*> v) -> Value*{                       \
        llvm::Function* f = this->GetStdFunction(opfunc);        \
        return this->Builder().CreateCall(f, v, retname);          \
    };

    ADD_STD_OP("ADD" , getComplexTy(), getComplexTy(), "complex_add" , getComplexTy(), "complexadd" );
    ADD_STD_OP("SUB" , getComplexTy(), getComplexTy(), "complex_sub" , getComplexTy(), "complexsub" );
    ADD_STD_OP("MULT", getComplexTy(), getComplexTy(), "complex_mult", getComplexTy(), "complexmult");
    ADD_STD_OP("MULT", getComplexTy(), getDoubleTy(), "complex_mult_double", getComplexTy(), "complexmult");
    ADD_STD_OP("DIV" , getComplexTy(), getComplexTy(), "complex_div" , getComplexTy(), "complexdiv" );
    ADD_STD_OP("DIV" , getComplexTy(), getDoubleTy(), "complex_div_double" , getComplexTy(), "complexdiv" );
    ADD_STD_OP("EQUAL" , getComplexTy(), getComplexTy(), "complex_equal" , getBooleanTy(), "complexeq" );

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

bool CodegenContext::IsVarInSomeEnclosingScope (std::string s) const {
    for(auto it = varsInScope_.rbegin(); it != varsInScope_.rend(); it++)
        if(it->find(s) != it->end())
            return true;
    return false;
}

std::pair<llvm::AllocaInst*, Type> CodegenContext::GetVarInfo (std::string s) {
    for(auto it = varsInScope_.rbegin(); it != varsInScope_.rend(); it++)
        if(it->find(s) != it->end())
            return (*it)[s];
    AddError("Asked for variable " + s + " that is not in scope");
    return {nullptr, getVoidTy()};
}

void CodegenContext::SetModuleAndFile(std::shared_ptr<llvm::Module> module, std::string infile) {
    theModule_ = module;
    filename_ = infile;
    PrepareStdFunctionPrototypes_();
}

void CodegenContext::AddError(std::string text, fileloc loc) const{
    errors_.push_back(std::make_tuple(currentFuncName_, text, loc));
}

bool CodegenContext::IsErrorFree(){
    return errors_.empty();
}

void CodegenContext::DisplayErrors(){
    for(const auto& e : errors_){
        std::cout << ColorStrings::Color(Color::White, true) << filename_ << ":" << std::get<2>(e) << ": " << ColorStrings::Color(Color::Red, true) << "ERROR" << ColorStrings::Reset();
        std::cout << " in function `" << std::get<0>(e) << "`: " << std::get<1>(e) << std::endl;
    }
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
    ADD_STDPROTO("print_complex", void(__cco_complex));

    ADD_STDPROTO("complex_new" , __cco_complex(double, double));
    ADD_STDPROTO("complex_add" , __cco_complex(__cco_complex,__cco_complex));
    ADD_STDPROTO("complex_sub" , __cco_complex(__cco_complex,__cco_complex));
    ADD_STDPROTO("complex_mult", __cco_complex(__cco_complex,__cco_complex));
    ADD_STDPROTO("complex_mult_double", __cco_complex(__cco_complex,double));
    ADD_STDPROTO("complex_div" , __cco_complex(__cco_complex,__cco_complex));
    ADD_STDPROTO("complex_div_double" , __cco_complex(__cco_complex,double));
    ADD_STDPROTO("complex_equal" , llvm::types::i<1>(__cco_complex,__cco_complex));
}

}
