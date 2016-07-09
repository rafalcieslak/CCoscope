#include "tree.h"
#include "../world/codegencontext.h"

#include "llvm/IR/Verifier.h"
#include "llvm/IR/Value.h"

#define DEBUG 0

namespace ccoscope {

template class Proxy<ExprAST>;
template class Proxy<PrimitiveExprAST<int>>;
template class Proxy<PrimitiveExprAST<double>>;
template class Proxy<PrimitiveExprAST<bool>>;
template class Proxy<ComplexValueAST>;
template class Proxy<VariableOccExprAST>;
template class Proxy<VariableDeclExprAST>;
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

    auto rets = ctx().GetVarInfo("Cmplx").first;
    auto idx1 = ctx().Builder().CreateStructGEP(ctx().getComplexTy()->toLLVMs(), rets, 0);
    ctx().Builder().CreateStore(rev, idx1);
    auto idx2 = ctx().Builder().CreateStructGEP(ctx().getComplexTy()->toLLVMs(), rets, 1);
    ctx().Builder().CreateStore(imv, idx2);
    auto retsload = ctx().Builder().CreateLoad(rets, "Cmplxloadret");
    return retsload;
}

llvm::Value* VariableOccExprAST::codegen() const {
    using namespace llvm;

    if(!ctx().IsVarInSomeEnclosingScope(Name)){
        ctx().AddError("Variable '" + Name + "' is not available in this scope.", pos());
        return nullptr;
    }
    return ctx().GetVarInfo(Name).first;
}

llvm::Value* VariableDeclExprAST::codegen() const {
    using namespace llvm;
    llvm::Function* parent = ctx().CurrentFunc();
    AllocaInst* Alloca = CreateEntryBlockAlloca(parent, Name, type->toLLVMs());
    // Initialize the var to 0.
    Value* zero = type->defaultLLVMsValue();
    ctx().Builder().CreateStore(zero, Alloca);
    ctx().SetVarInfo(Name, {Alloca, type});
    return Alloca;
}

llvm::Value* BinaryExprAST::codegen() const {
    // First, codegen both children
    llvm::Value* valL = LHS->codegen();
    llvm::Value* valR = RHS->codegen();
    if(!valL || !valR) return nullptr;

    auto binfun = ctx().binOpCreator_[{opcode, BestOverload}];
    return binfun({valL, valR});
}

llvm::Value* ReturnExprAST::codegen() const {
    llvm::Value* Val = Expression->codegen();
    if(!Val) return nullptr;

    llvm::Function* parent = ctx().CurrentFunc();
    llvm::BasicBlock* returnBB   = llvm::BasicBlock::Create(llvm::getGlobalContext(), "returnBB");
    llvm::BasicBlock* discardBB  = llvm::BasicBlock::Create(llvm::getGlobalContext(), "returnDiscard");

    this->ctx().Builder().CreateCondBr(llvm::ConstantInt::getTrue(llvm::getGlobalContext()), returnBB, discardBB);
    parent->getBasicBlockList().push_back(returnBB);
    this->ctx().Builder().SetInsertPoint(returnBB);
    this->ctx().Builder().CreateRet(Val);

    parent->getBasicBlockList().push_back(discardBB);
    this->ctx().Builder().SetInsertPoint(discardBB);

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
        ctx().EnterScope();

        // Generate statements inside the block
        Value* last = nullptr;
        bool errors = false;
        for(const auto& stat : Statements){
            last = stat->codegen();
            if(!last) errors = true;
        }

        ctx().CloseScope();

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

        return ctx().Builder().CreateCall(BestOverload, Val);
    }else{
        // Codegen arguments
        std::vector<llvm::Value *> ArgsV;
        for (unsigned i = 0, e = Args.size(); i != e; ++i) {
            ArgsV.push_back(Args[i]->codegen());
            if (!ArgsV.back())
                return nullptr;
        }

        return ctx().Builder().CreateCall(BestOverload, ArgsV, "calltmp");
    }
}

llvm::Value* IfExprAST::codegen() const {
    using namespace llvm;

    Value* cond = Cond->codegen();
    if(!cond) return nullptr;

    Value* cmp = ctx().Builder().CreateICmpNE(cond, ConstantInt::get(llvm::getGlobalContext(), APInt(1, 0, 1)), "ifcond");

    auto parent = ctx().CurrentFunc();

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

    ctx().Builder().CreateCondBr(cmp, ThenBB, ElseBB);

    // THEN
    // Add to parent
    parent->getBasicBlockList().push_back(ThenBB);
    // Codegen recursivelly
    ctx().Builder().SetInsertPoint(ThenBB);
    Value *ThenV = Then->codegen();
    if (!ThenV) return nullptr;
    ctx().Builder().CreateBr(MergeBB);
    // Codegen of 'Then' can change the current block, update ThenBB.
    ThenBB = ctx().Builder().GetInsertBlock();

    if(Else){
        // ELSE
        // Add to parent
        parent->getBasicBlockList().push_back(ElseBB);
        // Codegen recursivelly
        ctx().Builder().SetInsertPoint(ElseBB);
        Value *ElseV = Else->codegen();
        if (!ElseV) return nullptr;
        ctx().Builder().CreateBr(MergeBB);
        // Codegen of 'Else' can change the current block, update ElseBB.
        ElseBB = ctx().Builder().GetInsertBlock();
    }

    // Further instructions are to be placed at merge block
    parent->getBasicBlockList().push_back(MergeBB);
    ctx().Builder().SetInsertPoint(MergeBB);

    return ConstantInt::get(llvm::getGlobalContext(), APInt(32, 0, 1));
}

