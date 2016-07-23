#include <typeinfo>
#include <sstream>

#include "types.h"
#include "../world/codegencontext.h"
#include "../common/common_types.h"

namespace ccoscope {

template class Proxy<TypeAST>;
template class Proxy<PrimitiveTypeAST>;
template class Proxy<VoidTypeAST>;
template class Proxy<ArithmeticTypeAST>;
template class Proxy<IntegerTypeAST>;
template class Proxy<DoubleTypeAST>;
template class Proxy<BooleanTypeAST>;
template class Proxy<ComplexTypeAST>;
template class Proxy<FunctionTypeAST>;
template class Proxy<ReferenceTypeAST>;

using namespace llvm;

llvm::Type* TypeAST::codegen () const {
    if(!cache_) {
        cache_ = codegen_();
    }
    return cache_;
}

llvm::StructType* ComplexTypeAST::codegen () const {
    if(!cache_) {
        cache_ = codegen_();
    }
    return llvm::cast<llvm::StructType>(cache_);
}

llvm::StructType* StringTypeAST::codegen () const {
    if(!cache_) {
        cache_ = codegen_();
    }
    return llvm::cast<llvm::StructType>(cache_);
}

llvm::FunctionType* FunctionTypeAST::codegen () const {
    if(!cache_) {
        cache_ = codegen_();
    }
    return llvm::cast<llvm::FunctionType>(cache_);
}

llvm::Type* TypeAST::codegen_ () const {
    return nullptr;
}

llvm::Type* VoidTypeAST::codegen_ () const {
    return llvm::Type::getVoidTy(getGlobalContext());
}

llvm::Type* IntegerTypeAST::codegen_ () const {
    return llvm::Type::getInt32Ty(getGlobalContext());
}

llvm::Type* DoubleTypeAST::codegen_ () const {
    return llvm::Type::getDoubleTy(getGlobalContext());
}

llvm::Type* BooleanTypeAST::codegen_ () const {
    return llvm::Type::getInt1Ty(getGlobalContext());
}

llvm::StructType* ComplexTypeAST::codegen_ () const {
    return __cco_type_to_LLVM<__cco_complex>();
}

llvm::StructType* StringTypeAST::codegen_ () const {
    return __cco_type_to_LLVM<__cco_string>();
}

llvm::FunctionType* FunctionTypeAST::codegen_ () const {
    std::vector<llvm::Type*> argsTypes;
    for (size_t i = 1; i < size(); i++)
        argsTypes.push_back(operands_[i]->codegen());

    auto t = llvm::FunctionType::get(returnType()->codegen(), argsTypes, false);
    return llvm::cast<llvm::FunctionType>(t);
}

llvm::Type* ReferenceTypeAST::codegen_ () const {
    unsigned addressSpace = 0; // just a gues...
    return llvm::PointerType::get(of()->codegen(), addressSpace);
}

// ---------------------------------------------------------

llvm::Value* TypeAST::codegenDefaultValue () const {
    return llvm::UndefValue::get(codegen());
}
llvm::Value* IntegerTypeAST::codegenDefaultValue () const {
    return llvm::ConstantInt::get(getGlobalContext(), APInt(32, 0, 1));
}
llvm::Value* DoubleTypeAST::codegenDefaultValue () const {
    return llvm::ConstantFP::get(getGlobalContext(), APFloat(0.0));
}
llvm::Value* BooleanTypeAST::codegenDefaultValue () const {
    return llvm::ConstantInt::getFalse(getGlobalContext());
}
llvm::Value* ComplexTypeAST::codegenDefaultValue () const {
    auto v = dynamic_cast<llvm::Constant*>(ctx_.getDoubleTy()->codegenDefaultValue());
    std::vector<llvm::Constant*> vek{v, v};
    return llvm::ConstantStruct::get(codegen(), vek);
}
llvm::Value* StringTypeAST::codegenDefaultValue () const {
    auto ps = ctx().Builder().CreateGlobalStringPtr("");
    return llvm::ConstantStruct::get(llvm::cast<llvm::StructType>(codegen()),
        ps,
        llvm::ConstantInt::get(llvm::getGlobalContext(), llvm::APInt(64, 0, 1)), nullptr);
}
llvm::Value* ReferenceTypeAST::codegenDefaultValue () const {
    return of()->codegenDefaultValue();
}

}
