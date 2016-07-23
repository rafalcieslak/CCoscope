#include "../ast/ast.h"
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
template class Proxy<StringValueAST>;
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
                       Retype->name() + ", " + Imtype->name() + ".", pos());
        return ctx().getVoidTy();
    }else if(match.type == TypeMatcher::Result::MULTIPLE){
        ctx().AddError("Multiple candidates for complex constructor and types: " +
                       Retype->name() + ", " + Imtype->name() + ".", pos());
        return ctx().getVoidTy();
    }else{
        Re = ctx().makeConvert(Re, ctx().getDoubleTy(), match.converter_functions[0], Re->pos());
        Im = ctx().makeConvert(Im, ctx().getDoubleTy(), match.converter_functions[1], Im->pos());
        return ctx().getComplexTy();
    }
}

Type StringValueAST::Typecheck_() const {
    return ctx().getStringTy();
}

Type VariableOccExprAST::Typecheck_() const {
    return ctx().getReferenceTy( ctx().GetVarInfo(Name).second );
}

Type VariableDeclExprAST::Typecheck_() const {
    if(ctx().IsVarInCurrentScope(Name)){
        ctx().AddError("Variable " + Name + " declared more than once!", pos());
    } else {
        ctx().SetVarInfo(Name, {nullptr, type});
    }
    return ctx().getVoidTy();
}

Type BinaryExprAST::Typecheck_() const {
    if(!ctx().IsBinOpAvailable(opcode)) {
        ctx().AddError("Operator's '" + opcode + "' codegen is not implemented!", pos());
        return ctx().getVoidTy();
    }
    const std::list<MatchCandidateEntry>& operator_variants = ctx().VariantsForBinOp(opcode);

    Type Ltype = LHS->Typecheck();
    Type Rtype = RHS->Typecheck();

    auto match = ctx().typematcher.Match(operator_variants, {Ltype,Rtype});

    if(match.type == TypeMatcher::Result::NONE) {
        ctx().AddError("No matching operator '" + opcode + "' found to call with types: " +
                       Ltype->name() + ", " + Rtype->name() + ".", pos());
        return ctx().getVoidTy();
    }else if(match.type == TypeMatcher::Result::MULTIPLE){
        ctx().AddError("Multiple candidates for operator '" + opcode + "' and types: " +
                       Ltype->name() + ", " + Rtype->name() + ".", pos());
        return ctx().getVoidTy();
    }else{
        LHS = ctx().makeConvert(LHS, match.match.input_types[0], match.converter_functions[0], LHS->pos());
        RHS = ctx().makeConvert(RHS, match.match.input_types[1], match.converter_functions[1], RHS->pos());
        BestOverload = match.match;
        return match.match.return_type;
    }
}

Type ReturnExprAST::Typecheck_() const {
    Type target_type = ctx().CurrentFuncReturnType();
    Type expr_type = Expression->Typecheck();

    auto return_m = MatchCandidateEntry{
        {target_type},
        ctx().getVoidTy()
    };

    auto match = ctx().typematcher.Match({return_m}, {expr_type});

    if(match.type == TypeMatcher::Result::NONE){
        ctx().AddError("Return statement failed, type mismatch: Cannot implicitly convert a " +
                       expr_type->name() + " to " + target_type->name(), pos());
        return ctx().getVoidTy();
    }else if(match.type == TypeMatcher::Result::MULTIPLE){
        ctx().AddError("Return statement failed, type mismatch: Multiple equally viable implicit conversions from " +
                       expr_type->name() + " to " + target_type->name() + " are available", pos());
        return ctx().getVoidTy();
    }else{
        if(match.match.input_types[0] != target_type) {
            ctx().AddError("ReturnExprAST::Typecheck_() error: target type (" + target_type->name() + ") != match type (" +
                           match.match.input_types[0]->name() + ").", pos());
        }
        Expression = ctx().makeConvert(Expression, match.match.input_types[0], match.converter_functions[0], Expression->pos());
        return target_type;
    }
}

Type BlockAST::Typecheck_() const {
    ctx().EnterScope();

    for(const auto& stat : Statements){
        stat->Typecheck();
    }
    ctx().CloseScope();

    return ctx().getVoidTy();
}

