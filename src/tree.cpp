#include "tree.h"

#include "llvm/IR/Verifier.h"

/// CreateEntryBlockAlloca - Create an alloca instruction in the entry block of
/// the function.  This is used for mutable variables etc.
static AllocaInst *CreateEntryBlockAlloca(Function *TheFunction,
                                          const std::string &VarName) {
  IRBuilder<> TmpB(&TheFunction->getEntryBlock(),
                 TheFunction->getEntryBlock().begin());
  return TmpB.CreateAlloca(Type::getInt32Ty(getGlobalContext()), 0,
                           (VarName + "_addr").c_str());
}

// Creates a global i8 string. Useful for printing values.
Constant* CreateI8String(Module* M, char const* str, Twine const& name) {
  Constant* strConstant = ConstantDataArray::getString(getGlobalContext(), str);
  GlobalVariable* GVStr =
      new GlobalVariable(*M, strConstant->getType(), true,
                         GlobalValue::InternalLinkage, strConstant, name);
  Constant* zero = Constant::getNullValue(IntegerType::getInt32Ty(getGlobalContext()));
  Constant* indices[] = {zero, zero};
  
  
  Type* I8P = Type::getInt8PtrTy(getGlobalContext());
  
  
  Constant* strVal = ConstantExpr::getGetElementPtr(I8P, GVStr, indices);
  return strVal;
}

// ---------------------------------------------------------------------

Value* NumberExprAST::codegen(CodegenContext& ctx) const {
    // Again, assuming that everything is an int.
    return ConstantInt::get(getGlobalContext(), APInt(32, Val, 1));;
}

Value* VariableExprAST::codegen(CodegenContext& ctx) const {
    // Assuming that everything is an int.
    if(ctx.VarsInScope.count(Name) < 1){
        std::cout << "Variable '" << Name << "' is not available in this scope." << std::endl;
        return nullptr;
    }
    AllocaInst* alloca = ctx.VarsInScope[Name];
    Value* V = ctx.Builder.CreateLoad(alloca, Name.c_str());
    return V;
}

Value* BinaryExprAST::codegen(CodegenContext& ctx) const {
    Value* valL = LHS->codegen(ctx);
    Value* valR = RHS->codegen(ctx);
    if(!valL || !valR) return nullptr;

    // TODO: Operator overloading is necessary in order to respect types!
    // I believe it would make sense if we simply returned, from every codegen, a pair: datatype, value.
    // Then we might lookup viable operators in a global map   (name, datatype1, datatype2) -> codegen function ptr
    // and use that mapped function to generate code. As implicit conversions are possible, we would lookup
    // various combinations types, which would yield a set of possible operators. Or we could order them by their
    // conversion cost and lookup them one by one.

    // But temporarily I assume everything is an int.
    if(Opcode == "ADD"){
        return ctx.Builder.CreateAdd(valL, valR, "addtmp");
    }else if(Opcode == "SUB"){
        return ctx.Builder.CreateSub(valL, valR, "subtmp");
    }else if(Opcode == "MULT"){
        return ctx.Builder.CreateMul(valL, valR, "multmp");
    }else if(Opcode == "DIV"){
        return ctx.Builder.CreateSDiv(valL, valR, "divtmp");
    }else if(Opcode == "MOD"){
        return ctx.Builder.CreateSRem(valL, valR, "modtmp");

    }else if(Opcode == "EQUAL"){
        return ctx.Builder.CreateICmpEQ(valL, valR, "cmptmp");
    }else if(Opcode == "NEQUAL"){
        return ctx.Builder.CreateICmpNE(valL, valR, "cmptmp");

    }else if(Opcode == "GREATER"){
        return ctx.Builder.CreateICmpUGT(valL, valR, "cmptmp");
    }else if(Opcode == "GREATEREQ"){
        return ctx.Builder.CreateICmpUGE(valL, valR, "cmptmp");

    }else if(Opcode == "LESS"){
        return ctx.Builder.CreateICmpULT(valL, valR, "cmptmp");
    }else if(Opcode == "LESSEQ"){
        return ctx.Builder.CreateICmpULE(valL, valR, "cmptmp");

    }else if(Opcode == "LOGICAL_AND"){
        // These are currently implemented as arythmetical ANDs. This
        // will yield weird results when trying to (5 && 7). Maybe a
        // good solution would be to convert these into u1, then do
        // AND on these u1s, and then convert the result back to u32.
        return ctx.Builder.CreateAnd(valL, valR, "andtmp");
    }else if(Opcode == "LOGICAL_OR"){
        return ctx.Builder.CreateOr(valL, valR, "ortmp");

    }else{
        std::cout << "Operator '" << Opcode << "' codegen is not implemented!" << std::endl;
        return nullptr;
    }
}

