#include "tree.h"

#include "llvm/IR/Verifier.h"

#define NDEBUG
//#undef NDEBUG
namespace ccoscope {

template class Proxy<ExprAST>;
template class Proxy<PrimitiveExprAST<int>>;
template class Proxy<PrimitiveExprAST<double>>;
template class Proxy<PrimitiveExprAST<bool>>;
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

/// CreateEntryBlockAlloca - Create an alloca instruction in the entry block of
/// the function.  This is used for mutable variables etc.
static llvm::AllocaInst *CreateEntryBlockAlloca(llvm::Function *TheFunction,
                                          const std::string &VarName,
                                          llvm::Type* type) {
  llvm::IRBuilder<> TmpB(&TheFunction->getEntryBlock(),
                 TheFunction->getEntryBlock().begin());
  return TmpB.CreateAlloca(type, 0, (VarName + "_addr").c_str());
}

// Creates a global i8 string. Useful for printing values.
llvm::Constant* CreateI8String(llvm::Module* M, char const* str, llvm::Twine const& name, CodegenContext& ctx) {
  auto strVal = ctx.Builder.CreateGlobalStringPtr(str);
  return llvm::cast<Constant>(strVal);
}

bool ExprAST::equal(const ExprAST& other) const {
    return gid() == other.gid();
}

// ---------------------------------------------------------------------

template<>
Value* PrimitiveExprAST<int>::codegen() const {
    return llvm::ConstantInt::get(getGlobalContext(), APInt(32, Val, 1));
}

template<>
Value* PrimitiveExprAST<bool>::codegen() const {
    if(Val == true)
        return llvm::ConstantInt::getTrue(getGlobalContext());
    else
        return llvm::ConstantInt::getFalse(getGlobalContext());
}

template<>
Value* PrimitiveExprAST<double>::codegen() const {
    return llvm::ConstantFP::get(getGlobalContext(), APFloat(Val));
}

template<typename T>
Value* PrimitiveExprAST<T>::codegen() const {
    return nullptr;
}

Value* VariableExprAST::codegen() const {
    using namespace llvm;
    // Assuming that everything is an int.
     // --> where does this assumption appear?
    if(ctx().VarsInScope.count(Name) < 1){
        ctx().AddError("Variable '" + Name + "' is not available in this scope.");
        return nullptr;
    }
    AllocaInst* alloca = ctx().VarsInScope[Name].first;
    Value* V = ctx().Builder.CreateLoad(alloca, Name.c_str());
    return V;
}

Value* BinaryExprAST::codegen() const {
    Value* valL = LHS->codegen();
    Value* valR = RHS->codegen();
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

    auto fitit = ctx().BinOpCreator.end();
    if (valL->getType()->isIntegerTy() && valR->getType()->isIntegerTy())
        fitit = ctx().BinOpCreator.find(std::make_tuple(Opcode, CCIntegerType(), CCIntegerType()));
    else if (valL->getType()->isDoubleTy() && valR->getType()->isDoubleTy())
        fitit = ctx().BinOpCreator.find(std::make_tuple(Opcode, CCDoubleType(), CCDoubleType()));

    if(fitit != ctx().BinOpCreator.end())
        return (fitit->second)(valL, valR);
    else {
        ctx().AddError("Operator's '" + Opcode + "' codegen is not implemented!");
        return nullptr;
    }
}

Value* ReturnExprAST::codegen() const {
    using namespace llvm;
    
    Value* val = Expression->codegen();
    if(!val) return nullptr;

    Function* parent = ctx().CurrentFunc;
    BasicBlock* returnBB    = BasicBlock::Create(getGlobalContext(), "returnBB");
    BasicBlock* discardBB   = BasicBlock::Create(getGlobalContext(), "breakDiscard");

    ctx().Builder.CreateCondBr(ConstantInt::getTrue(getGlobalContext()), returnBB, discardBB);
    parent->getBasicBlockList().push_back(returnBB);
    ctx().Builder.SetInsertPoint(returnBB);
    ctx().Builder.CreateRet(val);

    parent->getBasicBlockList().push_back(discardBB);
    ctx().Builder.SetInsertPoint(discardBB);

    return val;
}

Value* BlockAST::codegen() const {
    using namespace llvm;
    
    if(Statements.size() == 0){
        // Aah, an empty block. That seems like a special case,
        // because this results in no Value, and there would be
        // nothing we can give to our caller. So let's just create a 0
        // value and return it.
        return ConstantInt::get(getGlobalContext(), APInt(32, 0, 1));
    }else{
        // Create new stack vars.
        Function* parent = ctx().CurrentFunc;
        ScopeManager SM {this, ctx()};

        // TODO: make ScopeManager sensitive to how many variables were
        // successfully initialized
        for(auto& var : Vars){
            if(ctx().VarsInScope.count(var.first) > 0){
                ctx().AddError("Variable shadowing is not allowed");
                return nullptr;
            }
            AllocaInst* Alloca = CreateEntryBlockAlloca(parent, var.first, datatype2llvmType(var.second));
            // Initialize the var to 0.
            Value* zero = createDefaultValue(var.second);
            ctx().Builder.CreateStore(zero, Alloca);
            ctx().VarsInScope[var.first] = std::make_pair(Alloca, var.second);
        }

        // Generate statements inside the block
        Value* last = nullptr;
        bool errors = false;
        for(const auto& stat : Statements){
            last = stat->codegen();
            if(!last) errors = true;
        }
        
        if(errors) return nullptr;
        return last;
    }
}

Value* AssignmentAST::codegen() const {
    using namespace llvm;
    
    Value* Val = Expression->codegen();
    if(!Val) return nullptr;

    // Look up the target var name.
    if (ctx().VarsInScope.count(Name) < 1){
        ctx().AddError("Variable '" + Name + "' is not available in this scope.");
        return nullptr;
    }
    AllocaInst* alloca = ctx().VarsInScope[Name].first;
    ctx().Builder.CreateStore(Val, alloca);
    return Val;
}

Value* CallExprAST::codegen() const {
    // Special case for print
    if(Callee == "print"){
        // Translate the call into a call to cstdlibs' printf.
        if(Args.size() != 1){
            ctx().AddError("Function print takes 1 argument, " + std::to_string(Args.size()) + " given.");
            return nullptr;
        }
        std::vector<llvm::Value*> ArgsV;
        std::string formatSpecifier;
        if(Args[0]->maintype(ctx).second == CCDoubleType())
            formatSpecifier = "%f";
        else
            formatSpecifier = "%d";

        ArgsV.push_back( CreateI8String(ctx().TheModule.get(), (formatSpecifier + "\n").c_str(), "printf_number", ctx) );
#ifndef NDEBUG
        std::cerr << "in call will codegen arg" << std::endl;
#endif
        auto temp = Args[0]->codegen(ctx());
       // auto vare = dynamic_cast<VariableExprAST*>(Args[0]);
      //  if(vare != nullptr) {
      //      std::cerr << "codegening var
      //  }
#ifndef NDEBUG
        std::cerr << "in call after codegen arg" << std::endl;
#endif
        ArgsV.push_back( temp ); //Args[0]->codegen(ctx) );
#ifndef NDEBUG
        std::cerr << "printing: " << formatSpecifier << " : ";
        temp->dump();
        std::cerr << std::endl;
#endif
        return ctx().Builder.CreateCall(ctx().func_printf, ArgsV, "calltmp");
    }

    // Look up the name in the global module table.
    llvm::Function *CalleeF = ctx().TheModule->getFunction(Callee);
    if (!CalleeF){
        ctx().AddError("Function " + Callee + " was not declared");
        return nullptr;
    }

    // If argument mismatch error.
    if (CalleeF->arg_size() != Args.size()){
        ctx().AddError("Function " + Callee + " takes " + std::to_string(CalleeF->arg_size()) + " arguments, " + std::to_string(Args.size()) + " given.");
        return nullptr;
    }

    std::vector<llvm::Value *> ArgsV;
    for (unsigned i = 0, e = Args.size(); i != e; ++i) {
        ArgsV.push_back(Args[i]->codegen());
        if (!ArgsV.back())
            return nullptr;
    }

  return ctx().Builder.CreateCall(CalleeF, ArgsV, "calltmp");
}

Value* IfExprAST::codegen() const {
    using namespace llvm;
    
    Value* cond = Cond->codegen();
    if(!cond) return nullptr;

    Value* cmp = ctx().Builder.CreateICmpNE(cond, ConstantInt::get(getGlobalContext(), APInt(1, 0, 1)), "ifcond");

    auto parent = ctx().CurrentFunc;

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
    ctx().Builder.SetInsertPoint(ThenBB);
    Value *ThenV = Then->codegen();
    if (!ThenV) return nullptr;
    ctx().Builder.CreateBr(MergeBB);
    // Codegen of 'Then' can change the current block, update ThenBB.
    ThenBB = ctx().Builder.GetInsertBlock();

    if(Else){
        // ELSE
        // Add to parent
        parent->getBasicBlockList().push_back(ElseBB);
        // Codegen recursivelly
        ctx().Builder.SetInsertPoint(ElseBB);
        Value *ElseV = Else->codegen();
        if (!ElseV) return nullptr;
        ctx().Builder.CreateBr(MergeBB);
        // Codegen of 'Else' can change the current block, update ElseBB.
        ElseBB = ctx().Builder.GetInsertBlock();
    }

    // Further instructions are to be placed at merge block
    parent->getBasicBlockList().push_back(MergeBB);
    ctx().Builder.SetInsertPoint(MergeBB);

    return ConstantInt::get(getGlobalContext(), APInt(32, 0, 1));
}

Value* WhileExprAST::codegen() const {

    auto parent = ctx().CurrentFunc;

    BasicBlock* HeaderBB = BasicBlock::Create(getGlobalContext(), "header");
    BasicBlock* BodyBB   = BasicBlock::Create(getGlobalContext(), "body");
    BasicBlock* PostBB   = BasicBlock::Create(getGlobalContext(), "postwhile");

    ctx().Builder.CreateBr(HeaderBB);

    // HEADER
    parent->getBasicBlockList().push_back(HeaderBB);
    ctx().Builder.SetInsertPoint(HeaderBB);
    Value* cond = Cond->codegen();
    if(!cond) return nullptr;
    Value* cmp = ctx().Builder.CreateICmpNE(cond, ConstantInt::get(getGlobalContext(), APInt(1, 0, 1)), "whilecond");
    ctx().Builder.CreateCondBr(cmp, BodyBB, PostBB);

    // Update of codegening context -- we are in a loop from now on
    ctx().LoopsBBHeaderPost.push_back({HeaderBB, PostBB});

    // BODY
    parent->getBasicBlockList().push_back(BodyBB);
    ctx().Builder.SetInsertPoint(BodyBB);
    Value* BodyV = Body->codegen();
    if(!BodyV) return nullptr;
    ctx().Builder.CreateBr(HeaderBB);


    // POSTWHILE
    parent->getBasicBlockList().push_back(PostBB);
    ctx().Builder.SetInsertPoint(PostBB);

    // Update of codegening context -- we've just got out of the loop
    ctx().LoopsBBHeaderPost.pop_back();

    return ConstantInt::get(getGlobalContext(), APInt(32, 0, 1));
}

Value* ForExprAST::codegen() const {
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
     return nullptr; // TODO - adjust to changes
    /* 
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

    return block->codegen(ctx);*/
}

Value* KeywordAST::codegen() const {
    auto parent = ctx().CurrentFunc;
    switch(which) {
        case keyword::Break:
            if (!ctx().is_inside_loop()) {
                // TODO: inform the user at which line (and column)
                // they wrote `break;` outside any loop
                ctx.AddError("'break' keyword outside any loop");
                return nullptr;
            } else {
                  auto postBB = ctx().LoopsBBHeaderPost.back().second;

                  // A bit of a hack here -- we generate a non-reachable basic block
                  // because it turns out to be a lot easier then fighting with
                  // branch instructions generated by `If` or `While` statements
                  // that may occur immediately below the branch from
                  // `break` or `continue` keyword
                  BasicBlock* discardBB   = BasicBlock::Create(getGlobalContext(), "breakDiscard");
                  parent->getBasicBlockList().push_back(discardBB);
                  ctx().Builder.CreateCondBr(ConstantInt::getTrue(getGlobalContext()), postBB, discardBB);
                  ctx().Builder.SetInsertPoint(discardBB);
                  return ConstantInt::get(getGlobalContext(), APInt(32, 0, 1));
            }
            break;
        case keyword::Continue:
            if (!ctx().is_inside_loop()) {
                // TODO: inform the user at which line (and column)
                // they wrote `break;` outside any loop
                ctx.AddError("'continue' keyword outside any loop");
                return nullptr;
            } else {
                auto headerBB = ctx().LoopsBBHeaderPost.back().first;
                BasicBlock* discardBB   = BasicBlock::Create(getGlobalContext(), "continueDiscard");
                parent->getBasicBlockList().push_back(discardBB);
                ctx().Builder.CreateCondBr(ConstantInt::getTrue(getGlobalContext()), headerBB, discardBB);
                ctx().Builder.SetInsertPoint(discardBB);
                return ConstantInt::get(getGlobalContext(), APInt(32, 0, 1));
            }
            break;
    }
    return nullptr;
}

//------------------------------------------


Function* PrototypeAST::codegen() const {
  // Make the function type:  double(double,double) etc.

    // TODO: Respect argument types.
  std::vector<llvm::Type*> argsTypes;
  for (auto& p : Args) {
      argsTypes.push_back(datatype2llvmType(p.second));
  }

  FunctionType *FT =
      FunctionType::get(datatype2llvmType(ReturnType), argsTypes, false);

  auto F =
      llvm::Function::Create(FT, Function::ExternalLinkage, Name, ctx().TheModule.get());

  // Set names for all arguments.
  unsigned Idx = 0;
  for (auto &Arg : F->args())
    Arg.setName(Args[Idx++].first);

  return F;
}


llvm::Function *FunctionAST::codegen() const {
  // First, check for an existing function from a previous 'extern' declaration.
  auto TheFunction = ctx().TheModule->getFunction(Proto->getName());

  // The function was not previously declared with an extern, so we
  // need to emit the prototype declaration.
  if (!TheFunction){
    TheFunction = Proto->codegen();
  }

  // Set current function
  ctx().CurrentFunc = TheFunction;

  // Create a new basic block to start insertion into.
  BasicBlock *BB = BasicBlock::Create(getGlobalContext(), "entry", TheFunction);
  ctx().Builder.SetInsertPoint(BB);

  // Clear the scope.
  ctx().VarsInScope.clear();

  size_t i = 0;
  // Record the function arguments in the VarsInScope map.
  for (auto &Arg : TheFunction->args()){
      AllocaInst* Alloca = CreateEntryBlockAlloca(TheFunction, Arg.getName(), Arg.getType());
      ctx().Builder.CreateStore(&Arg,Alloca);
      ctx().VarsInScope[Arg.getName()] = std::make_pair(Alloca, (Proto->getArgs())[i].second);
      i++;
  }

  // Insert function body into the function insertion point.
  Value* val = Body->codegen();

  // Before terminating the function, create a default return value, in case the function body does not contain one.
  // TODO: Default return type.
  ctx().Builder.CreateRet(createDefaultValue(Proto->ReturnType));

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

CCType ExprAST::maintype() const {
    return ctx.getVoidTy();
}
/* see `tree.h`
template<typename T>
ExprType PrimitiveExprAST<T>::maintype(CodegenContext& ctx) const {
    return {std::make_shared<PrimitiveExprAST<int>>(42), CCVoidType()};
}
*/
template<>
Type PrimitiveExprAST<int>::maintype() const {
    return ctx.getIntegerTy();
}

template<>
Type PrimitiveExprAST<bool>::maintype() const {
    return ctx.getBooleanTy();
}

template<>
Type PrimitiveExprAST<double>::maintype() const {
    return ctx.getDoubleTy();
}

Type VariableExprAST::maintype() const {
    return ctx.VarsInScope[Name].second;
}

Type BinaryExprAST::maintype() const {
    return ctx.getVoidTy(); // TODO!
}

Type AssignmentAST::maintype() const {
    // TODO
    return Expression->maintype();
}

Type CallExprAST::maintype() const {
    // TODO! 
    auto CalleeF = ctx().TheModule->getFunction(Callee);
    // ad-hoc solution :(
    // the more I create such solutions, the more I think
    // we should stick to the LLVM's type system
    // as much as we can
    if(CalleeF->getReturnType()->isIntegerTy())
        return ctx.getIntegerTy();
    else if(CalleeF->getReturnType()->isDoubleTy())
        return ctx.getDoubleTy();
    else
        return ctx.getVoidTy();
}

Type PrototypeAST::maintype() const {
    return ctx.getVoidTy(); // TODO
}

Type FunctionAST::maintype() const {
    return ctx.getVoidTy();// TODO
}

BlockAST::ScopeManager~BlockAST::ScopeManager() {
    for (auto& var : parent->Vars) {
    auto it = std::find_if(ctx.VarsInScope.begin(),
                           ctx.VarsInScope.end(),
                           [&var](auto& p) {
                               return p.first == var.first;
                           });
    ctx.VarsInScope.erase(it);
    }
}

}
