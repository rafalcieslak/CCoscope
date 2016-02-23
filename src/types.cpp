
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

using namespace llvm;

Type str2type (const CodegenContext& ctx, std::string s) {
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

llvm::FunctionType* FunctionTypeAST::toLLVMs () const {
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

bool TypeCmp::operator () (const Type& lhs,
                           const Type& rhs) const {
    return  lhs->gid() < rhs->gid();
}

// ---------------------------------------------------------


std::list<Conversion> IntegerTypeAST::ListConversions() const{
    return {
        Conversion{
            ctx_.getDoubleTy(),   // Conversion to a double
            10,                   // -- costs 10
            [](CodegenContext & ctx, llvm::Value* v)->llvm::Value*{  // Note: The CodegenContext is passed again. We
                                                                     // cannot reuse the parent context, because we
                                                                     // need a non-const context.
                return ctx.Builder.CreateSIToFP(v, llvm::Type::getDoubleTy(llvm::getGlobalContext()), "convtmp");
            }
        }
    };
}

std::list<Conversion> ReferenceTypeAST::ListConversions() const{
    return {
        Conversion{
            of(),                // Conversion to the inner type
            1,                   // -- costs 1
            [](CodegenContext & ctx, llvm::Value* v)->llvm::Value*{
                AllocaInst* alloca = dynamic_cast<AllocaInst*>(v);
                if(!alloca) return nullptr;
                return ctx.Builder.CreateLoad(alloca, v->getName() + "_load");
            }
        }
    };
}

}
