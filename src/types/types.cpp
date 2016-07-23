
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

Type str2type (CodegenContext& ctx, std::string s) {
    if (s == "int")
        return ctx.getIntegerTy();
    else if (s == "double")
        return ctx.getDoubleTy();
    else if (s == "bool")
        return ctx.getBooleanTy();
    else if (s == "complex")
        return ctx.getComplexTy();
    else if (s == "string")
        return ctx.getStringTy();
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

}
