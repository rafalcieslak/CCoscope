
#include <typeinfo>

#include "types.h"
#include "codegencontext.h"

namespace ccoscope {

template class Proxy<TypeAST>;
template class Proxy<PrimitiveTypeAST>;
template class Proxy<VoidTypeAST>;
template class Proxy<ArithmeticTypeAST>;
template class Proxy<IntegerTypeAST>;
template class Proxy<DoubleTypeAST>;
template class Proxy<BooleanTypeAST>;
template class Proxy<FunctionTypeAST>;
template class Proxy<ReferenceTypeAST>;
/*
bool TypeCmp::operator() (const Type& lhs, const Type& rhs) const {
    return lhs->gid() < rhs->gid();
}*/
/*
template<class T>
const T* Proxy<T>::deref() const {
    if (node_ == nullptr) return nullptr;

    const T* target = node_;
    for (; target->is_proxy(); target = target->representative_->template as<T>())
        assert(target != nullptr);
    
    return target->template as<T>();
}*/

using namespace llvm;

Type str2type (CodegenContext& ctx, std::string s) {
    if (s == "int")
        return ctx.getIntegerTy();
    else if (s == "double")
        return ctx.getDoubleTy();
    else if (s == "bool")
        return ctx.getBooleanTy();
    else
        return ctx.getVoidTy();
}

bool TypeAST::equal (const TypeAST& other) const {
    return typeid(*this) == typeid(other);
}

bool FunctionTypeAST::equal (const TypeAST& other) const {
    if(size() != other.size())
        return false;
    
    if(auto otherfun = other.isa<FunctionTypeAST>()) {
        for(size_t i = 0; i < size(); i++) {
            if(operand(i) != otherfun->operands_[i])
                return false;
        }
    }
    return false;
}

bool ReferenceTypeAST::equal (const TypeAST &other) const {
    return TypeAST::equal(other) && of() == other.as<ReferenceTypeAST>()->of();
}

// ---------------------------------------------------------

llvm::Type* TypeAST::toLLVMs () const {
    return nullptr;
}

llvm::Type* VoidTypeAST::toLLVMs () const { 
    return llvm::Type::getVoidTy(getGlobalContext());
}

llvm::Type* IntegerTypeAST::toLLVMs () const {
    return llvm::Type::getInt32Ty(getGlobalContext());
}

llvm::Type* DoubleTypeAST::toLLVMs () const {
    return llvm::Type::getDoubleTy(getGlobalContext());
}

llvm::Type* BooleanTypeAST::toLLVMs () const { 
    return llvm::Type::getInt1Ty(getGlobalContext());
}

llvm::Type* FunctionTypeAST::toLLVMs () const {
    return FuntoLLVMs();
    //return nullptr; // llvm::Type* and llvm::FunctionType* incompatibility?
    /*
    std::vector<llvm::Type*> argsTypes;
    for (size_t i = 1; i < size(); i++)
        argsTypes.push_back(operands_[i]->toLLVMs());
    
    return llvm::FunctionType::get(returnType()->toLLVMs(), argsTypes, false);*/
}

llvm::FunctionType* FunctionTypeAST::FuntoLLVMs () const {
    // llvm::Type* and llvm::FunctionType* incompatibility?
    
    std::vector<llvm::Type*> argsTypes;
    for (size_t i = 1; i < size(); i++)
        argsTypes.push_back(operands_[i]->toLLVMs());
    
    return llvm::FunctionType::get(returnType()->toLLVMs(), argsTypes, false);
}

llvm::Type* ReferenceTypeAST::toLLVMs () const {
    // will that be a pointer to of->toLLVMs() ? or just the of->toLLVMs()?
    // let's assume for now it's not a pointer
    return of()->toLLVMs();
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
llvm::Value* ReferenceTypeAST::defaultLLVMsValue () const {
    return of()->defaultLLVMsValue();
}

}
