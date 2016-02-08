#include "tree.h"

#include "llvm/IR/Verifier.h"

#define NDEBUG
//#undef NDEBUG

Type* datatype2llvmType(datatype d) {
    switch(d) {
        case DATATYPE_int:   return Type::getInt32Ty(getGlobalContext());
        case DATATYPE_bool:  return Type::getInt1Ty(getGlobalContext());
        case DATATYPE_float: return Type::getFloatTy(getGlobalContext());
        case DATATYPE_void:  return Type::getVoidTy(getGlobalContext());
    }
    return nullptr;
}

Value* createDefaultValue(datatype d) {
    switch(d) {
        case DATATYPE_int:   return ConstantInt::get(getGlobalContext(), APInt(32, 0, 1));
        case DATATYPE_float: return ConstantFP::get(Type::getFloatTy(getGlobalContext()), 0.0);
        case DATATYPE_bool:  return ConstantInt::getFalse(getGlobalContext());
        default: return nullptr;
    }
}

/// CreateEntryBlockAlloca - Create an alloca instruction in the entry block of
/// the function.  This is used for mutable variables etc.
static AllocaInst *CreateEntryBlockAlloca(Function *TheFunction,
                                          const std::string &VarName,
                                          Type* type) {
  IRBuilder<> TmpB(&TheFunction->getEntryBlock(),
                 TheFunction->getEntryBlock().begin());
  return TmpB.CreateAlloca(type, 0, (VarName + "_addr").c_str());
}

// Creates a global i8 string. Useful for printing values.
Constant* CreateI8String(Module* M, char const* str, Twine const& name, CodegenContext& ctx) {
  auto strVal = ctx.Builder.CreateGlobalStringPtr(str);
  return cast<Constant>(strVal);
}

datatype stodatatype (std::string s)
{
    if (s == "int")
        return datatype::DATATYPE_int;
    else if (s == "double")
        return datatype::DATATYPE_float;
    else if (s == "bool")
        return datatype::DATATYPE_bool;
    else
        return datatype::DATATYPE_void;
}

// ---------------------------------------------------------------------
template<>
Value* PrimitiveExprAST<int>::codegen(CodegenContext& ctx) const {
    return ConstantInt::get(getGlobalContext(), APInt(32, Val, 1));
}

template<>
Value* PrimitiveExprAST<bool>::codegen(CodegenContext& ctx) const {
    if(Val == true)
        return ConstantInt::getTrue(getGlobalContext());
    else
        return ConstantInt::getFalse(getGlobalContext());
}

template<>
Value* PrimitiveExprAST<float>::codegen(CodegenContext& ctx) const {
    return ConstantFP::get(Type::getFloatTy(getGlobalContext()), Val);
}

template<typename T>
Value* PrimitiveExprAST<T>::codegen(CodegenContext& ctx) const {
    return nullptr;
}

