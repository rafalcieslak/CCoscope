#include "tree.h"

#define CODEGEN_STUB(class) \
    Value* class::codegen(CodegenContext& ctx) const { \
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


Function* PrototypeAST::codegen(CodegenContext& ctx) const {
  // Make the function type:  double(double,double) etc.

    // TODO: Respect argument types.
  std::vector<Type *> Ints(Args.size(),
                              Type::getInt32Ty(getGlobalContext()));
  FunctionType *FT =
      FunctionType::get(Type::getInt32Ty(getGlobalContext()), Ints, false);

  Function *F =
      Function::Create(FT, Function::ExternalLinkage, Name, ctx.TheModule.get());

  // Set names for all arguments.
  unsigned Idx = 0;
  for (auto &Arg : F->args())
    Arg.setName(Args[Idx++].first);

  return F;
}
