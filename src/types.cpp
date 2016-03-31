
#include <typeinfo>
#include <sstream>

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
template class Proxy<ComplexTypeAST>;
template class Proxy<FunctionTypeAST>;
template class Proxy<ReferenceTypeAST>;

using namespace llvm;

Type str2type (CodegenContext& ctx, std::string s) {
    if (s == "int")
        return ctx.getIntegerTy();
    else if (s == "double")
        return ctx.getDoubleTy();
    else if (s == "bool")
        return ctx.getBooleanTy();
    else if (s == "complex")
        return ctx.getComplexTy();
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

std::string FunctionTypeAST::name() const {
    std::ostringstream ss;
    ss << "Function(";
    if(size() > 1) {
        size_t i = 1;
        ss << argument(i)->name();
        i++;
        while(i < size()) {
            ss << ", " << argument(i)->name();
            i++;
        }
        ss << ") : " << returnType()->name();
    }
    return ss.str();
}

// ---------------------------------------------------------

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
    auto dblt = ctx_.getDoubleTy()->toLLVMs();
    auto t = llvm::StructType::create(
       getGlobalContext(),
       {dblt, dblt},
       name()
    );
    return llvm::cast<llvm::StructType>(t);
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

// ---------------------------------------------------------


std::list<Conversion> IntegerTypeAST::ListConversions() const{
    return {
        Conversion{
            ctx_.getDoubleTy(),   // Conversion to a double
            10,                   // -- costs 10
            [this](llvm::Value* v)->llvm::Value*{
                return this->ctx().Builder().CreateSIToFP(v, llvm::Type::getDoubleTy(llvm::getGlobalContext()), "convtmp");
            }
        }
    };
}

std::list<Conversion> DoubleTypeAST::ListConversions() const{
    return {
        Conversion{
            ctx_.getComplexTy(),   // Conversion to complex
            15,                   // -- costs 15
            [this](llvm::Value* v)->llvm::Value*{
                llvm::Function *CalleeF = this->ctx().TheModule()->getFunction("newComplex");
                if(CalleeF) {
                    return this->ctx().Builder().CreateCall(CalleeF, {v, this->ctx().getDoubleTy()->defaultLLVMsValue()}, "callcmplxtmp");
                } else {
                    this->ctx().AddError("newComplex constructor not found!");
                    return nullptr;
                }
            }
        }
    };
}

std::list<Conversion> ReferenceTypeAST::ListConversions() const{
    return {
        Conversion{
            of(),                // Conversion to the inner type
            1,                   // -- costs 1
            [this](llvm::Value* v)->llvm::Value*{
                AllocaInst* alloca = dynamic_cast<AllocaInst*>(v);
                if(!alloca) return nullptr;
                return this->ctx().Builder().CreateLoad(alloca, v->getName() + "_load");
            }
        }
    };
}

}