Value* VariableExprAST::codegen(CodegenContext& ctx) const {
    // Assuming that everything is an int.
     // --> where does this assumption appear?
    if(ctx.VarsInScope.count(Name) < 1){
        std::cout << "Variable '" << Name << "' is not available in this scope." << std::endl;
        return nullptr;
    }
    AllocaInst* alloca = ctx.VarsInScope[Name].first;
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
    // <-----
    //       I think we can reuse the types that LLVM gives to us
    //       and this approach is applied in current implementation
    // TODO: implicit conversions and a cost function related to it

    auto fitit = ctx.BinOpCreator.end();
    if (valL->getType()->isIntegerTy() && valR->getType()->isIntegerTy())
        fitit = ctx.BinOpCreator.find(std::make_tuple(Opcode, DATATYPE_int, DATATYPE_int));
    else if (valL->getType()->isFloatTy() && valR->getType()->isFloatTy())
        fitit = ctx.BinOpCreator.find(std::make_tuple(Opcode, DATATYPE_float, DATATYPE_float));
    
    if(fitit != ctx.BinOpCreator.end())
        return (fitit->second)(valL, valR);
    else {
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
        ScopeManager SM {this, ctx};
        
        // TODO: make ScopeManager sensitive to how many variables were
        // successfully initialized
        for(auto& var : Vars){
            if(ctx.VarsInScope.count(var.first) > 0){
                std::cout << "Variable shadowing is not allowed" << std::endl;
                return nullptr;
            }
            AllocaInst* Alloca = CreateEntryBlockAlloca(parent, var.first, datatype2llvmType(var.second));
            // Initialize the var to 0.
            Value* zero = createDefaultValue(var.second);
#ifndef NDEBUG
            std::cerr << "created zero value ";
            zero->dump();
            std::cerr << std::endl;
#endif
              //ConstantInt::get(getGlobalContext(), APInt(32, 0, 1));
            ctx.Builder.CreateStore(zero, Alloca);
            ctx.VarsInScope[var.first] = std::make_pair(Alloca, var.second);
        }

        // Generate statements inside the block
        Value* last = nullptr;
        for(const auto& stat : Statements){
            last = stat->codegen(ctx);
            if(!last) return nullptr;
        }
        /* Not necessary now - ScopeManager will clean up
        // Remove stack vars
        for(auto& var : Vars){
            auto it = ctx.VarsInScope.find(var.first);
            ctx.VarsInScope.erase(it);
        }
        */
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
    AllocaInst* alloca = ctx.VarsInScope[Name].first;
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
        std::string formatSpecifier;
        switch(Args[0]->maintype(ctx).second) {
            case DATATYPE_int: formatSpecifier = "%d"; break;
            case DATATYPE_float: formatSpecifier = "%f"; break;
            case DATATYPE_bool: formatSpecifier = "%d"; break;
            default: formatSpecifier = "%d";
        }
        
        ArgsV.push_back( CreateI8String(ctx.TheModule.get(), (formatSpecifier + "\n").c_str(), "printf_number", ctx) );
#ifndef NDEBUG
        std::cerr << "in call will codegen arg" << std::endl;
#endif
        auto temp = Args[0]->codegen(ctx);
       // auto vare = dynamic_cast<VariableExprAST*>(Args[0]);
      //  if(vare != nullptr) {
      //      std::cerr << "codegening var 
      //  }
#ifndef NDEBUG
        std::cerr << "in call after codegen arg" << std::endl;
#endif
        ArgsV.push_back( temp ); //Args[0]->codegen(ctx) );
#ifndef NDEBUG
        std::cerr << "printujemy: " << formatSpecifier << " : ";
        temp->dump();
        std::cerr << std::endl;
#endif
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

    // Update of codegening context -- we are in a loop from now on
    ctx.LoopsBBHeaderPost.push_back({HeaderBB, PostBB});
    
    // BODY
    parent->getBasicBlockList().push_back(BodyBB);
    ctx.Builder.SetInsertPoint(BodyBB);
    Value* BodyV = Body->codegen(ctx);
    if(!BodyV) return nullptr;
    ctx.Builder.CreateBr(HeaderBB);


    // POSTWHILE
    parent->getBasicBlockList().push_back(PostBB);
    ctx.Builder.SetInsertPoint(PostBB);
    
    // Update of codegening context -- we've just got out of the loop
    ctx.LoopsBBHeaderPost.pop_back();

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

Value* KeywordAST::codegen(CodegenContext& ctx) const {
    Function* parent = ctx.CurrentFunc;
    switch(which) {
        case keyword::Break:
            if (!ctx.is_inside_loop()) {
                // TODO: inform the user at which line (and column)
                // they wrote `break;` outside any loop
                std::cout << "'break' keyword outside any loop\n";
                return nullptr;
            } else {
                  auto postBB = ctx.LoopsBBHeaderPost.back().second;
                  
                  // A bit of a hack here -- we generate a non-reachable basic block
                  // because it turns out to be a lot easier then fighting with
                  // branch instructions generated by `If` or `While` statements
                  // that may occur immediately below the branch from 
                  // `break` or `continue` keyword
                  BasicBlock* discardBB   = BasicBlock::Create(getGlobalContext(), "breakDiscard");
                  parent->getBasicBlockList().push_back(discardBB);
                  ctx.Builder.CreateCondBr(ConstantInt::getTrue(getGlobalContext()), postBB, discardBB);
                  ctx.Builder.SetInsertPoint(discardBB);
                  return ConstantInt::get(getGlobalContext(), APInt(32, 0, 1));
            }
            break;
        case keyword::Continue:
            if (!ctx.is_inside_loop()) {
                // TODO: inform the user at which line (and column)
                // they wrote `break;` outside any loop
                std::cout << "'continue' keyword outside any loop\n";
                return nullptr;
            } else {
                auto headerBB = ctx.LoopsBBHeaderPost.back().first;
                BasicBlock* discardBB   = BasicBlock::Create(getGlobalContext(), "continueDiscard");
                parent->getBasicBlockList().push_back(discardBB);
                ctx.Builder.CreateCondBr(ConstantInt::getTrue(getGlobalContext()), headerBB, discardBB);
                ctx.Builder.SetInsertPoint(discardBB);
                return ConstantInt::get(getGlobalContext(), APInt(32, 0, 1));
            }
            break;
    }
    return nullptr;
}

//------------------------------------------


Function* PrototypeAST::codegen(CodegenContext& ctx) const {
  // Make the function type:  double(double,double) etc.

    // TODO: Respect argument types.
  /*std::vector<Type *> Ints(Args.size(),
                              Type::getInt32Ty(getGlobalContext()));
  */
  std::vector<Type*> argsTypes;
  for (auto& p : Args) {
      argsTypes.push_back(datatype2llvmType(p.second));
  }  
  
  FunctionType *FT =
      FunctionType::get(datatype2llvmType(ReturnType), argsTypes, false);

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

  size_t i = 0;
  // Record the function arguments in the VarsInScope map.
  for (auto &Arg : TheFunction->args()){
      AllocaInst* Alloca = CreateEntryBlockAlloca(TheFunction, Arg.getName(), Arg.getType());
      ctx.Builder.CreateStore(&Arg,Alloca);
      ctx.VarsInScope[Arg.getName()] = std::make_pair(Alloca, (Proto->getArgs())[i].second);
      i++;
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

// --------------------------------------------------
// Typechecking
// --------------------------------------------------

ExprType ExprAST::maintype(CodegenContext& ctx) const {
    return {std::make_shared<PrimitiveExprAST<int>>(42), DATATYPE_void};//CCVoidType()};
}
/* see `tree.h`
template<typename T>
ExprType PrimitiveExprAST<T>::maintype(CodegenContext& ctx) const {
    return {std::make_shared<PrimitiveExprAST<int>>(42), CCVoidType()};
}
*/
template<>
ExprType PrimitiveExprAST<int>::maintype(CodegenContext& ctx) const {
    return {std::make_shared<PrimitiveExprAST<int>>(42), DATATYPE_int};
}

template<>
ExprType PrimitiveExprAST<bool>::maintype(CodegenContext& ctx) const {
    return {std::make_shared<PrimitiveExprAST<int>>(42), DATATYPE_bool};
}

template<>
ExprType PrimitiveExprAST<float>::maintype(CodegenContext& ctx) const {
    return {std::make_shared<PrimitiveExprAST<int>>(42), DATATYPE_float};
}

ExprType VariableExprAST::maintype(CodegenContext& ctx) const {
    return {std::make_shared<PrimitiveExprAST<int>>(42), ctx.VarsInScope[Name].second};//CCVoidType()};
}

ExprType BinaryExprAST::maintype(CodegenContext& ctx) const {
    return {std::make_shared<PrimitiveExprAST<int>>(42), DATATYPE_void};//CCVoidType()};
}

ExprType AssignmentAST::maintype(CodegenContext& ctx) const {
    return {std::make_shared<PrimitiveExprAST<int>>(42), DATATYPE_void};//CCVoidType()};
}

ExprType CallExprAST::maintype(CodegenContext& ctx) const {
    return {std::make_shared<PrimitiveExprAST<int>>(42), DATATYPE_void};//CCVoidType()};
}

ExprType PrototypeAST::maintype(CodegenContext& ctx) const {
    return {std::make_shared<PrimitiveExprAST<int>>(42), DATATYPE_void};//CCVoidType()};
}

ExprType FunctionAST::maintype(CodegenContext& ctx) const {
    return {std::make_shared<PrimitiveExprAST<int>>(42), DATATYPE_void};//CCVoidType()};
}
