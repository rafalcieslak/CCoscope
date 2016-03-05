#include "tree.h"
#include "../codegencontext.h"

#include "llvm/IR/Verifier.h"

#define DEBUG 0

namespace ccoscope {

Type ExprAST::maintype() const {
    return ctx().getVoidTy();
}

template<>
Type PrimitiveExprAST<int>::maintype() const {
    return ctx().getIntegerTy();
}

template<>
Type PrimitiveExprAST<bool>::maintype() const {
    return ctx().getBooleanTy();
}

template<>
Type PrimitiveExprAST<double>::maintype() const {
    return ctx().getDoubleTy();
}

Type ComplexValueAST::maintype() const {
    return ctx().getComplexTy();
}

Type VariableExprAST::maintype() const {
    return ctx().getReferenceTy( ctx().VarsInScope[Name].second );
}

Type BinaryExprAST::maintype() const {
    // If already resolved - OK. Otherwise try to resolve, if failed - return.
    // TODO: Remember that resolution failed in order no to redo it.
    if(!resolved && !Resolve()) return ctx().getVoidTy();

    return match_result.match.return_type;
}

Type AssignmentAST::maintype() const {
    // TODO -- maybe a ReferenceType ?
    return ctx().getVoidTy();
}

Type CallExprAST::maintype() const {
    // If already resolved - OK. Otherwise try to resolve, if failed - return.
    // TODO: Remember that resolution failed in order no to redo it.
    if(!resolved && !Resolve()) return ctx().getVoidTy();

    return match_result.match.return_type;
}

Type PrototypeAST::maintype() const {
    std::vector<Type> argsTypes;
    for (auto& p : Args) {
        argsTypes.push_back(p.second);
    }

    return ctx().getFunctionTy(ReturnType, argsTypes);
}

Type FunctionAST::maintype() const {
    return Proto->maintype();
}

}