llvm::Value* WhileExprAST::codegen() const {
    using namespace llvm;

    auto parent = ctx().CurrentFunc();

    BasicBlock* HeaderBB = BasicBlock::Create(llvm::getGlobalContext(), "header");
    BasicBlock* BodyBB   = BasicBlock::Create(llvm::getGlobalContext(), "body");
    BasicBlock* PostBB   = BasicBlock::Create(llvm::getGlobalContext(), "postwhile");

    ctx().Builder().CreateBr(HeaderBB);

    // HEADER
    parent->getBasicBlockList().push_back(HeaderBB);
    ctx().Builder().SetInsertPoint(HeaderBB);
    Value* cond = Cond->codegen();
    if(!cond) return nullptr;
    Value* cmp = ctx().Builder().CreateICmpNE(cond, ConstantInt::get(llvm::getGlobalContext(), APInt(1, 0, 1)), "whilecond");
    ctx().Builder().CreateCondBr(cmp, BodyBB, PostBB);

    // Update of codegening context -- we are in a loop from now on
    ctx().PutLoopInfo(HeaderBB, PostBB);

    // BODY
    parent->getBasicBlockList().push_back(BodyBB);
    ctx().Builder().SetInsertPoint(BodyBB);
    Value* BodyV = Body->codegen();
    if(!BodyV) return nullptr;
    ctx().Builder().CreateBr(HeaderBB);


    // POSTWHILE
    parent->getBasicBlockList().push_back(PostBB);
    ctx().Builder().SetInsertPoint(PostBB);

    // Update of codegening context -- we've just got out of the loop
    ctx().PopLoopInfo();

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
    auto innerStatements = body->Statements;
    innerStatements.insert(innerStatements.end(), Step.begin(), Step.end());
    auto whileAST = ctx().makeWhile(Cond,
         ctx().makeBlock(innerStatements, pos_), pos_);

    auto init = Init->as<BlockAST>();
    auto outerStatements = init->Statements;
    outerStatements.push_back(whileAST);

    auto block = ctx().makeBlock(outerStatements, pos_);

    return block->codegen();
}

llvm::Value* LoopControlStmtAST::codegen() const {
    using namespace llvm;

    auto parent = ctx().CurrentFunc();
    switch(which) {
        case loopControl::Break:
            if (!ctx().IsInsideLoop()) {
                // TODO: inform the user at which line (and column)
                // they wrote `break;` outside any loop
                ctx().AddError("'break' keyword outside any loop", pos());
                return nullptr;
            } else {
                  auto postBB = ctx().GetCurrentLoopInfo().second;

                  // A bit of a hack here -- we generate a non-reachable basic block
                  // because it turns out to be a lot easier then fighting with
                  // branch instructions generated by `If` or `While` statements
                  // that may occur immediately below the branch from
                  // `break` or `continue` keyword
                  BasicBlock* discardBB   = BasicBlock::Create(llvm::getGlobalContext(), "breakDiscard");
                  parent->getBasicBlockList().push_back(discardBB);
                  ctx().Builder().CreateCondBr(ConstantInt::getTrue(llvm::getGlobalContext()), postBB, discardBB);
                  ctx().Builder().SetInsertPoint(discardBB);
                  return ConstantInt::get(llvm::getGlobalContext(), APInt(32, 0, 1));
            }
            break;
        case loopControl::Continue:
            if (!ctx().IsInsideLoop()) {
                // TODO: inform the user at which line (and column)
                // they wrote `continue;` outside any loop
                ctx().AddError("'continue' keyword outside any loop", pos());
                return nullptr;
            } else {
                auto headerBB = ctx().GetCurrentLoopInfo().first;
                BasicBlock* discardBB   = BasicBlock::Create(llvm::getGlobalContext(), "continueDiscard");
                parent->getBasicBlockList().push_back(discardBB);
                ctx().Builder().CreateCondBr(ConstantInt::getTrue(llvm::getGlobalContext()), headerBB, discardBB);
                ctx().Builder().SetInsertPoint(discardBB);
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
        llvm::Function::ExternalLinkage, Name, ctx().TheModule()
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
    auto TheFunction = ctx().TheModule()->getFunction(Proto->getName());

    // The function was not previously declared with an extern, so we
    // need to emit the prototype declaration.
    if (!TheFunction){
        TheFunction = Proto->codegen();
    }

    // Set current function
    ctx().SetCurrentFunc(TheFunction);
    ctx().SetCurrentFuncReturnType(Proto->ReturnType);

    // Create a new basic block to start insertion into.
    BasicBlock *BB = BasicBlock::Create(llvm::getGlobalContext(), "entry", TheFunction);
    ctx().Builder().SetInsertPoint(BB);

    // Clear the scope.
    ctx().ClearVarsInfo();

    size_t i = 0;
    // Record the function arguments in the VarsInScope map.
    ctx().EnterScope();
    for (auto &Arg : TheFunction->args()){
        AllocaInst* Alloca = CreateEntryBlockAlloca(TheFunction, Arg.getName(), Arg.getType());
        ctx().Builder().CreateStore(&Arg,Alloca);
        ctx().SetVarInfo(Arg.getName(), {Alloca, (Proto->getArgs())[i].second});
        i++;
    }

    // Insert function body into the function insertion point.
    Value* val = Body->codegen();

    ctx().CloseScope();

    // Before terminating the function, create a default return value, in case the function body does not contain one.
    // TODO: Default return type.
    ctx().Builder().CreateRet(Proto->ReturnType->defaultLLVMsValue());

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

}
