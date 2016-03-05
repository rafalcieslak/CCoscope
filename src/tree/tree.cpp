#include "tree.h"
#include "../codegencontext.h"

#include "llvm/IR/Verifier.h"

#define DEBUG 0

namespace ccoscope {

template class Proxy<ExprAST>;
template class Proxy<PrimitiveExprAST<int>>;
template class Proxy<PrimitiveExprAST<double>>;
template class Proxy<PrimitiveExprAST<bool>>;
template class Proxy<ComplexValueAST>;
template class Proxy<VariableExprAST>;
template class Proxy<BinaryExprAST>;
template class Proxy<ReturnExprAST>;
template class Proxy<BlockAST>;
template class Proxy<AssignmentAST>;
template class Proxy<CallExprAST>;
template class Proxy<IfExprAST>;
template class Proxy<WhileExprAST>;
template class Proxy<ForExprAST>;
template class Proxy<KeywordAST>;
template class Proxy<PrototypeAST>;
template class Proxy<FunctionAST>;

bool ExprAST::equal(const ExprAST& other) const {
    return gid() == other.gid();
}

}