Type CallExprAST::Typecheck_() const {

    if(Callee == "print"){
        // Special case for print
        // Translate the call into a call to stdlibs function.
        if(Args.size() != 1){
            ctx().AddError("Function print takes 1 argument, " + std::to_string(Args.size()) + " given.", pos());
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
        auto print_variant_string = MatchCandidateEntry{
            {ctx().getStringTy()},
            ctx().getVoidTy()
        };

        auto expr_type = Args[0]->Typecheck();
        auto match = ctx().typematcher.Match({print_variant_int,
                                              print_variant_double,
                                              print_variant_boolean,
                                              print_variant_complex,
                                              print_variant_string},
                                             {expr_type}
            );

        if(match.type == TypeMatcher::Result::NONE){
            ctx().AddError("Unable to print a variable of type " + expr_type->name(), pos());
            return ctx().getVoidTy();
        }else if(match.type == TypeMatcher::Result::MULTIPLE){
            ctx().AddError("Multiple viable implicit conversions for printing a variable of type " + expr_type->name(), pos());
            return ctx().getVoidTy();
        }else{
            Args[0] = ctx().makeConvert(Args[0], match.match.input_types[0], match.converter_functions[0], Args[0]->pos());
            BestOverload = ctx().GetStdFunction(ctx().GetPrintFunctionName(match.match.input_types[0]));
            return match.match.return_type;
        }
    }else if(Callee == "newComplex"){
        // Special case for complex constructor.
        // This might be a different AST node? Or maybe a more generic solution for such constructors?


        if(Args.size() != 2){
            ctx().AddError("Function newComplex takes 2 argument, " + std::to_string(Args.size()) + " given.");
            return ctx().getVoidTy();
        }

        auto newComplex_doubles = MatchCandidateEntry{
            {ctx().getDoubleTy(), ctx().getDoubleTy()},
            ctx().getComplexTy()
        };

        auto arg0_type = Args[0]->Typecheck();
        auto arg1_type = Args[1]->Typecheck();
        auto match = ctx().typematcher.Match({newComplex_doubles},
                                             {arg0_type, arg1_type}
            );

        if(match.type == TypeMatcher::Result::NONE){
            ctx().AddError("Unable to create a Complex value from types " + arg0_type->name() + " and " + arg1_type->name(), pos());
            return ctx().getVoidTy();
        }else if(match.type == TypeMatcher::Result::MULTIPLE){
            ctx().AddError("Multiple viable implicit conversions for creating a Complex value from types " + arg0_type->name() + " and " + arg1_type->name(), pos());
            return ctx().getVoidTy();
        }else{
            Args[0] = ctx().makeConvert(Args[0], match.match.input_types[0], match.converter_functions[0], Args[0]->pos());
            Args[1] = ctx().makeConvert(Args[1], match.match.input_types[1], match.converter_functions[1], Args[1]->pos());
            BestOverload = ctx().GetStdFunction("complex_new");
            return match.match.return_type;
        }
    }

    // Find a a corresponding candidate.
    if(!ctx().HasPrototype(Callee)){
        ctx().AddError("Function " + Callee + " was not declared", pos());
        return ctx().getVoidTy();
    }
    Prototype proto = ctx().GetPrototype(Callee);
    std::vector<Type> signature = proto->GetSignature();

    // Look up the name in the global module table.
    llvm::Function *CalleeF = ctx().TheModule()->getFunction(Callee);
    if (!CalleeF){
        ctx().AddError("Internal error: Function " + Callee + " is present in prototypes map but was not declared in the module", pos());
        return ctx().getVoidTy();
    }

    // If argument mismatch error.
    if (signature.size() != Args.size()){
        ctx().AddError("Function " + Callee + " takes " + std::to_string(signature.size()) + " arguments, " +
                       std::to_string(Args.size()) + " given.", pos());
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
        ctx().AddError("Unable to call `" + Callee + "`: argument type mismatch", pos());
        return ctx().getVoidTy();
    }else if(match.type == TypeMatcher::Result::MULTIPLE){
        ctx().AddError("Multiple viable implicit conversions available for calling " + Callee, pos());
        return ctx().getVoidTy();
    }else{
        for(size_t i = 0; i < Args.size(); i++) {
            Args[i] = ctx().makeConvert(Args[i], match.match.input_types[i], match.converter_functions[i], Args[i]->pos());
        }

        // currently no overloading, so one candidate available
        BestOverload = ctx().TheModule()->getFunction(Callee);
        return match.match.return_type;
    }
}

Type IfExprAST::Typecheck_() const {
    auto CondType = Cond->Typecheck();
    if(CondType != ctx().getBooleanTy()) {
        ctx().AddError("Cond in If statement has type " + CondType->name(), pos());
        return ctx().getVoidTy();
    }

    Then->Typecheck();
    Else->Typecheck();
    return ctx().getVoidTy();
}

Type WhileExprAST::Typecheck_() const {
    auto CondType = Cond->Typecheck();
    if(CondType != ctx().getBooleanTy()) {
        ctx().AddError("Cond in While statement has type " + CondType->name(), pos());
        return ctx().getVoidTy();
    }

    Body->Typecheck();
    return ctx().getVoidTy();
}

Type ForExprAST::Typecheck_() const {
    auto CondType = Cond->Typecheck();
    if(CondType != ctx().getBooleanTy()) {
        ctx().AddError("Cond in For statement has type " + CondType->name(), pos());
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
    ctx().EnterScope();

    for(auto& p : Proto->Args) {
        ctx().SetVarInfo(p.first, {nullptr, p.second});
    }

    ctx().SetCurrentFuncName(Proto->getName());
    ctx().SetCurrentFuncReturnType(Proto->ReturnType);
    /*auto BodyType =*/ Body->Typecheck();
    // can we ignore BodyType?
    ctx().CloseScope();

    auto res = Proto->Typecheck();
    ctx().ClearVarsInfo();
    return res;
}

Type ConvertAST::Typecheck_() const {
    return ResultingType;
}

}
