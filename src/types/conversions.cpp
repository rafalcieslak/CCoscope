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
                llvm::Function *CalleeF = this->ctx().GetStdFunction("complex_new");
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