Value* ReturnExprAST::codegen(CodegenContext& ctx) const {
    Value* val = Expr->codegen(ctx);
    if(!val) return nullptr;
    ctx.Builder.CreateRet(val);
    return val;
}

Value* BlockAST::codegen(CodegenContext& ctx) const {
    if(Statements.size() == 0){
        // Aah, an empty block. That seems like a special case,
        // because this results in no Value, and there would be
        // nothing we can give to our caller. So let's just create a 0
        // value and return it.
        return ConstantInt::get(getGlobalContext(), APInt(32, 0, 1));
    }else{
        // Create new stack vars.
        Function* parent = ctx.CurrentFunc;
        for(auto& var : Vars){
            if(ctx.VarsInScope.count(var.first) > 0){
                std::cout << "Variable shadowing is not allowed" << std::endl;
                return nullptr;
            }
            AllocaInst* Alloca = CreateEntryBlockAlloca(parent, var.first);
            // Initialize the var to 0.
            Value* zero = ConstantInt::get(getGlobalContext(), APInt(32, 0, 1));;
            ctx.Builder.CreateStore(zero,Alloca);
            ctx.VarsInScope[var.first] = Alloca;
        }

        // Generate statements inside the block
        Value* last = nullptr;
        for(const auto& stat : Statements){
            last = stat->codegen(ctx);
            if(!last) return nullptr;
        }

        // Remove stack vars
        for(auto& var : Vars){
            auto it = ctx.VarsInScope.find(var.first);
            ctx.VarsInScope.erase(it);
        }

        return last;
    }
}

Value* AssignmentAST::codegen(CodegenContext& ctx) const {
    Value* Val = Expr->codegen(ctx);
    if(!Val) return nullptr;

    // Look up the target var name.
    if (ctx.VarsInScope.count(Name) < 1){
        std::cout << "Variable '" << Name << "' is not available in this scope." << std::endl;
        return nullptr;
    }
    AllocaInst* alloca = ctx.VarsInScope[Name];
    ctx.Builder.CreateStore(Val, alloca);
    return Val;
}

Value* CallExprAST::codegen(CodegenContext& ctx) const {
    // Special case for print
    if(Callee == "print"){
        // Translate the call into a call to cstdlibs' printf.
        if(Args.size() != 1){
            std::cout << "Function print takes 1 argument, " << Args.size() << " given." << std::endl;
            return nullptr;
        }
        std::vector<Value*> ArgsV;
        ArgsV.push_back( CreateI8String(ctx.TheModule.get(), "%d\n", "printf_number") );
        ArgsV.push_back( Args[0]->codegen(ctx) );
        return ctx.Builder.CreateCall(ctx.func_printf, ArgsV, "calltmp");
    }

    // Look up the name in the global module table.
    Function *CalleeF = ctx.TheModule->getFunction(Callee);
    if (!CalleeF){
        std::cout << "Function " << Callee << " was not declared" << std::endl;
        return nullptr;
    }

    // If argument mismatch error.
    if (CalleeF->arg_size() != Args.size()){
        std::cout << "Function " << Callee << " takes " << CalleeF->arg_size() << " arguments, " << Args.size() << " given." << std::endl;
        return nullptr;
    }

    std::vector<Value *> ArgsV;
    for (unsigned i = 0, e = Args.size(); i != e; ++i) {
        ArgsV.push_back(Args[i]->codegen(ctx));
        if (!ArgsV.back())
            return nullptr;
    }

  return ctx.Builder.CreateCall(CalleeF, ArgsV, "calltmp");
}

Value* IfExprAST::codegen(CodegenContext& ctx) const {
    Value* cond = Cond->codegen(ctx);
    if(!cond) return nullptr;

    Value* cmp = ctx.Builder.CreateICmpNE(cond, ConstantInt::get(getGlobalContext(), APInt(1, 0, 1)), "ifcond");

    Function* parent = ctx.CurrentFunc;

    BasicBlock *ThenBB  = BasicBlock::Create(getGlobalContext(), "then");
    BasicBlock *MergeBB = BasicBlock::Create(getGlobalContext(), "ifcont");

    BasicBlock *ElseBB;
    if(Else){
        // There is an else-block for this if. Create BB for else.
        ElseBB  = BasicBlock::Create(getGlobalContext(), "else");
    }else{
        // There is no else-block for this if. Do not create a block. Make sure that failing the condition will jump to merge.
        ElseBB = MergeBB;
    }

    ctx.Builder.CreateCondBr(cmp, ThenBB, ElseBB);

    // THEN
    // Add to parent
    parent->getBasicBlockList().push_back(ThenBB);
    // Codegen recursivelly
    ctx.Builder.SetInsertPoint(ThenBB);
    Value *ThenV = Then->codegen(ctx);
    if (!ThenV) return nullptr;
    ctx.Builder.CreateBr(MergeBB);
    // Codegen of 'Then' can change the current block, update ThenBB.
    ThenBB = ctx.Builder.GetInsertBlock();

    if(Else){
        // ELSE
        // Add to parent
        parent->getBasicBlockList().push_back(ElseBB);
        // Codegen recursivelly
        ctx.Builder.SetInsertPoint(ElseBB);
        Value *ElseV = Else->codegen(ctx);
        if (!ElseV) return nullptr;
        ctx.Builder.CreateBr(MergeBB);
        // Codegen of 'Else' can change the current block, update ElseBB.
        ElseBB = ctx.Builder.GetInsertBlock();
    }

    // Further instructions are to be placed at merge block
    parent->getBasicBlockList().push_back(MergeBB);
    ctx.Builder.SetInsertPoint(MergeBB);

    return ConstantInt::get(getGlobalContext(), APInt(32, 0, 1));
}

