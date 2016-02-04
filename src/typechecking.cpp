
#include "typechecking.h"

std::pair<CCType, ExprAST> maintype(std::shared_ptr<ExprAST> tree) {
    return {CCVoidType(), tree};
}
