#include "tree.h"
#include "codegencontext.h"

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
template class Proxy<CallExprAST>;
template class Proxy<IfExprAST>;
template class Proxy<WhileExprAST>;
template class Proxy<ForExprAST>;
template class Proxy<LoopControlStmtAST>;
template class Proxy<PrototypeAST>;
template class Proxy<FunctionAST>;

bool ExprAST::equal(const ExprAST& other) const {
    return gid() == other.gid();
}

// ---------------------------------------------------------------------

template<>
llvm::Value* PrimitiveExprAST<int>::codegen() const {
    return llvm::ConstantInt::get(llvm::getGlobalContext(), llvm::APInt(32, Val, 1));
}

template<>
llvm::Value* PrimitiveExprAST<bool>::codegen() const {
    if(Val == true)
        return llvm::ConstantInt::getTrue(llvm::getGlobalContext());
    else
        return llvm::ConstantInt::getFalse(llvm::getGlobalContext());
}

template<>
llvm::Value* PrimitiveExprAST<double>::codegen() const {
    return llvm::ConstantFP::get(llvm::getGlobalContext(), llvm::APFloat(Val));
}

template<typename T>
llvm::Value* PrimitiveExprAST<T>::codegen() const {
    ctx().AddError("Codegen of unknown PrimitiveExpr");
    return nullptr;
}

llvm::Value* ComplexValueAST::codegen() const {
    llvm::Value* rev = Re->codegen();
    llvm::Value* imv = Im->codegen();
    if(!rev || !imv) return nullptr;
    
    auto rets = ctx().VarsInScope["Cmplx"].first;
    auto idx1 = ctx().Builder.CreateStructGEP(ctx().getComplexTy()->toLLVMs(), rets, 0);
    ctx().Builder.CreateStore(rev, idx1);
    auto idx2 = ctx().Builder.CreateStructGEP(ctx().getComplexTy()->toLLVMs(), rets, 1);
    ctx().Builder.CreateStore(imv, idx2);
    auto retsload = ctx().Builder.CreateLoad(rets, "Cmplxloadret");
    return retsload;
}

llvm::Value* VariableExprAST::codegen() const {
    using namespace llvm;

    if(ctx().VarsInScope.count(Name) < 1){
        ctx().AddError("Variable '" + Name + "' is not available in this scope.");
        return nullptr;
    }
    return ctx().VarsInScope[Name].first;
}

llvm::Value* BinaryExprAST::codegen() const {
    // First, codegen both children
    llvm::Value* valL = LHS->codegen();
    llvm::Value* valR = RHS->codegen();
    if(!valL || !valR) return nullptr;
    
    auto binfun = ctx().BinOpCreator[{opcode, BestOverload}];
    return binfun({valL, valR});
}

llvm::Value* ReturnExprAST::codegen() const {
    llvm::Value* Val = Expression->codegen();
    if(!Val) return nullptr;
    
    llvm::Function* parent = this->ctx().CurrentFunc;
    llvm::BasicBlock* returnBB    = llvm::BasicBlock::Create(llvm::getGlobalContext(), "returnBB");
    llvm::BasicBlock* discardBB   = llvm::BasicBlock::Create(llvm::getGlobalContext(), "returnDiscard");

    this->ctx().Builder.CreateCondBr(llvm::ConstantInt::getTrue(llvm::getGlobalContext()), returnBB, discardBB);
    parent->getBasicBlockList().push_back(returnBB);
    this->ctx().Builder.SetInsertPoint(returnBB);
    this->ctx().Builder.CreateRet(Val);

    parent->getBasicBlockList().push_back(discardBB);
    this->ctx().Builder.SetInsertPoint(discardBB);

    return Val;
}

