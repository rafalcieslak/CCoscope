#include "tree.h"

#define CODEGEN_STUB(class) \
    Value* class::codegen(CodegenContext& ctx){ \
        std::cout << "Codegen for " #class " is unimplemented!" << std::endl; \
        return nullptr; \
    }

CODEGEN_STUB(NumberExprAST);
CODEGEN_STUB(VariableExprAST);
CODEGEN_STUB(BinaryExprAST);
CODEGEN_STUB(ReturnExprAST);
CODEGEN_STUB(BlockAST);
CODEGEN_STUB(AssignmentAST);
CODEGEN_STUB(CallExprAST);
CODEGEN_STUB(IfExprAST);
CODEGEN_STUB(WhileExprAST);