Value* WhileExprAST::codegen(CodegenContext& ctx) const {

    Function* parent = ctx.CurrentFunc;

    BasicBlock* HeaderBB = BasicBlock::Create(getGlobalContext(), "header");
    BasicBlock* BodyBB   = BasicBlock::Create(getGlobalContext(), "body");
    BasicBlock* PostBB   = BasicBlock::Create(getGlobalContext(), "postwhile");

    ctx.Builder.CreateBr(HeaderBB);

    // HEADER
    parent->getBasicBlockList().push_back(HeaderBB);
    ctx.Builder.SetInsertPoint(HeaderBB);
    Value* cond = Cond->codegen(ctx);
    if(!cond) return nullptr;
    Value* cmp = ctx.Builder.CreateICmpNE(cond, ConstantInt::get(getGlobalContext(), APInt(1, 0, 1)), "whilecond");
    ctx.Builder.CreateCondBr(cmp, BodyBB, PostBB);

    // BODY
    parent->getBasicBlockList().push_back(BodyBB);
    ctx.Builder.SetInsertPoint(BodyBB);
    Value* BodyV = Body->codegen(ctx);
    if(!BodyV) return nullptr;
    ctx.Builder.CreateBr(HeaderBB);

    // POSTWHILE
    parent->getBasicBlockList().push_back(PostBB);
    ctx.Builder.SetInsertPoint(PostBB);

    return ConstantInt::get(getGlobalContext(), APInt(32, 0, 1));
}

Value* ForExprAST::codegen(CodegenContext& ctx) const {
    /* Transform
     * `for(Init | Cond | Step) { Body }`
     * into
     * `{
     *     Init;
     *     while(Cond) {
     *         Body;
     *         Step;
     *     }
     * }`
     */
    auto body = std::static_pointer_cast<BlockAST>(Body);
    auto innerVars = body->Vars;
    auto innerStatements = body->Statements;
    innerStatements.insert(innerStatements.end(), Step.begin(), Step.end());
    auto whileAST = std::make_shared<WhileExprAST>(Cond, 
      std::make_shared<BlockAST>(innerVars, innerStatements));
    
    auto init = std::static_pointer_cast<BlockAST>(Init);
    auto outerStatements = init->Statements;
    outerStatements.push_back(whileAST);
    
    auto block = std::make_shared<BlockAST>(init->Vars, outerStatements);
    
    return block->codegen(ctx);
}

//------------------------------------------


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


Function *FunctionAST::codegen(CodegenContext& ctx) const {
  // First, check for an existing function from a previous 'extern' declaration.
  Function *TheFunction = ctx.TheModule->getFunction(Proto->getName());

  // The function was not previously declared with an extern, so we
  // need to emit the prototype declaration.
  if (!TheFunction){
    TheFunction = Proto->codegen(ctx);
  }

  // Set current function
  ctx.CurrentFunc = TheFunction;

  // Create a new basic block to start insertion into.
  BasicBlock *BB = BasicBlock::Create(getGlobalContext(), "entry", TheFunction);
  ctx.Builder.SetInsertPoint(BB);

  // Clear the scope.
  ctx.VarsInScope.clear();


  // Record the function arguments in the VarsInScope map.
  for (auto &Arg : TheFunction->args()){
      AllocaInst* Alloca = CreateEntryBlockAlloca(TheFunction, Arg.getName());
      ctx.Builder.CreateStore(&Arg,Alloca);
      ctx.VarsInScope[Arg.getName()] = Alloca;
  }


  // Insert function body into the function insertion point.
  Value* val = Body->codegen(ctx);

  // Before terminating the function, create a default return value, in case the function body does not contain one.
  // TODO: Default return type.
  ctx.Builder.CreateRet( ConstantInt::get(getGlobalContext(), APInt(32, 0, 1)) );

  if(val){
    // Validate the generated code, checking for consistency.
    verifyFunction(*TheFunction);

    return TheFunction;
  }

  // Codegenning body returned nullptr, so an error was encountered. Remove the function.
  TheFunction->eraseFromParent();
  return nullptr;
}