llvm::Value* BlockAST::codegen() const {
    using namespace llvm;

    if(Statements.size() == 0){
        // Aah, an empty block. That seems like a special case,
        // because this results in no Value, and there would be
        // nothing we can give to our caller. So let's just create a 0
        // value and return it.
        return ConstantInt::get(llvm::getGlobalContext(), APInt(32, 0, 1));
    }else{
        // Create new stack vars.
        llvm::Function* parent = ctx().CurrentFunc;
        ScopeManager SM {this, ctx()};

        // TODO: make ScopeManager sensitive to how many variables were
        // successfully initialized
        for(auto& var : Vars){
            if(ctx().VarsInScope.count(var.first) > 0){
                ctx().AddError("Variable shadowing is not allowed");
                return nullptr;
            }
            AllocaInst* Alloca = CreateEntryBlockAlloca(parent, var.first, var.second->toLLVMs());
            // Initialize the var to 0.
            Value* zero = var.second->defaultLLVMsValue();
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

llvm::Value* CallExprAST::codegen() const {
    // Special case for print
    if(Callee == "print"){
        // Codegen arguments
        auto Val = Args[0]->codegen();
        if(!Val) return nullptr;
        
        if(Args[0]->GetType() == ctx().getComplexTy()) {
            auto rev = ctx().Builder.CreateExtractValue(Val, {0});
            auto imv = ctx().Builder.CreateExtractValue(Val, {1});
            return ctx().Builder.CreateCall(BestOverload, {rev, imv});
        } else {
            return ctx().Builder.CreateCall(BestOverload, Val);
        }

    }else{
        // Codegen arguments
        std::vector<llvm::Value *> ArgsV;
        for (unsigned i = 0, e = Args.size(); i != e; ++i) {
            ArgsV.push_back(Args[i]->codegen());
            if (!ArgsV.back())
                return nullptr;
        }
        
        return ctx().Builder.CreateCall(BestOverload, ArgsV, "calltmp");
    }
}

llvm::Value* IfExprAST::codegen() const {
    using namespace llvm;

    Value* cond = Cond->codegen();
    if(!cond) return nullptr;

    Value* cmp = ctx().Builder.CreateICmpNE(cond, ConstantInt::get(llvm::getGlobalContext(), APInt(1, 0, 1)), "ifcond");

    auto parent = ctx().CurrentFunc;

    BasicBlock *ThenBB  = BasicBlock::Create(llvm::getGlobalContext(), "then");
    BasicBlock *MergeBB = BasicBlock::Create(llvm::getGlobalContext(), "ifcont");

    BasicBlock *ElseBB;
    if(Else){
        // There is an else-block for this if. Create BB for else.
        ElseBB  = BasicBlock::Create(llvm::getGlobalContext(), "else");
    }else{
        // There is no else-block for this if. Do not create a block. Make sure that failing the condition will jump to merge.
        ElseBB = MergeBB;
    }

    ctx().Builder.CreateCondBr(cmp, ThenBB, ElseBB);

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

    return ConstantInt::get(llvm::getGlobalContext(), APInt(32, 0, 1));
}

llvm::Value* WhileExprAST::codegen() const {
    using namespace llvm;

    auto parent = ctx().CurrentFunc;

    BasicBlock* HeaderBB = BasicBlock::Create(llvm::getGlobalContext(), "header");
    BasicBlock* BodyBB   = BasicBlock::Create(llvm::getGlobalContext(), "body");
    BasicBlock* PostBB   = BasicBlock::Create(llvm::getGlobalContext(), "postwhile");

    ctx().Builder.CreateBr(HeaderBB);

    // HEADER
    parent->getBasicBlockList().push_back(HeaderBB);
    ctx().Builder.SetInsertPoint(HeaderBB);
    Value* cond = Cond->codegen();
    if(!cond) return nullptr;
    Value* cmp = ctx().Builder.CreateICmpNE(cond, ConstantInt::get(llvm::getGlobalContext(), APInt(1, 0, 1)), "whilecond");
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

    return ConstantInt::get(llvm::getGlobalContext(), APInt(32, 0, 1));
}

llvm::Value* ForExprAST::codegen() const {
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

    auto body = Body->as<BlockAST>();
    auto innerVars = body->Vars;
    auto innerStatements = body->Statements;
    innerStatements.insert(innerStatements.end(), Step.begin(), Step.end());
    auto whileAST = ctx().makeWhile(Cond,
        ctx().makeBlock(innerVars, innerStatements));

    auto init = Init->as<BlockAST>();
    auto outerStatements = init->Statements;
    outerStatements.push_back(whileAST);

    auto block = ctx().makeBlock(init->Vars, outerStatements);

    return block->codegen();
}

llvm::Value* LoopControlStmtAST::codegen() const {
    using namespace llvm;

    auto parent = ctx().CurrentFunc;
    switch(which) {
        case loopControl::Break:
            if (!ctx().is_inside_loop()) {
                // TODO: inform the user at which line (and column)
                // they wrote `break;` outside any loop
                ctx().AddError("'break' keyword outside any loop");
                return nullptr;
            } else {
                  auto postBB = ctx().LoopsBBHeaderPost.back().second;

                  // A bit of a hack here -- we generate a non-reachable basic block
                  // because it turns out to be a lot easier then fighting with
                  // branch instructions generated by `If` or `While` statements
                  // that may occur immediately below the branch from
                  // `break` or `continue` keyword
                  BasicBlock* discardBB   = BasicBlock::Create(llvm::getGlobalContext(), "breakDiscard");
                  parent->getBasicBlockList().push_back(discardBB);
                  ctx().Builder.CreateCondBr(ConstantInt::getTrue(llvm::getGlobalContext()), postBB, discardBB);
                  ctx().Builder.SetInsertPoint(discardBB);
                  return ConstantInt::get(llvm::getGlobalContext(), APInt(32, 0, 1));
            }
            break;
        case loopControl::Continue:
            if (!ctx().is_inside_loop()) {
                // TODO: inform the user at which line (and column)
                // they wrote `continue;` outside any loop
                ctx().AddError("'continue' keyword outside any loop");
                return nullptr;
            } else {
                auto headerBB = ctx().LoopsBBHeaderPost.back().first;
                BasicBlock* discardBB   = BasicBlock::Create(llvm::getGlobalContext(), "continueDiscard");
                parent->getBasicBlockList().push_back(discardBB);
                ctx().Builder.CreateCondBr(ConstantInt::getTrue(llvm::getGlobalContext()), headerBB, discardBB);
                ctx().Builder.SetInsertPoint(discardBB);
                return ConstantInt::get(llvm::getGlobalContext(), APInt(32, 0, 1));
            }
            break;
    }
    return nullptr;
}

//------------------------------------------


llvm::Function* PrototypeAST::codegen() const {
    auto F =
      llvm::Function::Create(this->GetType().as<FunctionType>()->toLLVMs(),
        llvm::Function::ExternalLinkage, Name, ctx().TheModule.get()
    );

    // Set names for all arguments.
    unsigned Idx = 0;
    for (auto &Arg : F->args())
        Arg.setName(Args[Idx++].first);

    return F;
}

std::vector<Type> PrototypeAST::GetSignature() const{
    std::vector<Type> result;
    for(const auto& p : Args) result.push_back(p.second);
    return result;
}

llvm::Function* FunctionAST::codegen() const {
    using namespace llvm;

    // First, check for an existing function from a previous 'extern' declaration.
    auto TheFunction = ctx().TheModule->getFunction(Proto->getName());

    // The function was not previously declared with an extern, so we
    // need to emit the prototype declaration.
    if (!TheFunction){
        TheFunction = Proto->codegen();
    }

    // Set current function
    ctx().CurrentFunc = TheFunction;
    ctx().CurrentFuncReturnType = Proto->ReturnType;

    // Create a new basic block to start insertion into.
    BasicBlock *BB = BasicBlock::Create(llvm::getGlobalContext(), "entry", TheFunction);
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
    ctx().Builder.CreateRet(Proto->ReturnType->defaultLLVMsValue());

    if(val){
    // Validate the generated code, checking for consistency.
        verifyFunction(*TheFunction);

        return TheFunction;
    }

    // Codegenning body returned nullptr, so an error was encountered. Remove the function.
    TheFunction->eraseFromParent();
    return nullptr;
}

llvm::Value* ConvertAST::codegen() const {
    auto ev = Expression->codegen();
    if(!ev) return nullptr;
    
    return Converter(ev);
}

// --------------------------------------------------
// Typechecking
// --------------------------------------------------

Type ExprAST::GetType() const {
    return Typecheck();
}

Type ExprAST::Typecheck() const {
    if(type_cache_.is_empty()) {
        type_cache_ = Typecheck_();
    }
    return type_cache_;
}

Type ExprAST::Typecheck_() const {
    return ctx().getVoidTy(); // or a poison/bottom type?
}

template<>
Type PrimitiveExprAST<int>::Typecheck_() const {
    return ctx().getIntegerTy();
}

template<>
Type PrimitiveExprAST<bool>::Typecheck_() const {
    return ctx().getBooleanTy();
}

template<>
Type PrimitiveExprAST<double>::Typecheck_() const {
    return ctx().getDoubleTy();
}

Type ComplexValueAST::Typecheck_() const {
    auto Retype = Re->Typecheck();
    auto Imtype = Im->Typecheck();
    
    auto rett = MatchCandidateEntry{
        {ctx().getDoubleTy(), ctx().getDoubleTy()},
        ctx().getComplexTy()
    };

    auto match = ctx().typematcher.Match({rett}, {Retype, Imtype});

    if(match.type == TypeMatcher::Result::NONE) {
        ctx().AddError("No matching complex constructor found to call with types: " +
                     Retype->name() + ", " + Imtype->name() + ".");
        return ctx().getVoidTy();
    }else if(match.type == TypeMatcher::Result::MULTIPLE){
        ctx().AddError("Multiple candidates for complex constructor and types: " +
                     Retype->name() + ", " + Imtype->name() + ".");
        return ctx().getVoidTy();
    }else{
        Re = ctx().makeConvert(Re, ctx().getDoubleTy(), match.converter_functions[0]);
        Im = ctx().makeConvert(Im, ctx().getDoubleTy(), match.converter_functions[1]);
        return ctx().getComplexTy();
    }
}

Type VariableExprAST::Typecheck_() const {
    return ctx().getReferenceTy( ctx().VarsInScope[Name].second );
}

Type BinaryExprAST::Typecheck_() const {
    auto opit = ctx().AvailableBinOps.find(opcode);
    if(opit == ctx().AvailableBinOps.end()) {
        ctx().AddError("Operator's '" + opcode + "' codegen is not implemented!");
        return ctx().getVoidTy();
    }
    const std::list<MatchCandidateEntry>& operator_variants = opit->second;

    Type Ltype = LHS->Typecheck();
    Type Rtype = RHS->Typecheck();
    
    auto match = ctx().typematcher.Match(operator_variants, {Ltype,Rtype});
    
    if(match.type == TypeMatcher::Result::NONE) {
        ctx().AddError("No matching operator '" + opcode + "' found to call with types: " +
                     Ltype->name() + ", " + Rtype->name() + ".");
        return ctx().getVoidTy();
    }else if(match.type == TypeMatcher::Result::MULTIPLE){
        ctx().AddError("Multiple candidates for operator '" + opcode + "' and types: " +
                     Ltype->name() + ", " + Rtype->name() + ".");
        return ctx().getVoidTy();
    }else{
        LHS = ctx().makeConvert(LHS, match.match.input_types[0], match.converter_functions[0]);
        RHS = ctx().makeConvert(RHS, match.match.input_types[1], match.converter_functions[1]);
        BestOverload = match.match;
        return match.match.return_type;
    }
}

Type ReturnExprAST::Typecheck_() const {
    Type target_type = ctx().CurrentFuncReturnType;
    Type expr_type = Expression->Typecheck();
    
    auto return_m = MatchCandidateEntry{
        {target_type},
        ctx().getVoidTy()
    };
    
    auto match = ctx().typematcher.Match({return_m}, {expr_type});

    if(match.type == TypeMatcher::Result::NONE){
        ctx().AddError("Return statement failed, type mismatch: Cannot implicitly convert a " +
                       expr_type->name() + " to " + target_type->name());
        return ctx().getVoidTy();
    }else if(match.type == TypeMatcher::Result::MULTIPLE){
        ctx().AddError("Return statement failed, type mismatch: Multiple equally viable implicit conversions from " +
                       expr_type->name() + " to " + target_type->name() + " are available");
        return ctx().getVoidTy();
    }else{
        if(match.match.input_types[0] != target_type) {
            ctx().AddError("ReturnExprAST::Typecheck_() error: target type (" + target_type->name() + ") != match type (" +
                match.match.input_types[0]->name() + ").");
        }
        Expression = ctx().makeConvert(Expression, match.match.input_types[0] /*target_type*/, match.converter_functions[0]);
        return target_type;
    }
}

Type BlockAST::Typecheck_() const {
    ScopeManager SM {this, ctx()};

    // TODO: make ScopeManager sensitive to how many variables were
    // successfully initialized
    for(auto& var : Vars){
        if(ctx().VarsInScope.count(var.first) > 0){
            ctx().AddError("Variable shadowing is not allowed");
            return ctx().getVoidTy();
        }
        ctx().VarsInScope[var.first] = std::make_pair(nullptr, var.second);
    }

    for(const auto& stat : Statements){
        stat->Typecheck();
    }
    
    return ctx().getVoidTy();
}

Type CallExprAST::Typecheck_() const {
    // Special case for print
    if(Callee == "print"){
        // Translate the call into a call to stdlibs function.
        if(Args.size() != 1){
            ctx().AddError("Function print takes 1 argument, " + std::to_string(Args.size()) + " given.");
            return ctx().getVoidTy();
        }

        auto print_variant_int = MatchCandidateEntry{
            {ctx().getIntegerTy()},
            ctx().getVoidTy()
        };
        auto print_variant_double = MatchCandidateEntry{
            {ctx().getDoubleTy()},
            ctx().getVoidTy()
        };
        auto print_variant_boolean = MatchCandidateEntry{
            {ctx().getBooleanTy()},
            ctx().getVoidTy()
        };
        auto print_variant_complex = MatchCandidateEntry{
            {ctx().getComplexTy()},
            ctx().getVoidTy()
        };

        auto expr_type = Args[0]->Typecheck();
        auto match = ctx().typematcher.Match({print_variant_int,
                                              print_variant_double,
                                              print_variant_boolean,
                                              print_variant_complex},
                                             {expr_type}
            );

        if(match.type == TypeMatcher::Result::NONE){
            ctx().AddError("Unable to print a variable of type " + expr_type->name());
            ctx().getVoidTy();
        }else if(match.type == TypeMatcher::Result::MULTIPLE){
            ctx().AddError("Multiple viable implicit conversions for printing a variable of type " + expr_type->name());
            ctx().getVoidTy();
        }else{
            Args[0] = ctx().makeConvert(Args[0], match.match.input_types[0], match.converter_functions[0]);
            BestOverload = ctx().GetStdFunction(ctx().GetPrintFunctionName(match.match.input_types[0]));
            return match.match.return_type;
        }

    } // if callee == print

    // Find a a corresponding candidate.
    auto it = ctx().prototypesMap.find(Callee);
    if(it == ctx().prototypesMap.end()){
        ctx().AddError("Function " + Callee + " was not declared");
        return ctx().getVoidTy();
    }
    Prototype proto = it->second;
    std::vector<Type> signature = proto->GetSignature();

    // Look up the name in the global module table.
    llvm::Function *CalleeF = ctx().TheModule->getFunction(Callee);
    if (!CalleeF){
        ctx().AddError("Internal error: Function " + Callee + " is present in prototypes map but was not declared in the module");
        return ctx().getVoidTy();
    }

    // If argument mismatch error.
    if (signature.size() != Args.size()){
        ctx().AddError("Function " + Callee + " takes " + std::to_string(signature.size()) + " arguments, " +
                       std::to_string(Args.size()) + " given.");
        return ctx().getVoidTy();
    }

    // TODO: When function overloading is implemented, there will be multiple variants to call.
    auto call_variant = MatchCandidateEntry{
        {signature},
        proto->GetReturnType()
    };

    std::vector<Type> argtypes;
    for (unsigned i = 0, e = Args.size(); i != e; ++i)
        argtypes.push_back(Args[i]->Typecheck());

    auto match = ctx().typematcher.Match({call_variant}, argtypes);

    if(match.type == TypeMatcher::Result::NONE){
        ctx().AddError("Unable to call `" + Callee + "`: argument type mismatch");
        return ctx().getVoidTy();
    }else if(match.type == TypeMatcher::Result::MULTIPLE){
        ctx().AddError("Multiple viable implicit conversions available for calling " + Callee);
        return ctx().getVoidTy();
    }else{
        for(size_t i = 0; i < Args.size(); i++) {
            Args[i] = ctx().makeConvert(Args[i], match.match.input_types[i], match.converter_functions[i]);
        }
        
        // currently no overloading, so one candidate available
        BestOverload = ctx().TheModule->getFunction(Callee);
        return match.match.return_type;
    }
}

Type IfExprAST::Typecheck_() const {
    auto CondType = Cond->Typecheck();
    if(CondType != ctx().getBooleanTy()) {
        ctx().AddError("Cond in If statement has type " + CondType->name());
        return ctx().getVoidTy();
    }
    
    Then->Typecheck();
    Else->Typecheck();
    return ctx().getVoidTy();
}

Type WhileExprAST::Typecheck_() const {
    auto CondType = Cond->Typecheck();
    if(CondType != ctx().getBooleanTy()) {
        ctx().AddError("Cond in While statement has type " + CondType->name());
        return ctx().getVoidTy();
    }
    
    Body->Typecheck();
    return ctx().getVoidTy();
}

Type ForExprAST::Typecheck_() const {
    auto CondType = Cond->Typecheck();
    if(CondType != ctx().getBooleanTy()) {
        ctx().AddError("Cond in For statement has type " + CondType->name());
        return ctx().getVoidTy();
    }
    
    for(auto& E : Step) {
        E->Typecheck();
    }
    Body->Typecheck();
    return ctx().getVoidTy();
}

Type LoopControlStmtAST::Typecheck_() const {
    return ctx().getVoidTy();
}

Type PrototypeAST::Typecheck_() const {
    std::vector<Type> argsTypes;
    for (auto& p : Args) {
        argsTypes.push_back(p.second);
    }

    return ctx().getFunctionTy(ReturnType, argsTypes);
}

Type FunctionAST::Typecheck_() const {
    for(auto& p : Proto->Args) {
        ctx().VarsInScope[p.first] = std::make_pair(nullptr, p.second);
    }
    
    //ctx().CurrentFunc = TheFunction;
    ctx().CurrentFuncReturnType = Proto->ReturnType;
    /*auto BodyType =*/ Body->Typecheck();
    // can we ignore BodyType? 
    auto res = Proto->Typecheck();
    ctx().VarsInScope.clear();
    return res;
}

Type ConvertAST::Typecheck_() const {
    return ResultingType;
}

BlockAST::ScopeManager::~ScopeManager() {
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
