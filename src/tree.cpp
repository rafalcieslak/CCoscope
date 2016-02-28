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
    return nullptr;
}

llvm::Value* ComplexValueAST::codegen() const {
    if(!resolved && !Resolve()) return nullptr;

    llvm::Value* valRe = Re->codegen();
    llvm::Value* valIm = Im->codegen();
    if(!valRe || !valIm) return nullptr;

    auto converted = match_result.converter_function(ctx(), {valRe, valIm});
    return match_result.match.associated_function(converted);
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
    // If already resolved - OK. Otherwise try to resolve, if failed - return.
    // TODO: Remember that resolution failed in order not to redo it.
    if(!resolved && !Resolve()) return nullptr;

    // First, codegen both children
    llvm::Value* valL = LHS->codegen();
    llvm::Value* valR = RHS->codegen();
    if(!valL || !valR) return nullptr;

    // Then, perform the conversion.
    auto converted = match_result.converter_function(ctx(), {valL, valR});
    // Finally, call the operator performer
    return match_result.match.associated_function(converted);
}

llvm::Value* ReturnExprAST::codegen() const {
    // If already resolved - OK. Otherwise try to resolve, if failed - return.
    // TODO: Remember that resolution failed in order no to redo it.
    if(!resolved && !Resolve()) return nullptr;

    llvm::Value* Val = Expression->codegen();
    if(!Val) return nullptr;

    // First perform conversions
    auto converted = match_result.converter_function(ctx(), {Val});
    // Then perform the assignment
    return match_result.match.associated_function(converted);
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

llvm::Value* AssignmentAST::codegen() const {
    // If already resolved - OK. Otherwise try to resolve, if failed - return.
    // TODO: Remember that resolution failed in order no to redo it.
    if(!resolved && !Resolve()) return nullptr;

    llvm::Value* Val = Expression->codegen();
    if(!Val) return nullptr;

    // First perform conversions
    auto converted = match_result.converter_function(ctx(), {Val});
    // Then perform the assignment
    return match_result.match.associated_function(converted);
}

llvm::Value* CallExprAST::codegen() const {
    // If already resolved - OK. Otherwise try to resolve, if failed - return.
    // TODO: Remember that resolution failed in order no to redo it.
    if(!resolved && !Resolve()) return nullptr;

    // Special case for print
    if(Callee == "print"){
        // Codegen arguments
        auto Val = Args[0]->codegen();
        if(!Val) return nullptr;

        // Then perform conversions
        auto converted = match_result.converter_function(ctx(), {Val});
        // Finally generate the call
        return match_result.match.associated_function(converted);

    }else{
        // Codegen arguments
        std::vector<llvm::Value *> ArgsV;
        for (unsigned i = 0, e = Args.size(); i != e; ++i) {
            ArgsV.push_back(Args[i]->codegen());
            if (!ArgsV.back())
                return nullptr;
        }

        // Perform conversion
        auto converted = match_result.converter_function(ctx(), ArgsV);
        // Perform the call
        return match_result.match.associated_function(converted);
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

llvm::Value* KeywordAST::codegen() const {
    using namespace llvm;

    auto parent = ctx().CurrentFunc;
    switch(which) {
        case keyword::Break:
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
        case keyword::Continue:
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
      llvm::Function::Create(this->maintype().as<FunctionType>()->toLLVMs(),
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

llvm::Function *FunctionAST::codegen() const {
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

// --------------------------------------------------
// Typechecking
// --------------------------------------------------

Type ExprAST::maintype() const {
    return ctx().getVoidTy();
}
/* see `tree.h`
template<typename T>
ExprType PrimitiveExprAST<T>::maintype(CodegenContext& ctx) const {
    return {std::make_shared<PrimitiveExprAST<int>>(42), CCVoidType()};
}
*/
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

// =================================

bool ComplexValueAST::Resolve() const {
    Type Retype = Re->maintype();
    Type Imtype = Im->maintype();
    auto dblt = ctx().getDoubleTy();
    auto ret = MatchCandidateEntry{
        {dblt, dblt},
        [this](std::vector<llvm::Value*> v){
            auto rev = v[0];
            auto imv = v[1];
            auto rets = ctx().VarsInScope["Cmplx"].first;
            auto idx1 = ctx().Builder.CreateStructGEP(ctx().getComplexTy()->toLLVMs(), rets, 0);
            ctx().Builder.CreateStore(rev, idx1);
            auto idx2 = ctx().Builder.CreateStructGEP(ctx().getComplexTy()->toLLVMs(), rets, 1);
            ctx().Builder.CreateStore(imv, idx2);
            
            return rets;
        },
        ctx().getComplexTy()
    };
    
    auto match = ctx().typematcher.Match({ret}, {Retype, Imtype});
    
    if(match.type == TypeMatcher::Result::NONE) {
        ctx().AddError("No matching complex constructor found to call with types: " +
                     Retype->name() + ", " + Imtype->name() + ".");
        return false;
    }else if(match.type == TypeMatcher::Result::MULTIPLE){
        ctx().AddError("Multiple candidates for complex constructor and types: " +
                     Retype->name() + ", " + Imtype->name() + ".");
        return false;
    }else{
        resolved = true;
        match_result = match;
        return true;
    }
}

bool BinaryExprAST::Resolve() const{
    // Get the list of operator variants under this name
    auto opit = ctx().BinOpCreator.find(opcode);
    if(opit == ctx().BinOpCreator.end()) {
        ctx().AddError("Operator's '" + opcode + "' codegen is not implemented!");
        return false;
    }
    const std::list<MatchCandidateEntry>& operator_variants = opit->second;

    Type Ltype = LHS->maintype();
    Type Rtype = RHS->maintype();;
    auto match = ctx().typematcher.Match(operator_variants, {Ltype,Rtype});

    if(match.type == TypeMatcher::Result::NONE) {
        ctx().AddError("No matching operator '" + opcode + "' found to call with types: " +
                     Ltype->name() + ", " + Rtype->name() + ".");
        return false;
    }else if(match.type == TypeMatcher::Result::MULTIPLE){
        ctx().AddError("Multiple candidates for operator '" + opcode + "' and types: " +
                     Ltype->name() + ", " + Rtype->name() + ".");
        return false;
    }else{
        resolved = true;
        match_result = match;
        return true;
    }
}

bool ReturnExprAST::Resolve() const{
    // TODO: This looks VERY similar to assignmentAST cogeden. Is there some wise way to unify these (and related?)

    Type target_type = ctx().CurrentFuncReturnType;

    auto return_m = MatchCandidateEntry{
        {target_type},
        [this](std::vector<llvm::Value*> v){
            auto val = v[0];
            llvm::Function* parent = this->ctx().CurrentFunc;
            llvm::BasicBlock* returnBB    = llvm::BasicBlock::Create(llvm::getGlobalContext(), "returnBB");
            llvm::BasicBlock* discardBB   = llvm::BasicBlock::Create(llvm::getGlobalContext(), "returnDiscard");

            this->ctx().Builder.CreateCondBr(llvm::ConstantInt::getTrue(llvm::getGlobalContext()), returnBB, discardBB);
            parent->getBasicBlockList().push_back(returnBB);
            this->ctx().Builder.SetInsertPoint(returnBB);
            this->ctx().Builder.CreateRet(val);

            parent->getBasicBlockList().push_back(discardBB);
            this->ctx().Builder.SetInsertPoint(discardBB);

            return val;

        },
        ctx().getVoidTy()
    };
    Type expr_type = Expression->maintype();
    auto match = ctx().typematcher.Match({return_m}, {expr_type});


    if(match.type == TypeMatcher::Result::NONE){
        ctx().AddError("Return statement failed, type mismatch: Cannot implicitly convert a " +
                       expr_type->name() + " to " + target_type->name());
        return false;
    }else if(match.type == TypeMatcher::Result::MULTIPLE){
        ctx().AddError("Return statement failed, type mismatch: Multiple equally viable implicit conversions from " +
                       expr_type->name() + " to " + target_type->name() + " are available");
        return false;
    }else{
        resolved = true;
        match_result = match;
        return true;
    }
}

bool AssignmentAST::Resolve() const{

    // TODO: The assignment should really be implemented as a binary operator.

    // Look up the target var name.
    if (ctx().VarsInScope.count(Name) < 1){
        ctx().AddError("Variable '" + Name + "' is not available in this scope.");
        return false;
    }

    llvm::AllocaInst* alloca = ctx().VarsInScope[Name].first;
    Type target_type = ctx().VarsInScope[Name].second;

    auto assignment = MatchCandidateEntry{
        {target_type},
        [this, alloca](std::vector<llvm::Value*> v){
            return this->ctx().Builder.CreateStore(v[0], alloca);
        },
        ctx().getVoidTy()
    };
    Type expr_type = Expression->maintype();
    auto match = ctx().typematcher.Match({assignment}, {expr_type});

    if(match.type == TypeMatcher::Result::NONE){
        ctx().AddError("Assignment failed, type mismatch: Cannot implicitly convert a " +
                       expr_type->name() + " to " + target_type->name());
        return true;
    }else if(match.type == TypeMatcher::Result::MULTIPLE){
        ctx().AddError("Assignment failed, type mismatch: Multiple equally viable implicit conversions from " +
                       expr_type->name() + " to " + target_type->name() + " are available");
        return true;
    }else{
        resolved = true;
        match_result = match;
        return true;
    }
}

bool CallExprAST::Resolve() const{
    // Special case for print
    if(Callee == "print"){
        // Translate the call into a call to stdlibs function.
        if(Args.size() != 1){
            ctx().AddError("Function print takes 1 argument, " + std::to_string(Args.size()) + " given.");
            return false;
        }

        auto print_variant_int = MatchCandidateEntry{
            {ctx().getIntegerTy()},
            [this](std::vector<llvm::Value*> v){
                return ctx().Builder.CreateCall(ctx().GetStdFunction("print_int"), v);
            },
            ctx().getVoidTy()
        };
        auto print_variant_double = MatchCandidateEntry{
            {ctx().getDoubleTy()},
            [this](std::vector<llvm::Value*> v){
                return ctx().Builder.CreateCall(ctx().GetStdFunction("print_double"), v);
            },
            ctx().getVoidTy()
        };
        auto print_variant_boolean = MatchCandidateEntry{
            {ctx().getBooleanTy()},
            [this](std::vector<llvm::Value*> v){
                return ctx().Builder.CreateCall(ctx().GetStdFunction("print_bool"), v);
            },
            ctx().getVoidTy()
        };
      /*  auto print_variant_boolean = MatchCandidateEntry{
            {ctx().getComplexTy()},
            [this](std::vector<llvm::Value*> v){
                return ctx().Builder.CreateCall(ctx().GetStdFunction("print_bool"), v);
            },
            ctx().getVoidTy()
        };*/

        auto expr_type = Args[0]->maintype();
        auto match = ctx().typematcher.Match({print_variant_int,
                                              print_variant_double,
                                              print_variant_boolean},
                                             {expr_type}
            );

        if(match.type == TypeMatcher::Result::NONE){
            ctx().AddError("Unable to print a variable of type " + expr_type->name());
            return false;
        }else if(match.type == TypeMatcher::Result::MULTIPLE){
            ctx().AddError("Multiple viable implicit conversions for printing a variable of type " + expr_type->name());
            return false;
        }else{
            resolved = true;
            match_result = match;
            return true;
        }

    } // if callee == print

    // Find a a coressponding candidate.
    auto it = ctx().prototypesMap.find(Callee);
    if(it == ctx().prototypesMap.end()){
        ctx().AddError("Function " + Callee + " was not declared");
        return false;
    }
    Prototype proto = it->second;
    std::vector<Type> signature = proto->GetSignature();

    // Look up the name in the global module table.
    llvm::Function *CalleeF = ctx().TheModule->getFunction(Callee);
    if (!CalleeF){
        ctx().AddError("Internal error: Function " + Callee + " is present in prototypes map but was not declared in the module");
        return false;
    }

    // If argument mismatch error.
    if (signature.size() != Args.size()){
        ctx().AddError("Function " + Callee + " takes " + std::to_string(signature.size()) + " arguments, " +
                       std::to_string(Args.size()) + " given.");
        return false;
    }

    // TODO: When function overloading is implemented, there will be multiple variants to call.
    auto call_variant = MatchCandidateEntry{
        {signature},
        [this, CalleeF](std::vector<llvm::Value*> v){
            return this->ctx().Builder.CreateCall(CalleeF, v, "calltmp");
        },
        proto->GetReturnType()
    };

    std::vector<Type> argtypes;
    for (unsigned i = 0, e = Args.size(); i != e; ++i)
        argtypes.push_back(Args[i]->maintype());

    auto match = ctx().typematcher.Match({call_variant}, argtypes);

    if(match.type == TypeMatcher::Result::NONE){
        ctx().AddError("Unable to call `" + Callee + "`: argument type mismatch");
        return false;
    }else if(match.type == TypeMatcher::Result::MULTIPLE){
        ctx().AddError("Multiple viable implicit conversions available for calling " + Callee);
        return false;
    }else{
        resolved = true;
        match_result = match;
        return true;
    }
}


}
