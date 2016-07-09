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

llvm::Type* TypeAST::toLLVMs () const {
    if(!cache_) {
        cache_ = toLLVMs_();
    }
    return cache_;
}

llvm::StructType* ComplexTypeAST::toLLVMs () const {
    if(!cache_) {
        cache_ = toLLVMs_();
    }
    return llvm::cast<llvm::StructType>(cache_);
}

llvm::FunctionType* FunctionTypeAST::toLLVMs () const {
    if(!cache_) {
        cache_ = toLLVMs_();
    }
    return llvm::cast<llvm::FunctionType>(cache_);
}

llvm::Type* TypeAST::toLLVMs_ () const {
    return nullptr;
}

llvm::Type* VoidTypeAST::toLLVMs_ () const {
    return llvm::Type::getVoidTy(getGlobalContext());
}

llvm::Type* IntegerTypeAST::toLLVMs_ () const {
    return llvm::Type::getInt32Ty(getGlobalContext());
}

llvm::Type* DoubleTypeAST::toLLVMs_ () const {
    return llvm::Type::getDoubleTy(getGlobalContext());
}

llvm::Type* BooleanTypeAST::toLLVMs_ () const {
    return llvm::Type::getInt1Ty(getGlobalContext());
}

llvm::StructType* ComplexTypeAST::toLLVMs_ () const {
    return __cco_type_to_LLVM<__cco_complex>();
}

llvm::FunctionType* FunctionTypeAST::toLLVMs_ () const {
    std::vector<llvm::Type*> argsTypes;
    for (size_t i = 1; i < size(); i++)
        argsTypes.push_back(operands_[i]->toLLVMs());

    auto t = llvm::FunctionType::get(returnType()->toLLVMs(), argsTypes, false);
    return llvm::cast<llvm::FunctionType>(t);
}

llvm::Type* ReferenceTypeAST::toLLVMs_ () const {
    unsigned addressSpace = 0; // just a gues...
    return llvm::PointerType::get(of()->toLLVMs(), addressSpace);
}

// ---------------------------------------------------------

llvm::Value* TypeAST::defaultLLVMsValue () const {
    return llvm::UndefValue::get(toLLVMs());
}
llvm::Value* IntegerTypeAST::defaultLLVMsValue () const {
    return llvm::ConstantInt::get(getGlobalContext(), APInt(32, 0, 1));
}
llvm::Value* DoubleTypeAST::defaultLLVMsValue () const {
    return llvm::ConstantFP::get(getGlobalContext(), APFloat(0.0));
}
llvm::Value* BooleanTypeAST::defaultLLVMsValue () const {
    return llvm::ConstantInt::getFalse(getGlobalContext());
}
llvm::Value* ComplexTypeAST::defaultLLVMsValue () const {
    auto v = dynamic_cast<llvm::Constant*>(ctx_.getDoubleTy()->defaultLLVMsValue());
    std::vector<llvm::Constant*> vek{v, v};
    return llvm::ConstantStruct::get(toLLVMs(), vek);
}
llvm::Value* ReferenceTypeAST::defaultLLVMsValue () const {
    return of()->defaultLLVMsValue();
}

}
