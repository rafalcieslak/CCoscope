// -*- mode: c++; fill-column: 80 -*-
#ifndef __TREE_H__
#define __TREE_H__

#include "cast.h"
#include "types.h"
#include "proxy.h"
#include "typematcher.h"
#include "utils.h"

#include <string>
#include <memory>
#include <vector>
#include <cassert>
#include <list>
#include <iostream>

namespace ccoscope {

class CodegenContext; // forward declaration needed

enum class loopControl {
    Break,
    Continue
};

class ExprAST;             using Expr                = Proxy<ExprAST>;
template<typename T> class PrimitiveExprAST;
template<typename T>       using PrimitiveExpr       = Proxy<PrimitiveExprAST<T>>;
class ComplexValueAST;     using ComplexValue        = Proxy<ComplexValueAST>;
class VariableOccExprAST;  using VariableOccExpr     = Proxy<VariableOccExprAST>;
class VariableDeclExprAST; using VariableDeclExpr    = Proxy<VariableDeclExprAST>;
class BinaryExprAST;       using BinaryExpr          = Proxy<BinaryExprAST>;
class ReturnExprAST;       using ReturnExpr          = Proxy<ReturnExprAST>;
class BlockAST;            using Block               = Proxy<BlockAST>;
class CallExprAST;         using CallExpr            = Proxy<CallExprAST>;
class IfExprAST;           using IfExpr              = Proxy<IfExprAST>;
class WhileExprAST;        using WhileExpr           = Proxy<WhileExprAST>;
class ForExprAST;          using ForExpr             = Proxy<ForExprAST>;
class LoopControlStmtAST;  using LoopControlStmt     = Proxy<LoopControlStmtAST>;
class PrototypeAST;        using Prototype           = Proxy<PrototypeAST>;
class FunctionAST;         using Function            = Proxy<FunctionAST>;
class ConvertAST;          using Convert             = Proxy<ConvertAST>;

/// ExprAST - Base class for all expression nodes.
class ExprAST : public MagicCast<ExprAST> {
public:
    ExprAST(CodegenContext& ctx, size_t gid, fileloc pos)
        : ctx_(ctx)
        , gid_(gid)
        , pos_(pos)
        , representative_(this)
    {}

    virtual ~ExprAST() {}

    virtual llvm::Value* codegen() const = 0;
    Type Typecheck() const;
    Type GetType() const;
    bool operator < (const ExprAST& other) const { return gid() < other.gid(); }

    size_t gid () const { return gid_; }
    fileloc pos () const { return pos_; }
    CodegenContext& ctx () const { return ctx_; }
    bool equal(const ExprAST& other) const;
    bool is_proxy () const { return representative_ != this; }
    bool was_typechecked () const { return type_cache_.is_empty(); }

protected:
    virtual Type Typecheck_() const;

    CodegenContext& ctx_;
    size_t gid_;
    fileloc pos_;
    mutable const ExprAST* representative_;
    mutable Type type_cache_;

    template<typename T> friend class Proxy;
};

/// PrimitiveExprAST - Expression class for numeric literals like "1.0"
/// as well as boolean constants
template<typename T>
class PrimitiveExprAST : public ExprAST {
public:
    PrimitiveExprAST(CodegenContext& ctx, size_t gid, T v, fileloc pos)
        : ExprAST(ctx, gid, pos)
        , Val(v)
    {}

    llvm::Value* codegen() const override;

protected:
    virtual Type Typecheck_() const override;

    T Val;
};
/*
// For some reason putting the implementation below to the .cpp file
// yields a compilation-time error.
// TODO: understand why :)
template<typename T>
Type PrimitiveExprAST<T>::maintype() const {
    return ctx.getVoidTy();
}*/

class ComplexValueAST : public ExprAST {
public:
    ComplexValueAST(CodegenContext& ctx, size_t gid, Expr re, Expr im, fileloc pos)
        : ExprAST(ctx, gid, pos)
        , Re(re)
        , Im(im)
    {}

    llvm::Value* codegen() const override;

protected:
    virtual Type Typecheck_() const override;

    Expr Re, Im;
};

/// VariableOccExprAST - Expression class for referencing a variable occurence, like "a".
class VariableOccExprAST : public ExprAST {
public:
    VariableOccExprAST(CodegenContext& ctx, size_t gid, const std::string &Name, fileloc pos)
        : ExprAST(ctx, gid, pos)
        , Name(Name)
    {}

    llvm::Value* codegen() const override;

protected:
    virtual Type Typecheck_() const override;

    std::string Name;
};

/// VariableDeclAST - Expression clas for variable declaration, like `x : int`
class VariableDeclExprAST : public ExprAST {
public:
    VariableDeclExprAST(CodegenContext& ctx, size_t gid, const std::string& Name, Type type, fileloc pos)
        : ExprAST(ctx, gid, pos)
        , Name(Name)
        , type(type)
    {}

    llvm::Value* codegen() const override;

protected:
    virtual Type Typecheck_() const override;

    std::string Name;
    Type type;
};

/// BinaryExprAST - Expression class for a binary operator.
class BinaryExprAST : public ExprAST {
public:
    BinaryExprAST(CodegenContext& ctx, size_t gid, std::string Op, Expr LHS, Expr RHS, fileloc pos)
        : ExprAST(ctx, gid, pos)
        , opcode(Op), LHS(LHS), RHS(RHS)
    {}

    llvm::Value* codegen() const override;

protected:
    virtual Type Typecheck_() const override;

    std::string opcode;
    Expr LHS, RHS;
    mutable MatchCandidateEntry BestOverload;

};

/// ReturnExprAST - Represents a value return expression
class ReturnExprAST : public ExprAST {
public:
    ReturnExprAST(CodegenContext& ctx, size_t gid, Expr expr, fileloc pos)
        : ExprAST(ctx, gid, pos)
        , Expression(expr)
    {}

    llvm::Value* codegen() const override;

protected:
    virtual Type Typecheck_() const override;

    Expr Expression;
};

/// BlockAST - Represents a list of variable definitions and a list of
/// statements executed in a particular order
class BlockAST : public ExprAST {
public:
    BlockAST(CodegenContext& ctx, size_t gid, const std::list<Expr>& s, fileloc pos)
        : ExprAST(ctx, gid, pos)
        , Statements(s)
    {}

    llvm::Value* codegen() const override;

protected:
    virtual Type Typecheck_() const override;

    std::list<Expr> Statements;

    friend ForExprAST;
};

/// CallExprAST - Expression class for function calls.
class CallExprAST : public ExprAST {
public:
    CallExprAST(CodegenContext& ctx, size_t gid, const std::string &Callee,
                std::vector<Expr> Args, fileloc pos)
        : ExprAST(ctx, gid, pos)
        , Callee(Callee), Args(std::move(Args))
    {}

    llvm::Value* codegen() const override;

protected:
    virtual Type Typecheck_() const override;
    mutable llvm::Function* BestOverload;

    std::string Callee;
    std::vector<Expr> Args;
};

/// IfExprAST - Expression class for if/then/else.
class IfExprAST : public ExprAST {
public:
    IfExprAST(CodegenContext& ctx, size_t gid, Expr Cond, Expr Then, Expr Else, fileloc pos)
        : ExprAST(ctx, gid, pos)
        , Cond(Cond), Then(Then), Else(Else)
    {}

    llvm::Value* codegen() const override;

protected:
    virtual Type Typecheck_() const override;

    Expr Cond, Then, Else;
};

/// WhileExprAST - Expression class for while.
class WhileExprAST : public ExprAST {
public:
    WhileExprAST(CodegenContext& ctx, size_t gid, Expr Cond, Expr Body, fileloc pos)
        : ExprAST(ctx, gid, pos)
        , Cond(Cond), Body(Body)
    {}

    llvm::Value* codegen() const override;

protected:
    virtual Type Typecheck_() const override;

    Expr Cond, Body;
};

/// ForExprAST - Expression class for for.
class ForExprAST : public ExprAST {
public:
    ForExprAST(CodegenContext& ctx, size_t gid, Expr Init,
               Expr Cond, std::list<Expr> Step, Expr Body, fileloc pos)
        : ExprAST(ctx, gid, pos)
        , Init(Init), Cond(Cond),
          Step(Step), Body(Body)
    {}

    llvm::Value* codegen() const override;

protected:
    virtual Type Typecheck_() const override;

    Expr Init, Cond;
    std::list<Expr> Step;
    Expr Body;
};

class LoopControlStmtAST : public ExprAST {
public:
    LoopControlStmtAST(CodegenContext& ctx, size_t gid, loopControl which, fileloc pos)
        : ExprAST(ctx, gid, pos)
        , which(which)
    {}

    llvm::Value* codegen() const override;

protected:
    virtual Type Typecheck_() const override;

    loopControl which;
};

// --------------------------------------------------------------------------------

/// PrototypeAST - This class represents the "prototype" for a function,
/// which captures its name, and its argument names (thus implicitly the number
/// of arguments the function takes).
class PrototypeAST : public ExprAST {
public:
    PrototypeAST(CodegenContext& ctx, size_t gid, const std::string &Name,
                 std::vector<std::pair<std::string, Type>> Args, Type ReturnType, fileloc pos)
        : ExprAST(ctx, gid, pos)
        , Name(Name), Args(std::move(Args)), ReturnType(ReturnType)
    {}

    const std::string &getName() const { return Name; }
    Type getReturnType() const { return ReturnType; }
    const std::vector<std::pair<std::string, Type>>& getArgs() const { return Args; }
    llvm::Function* codegen() const override;
    std::vector<Type> GetSignature() const;
    Type GetReturnType() const {return ReturnType;}

protected:
    virtual Type Typecheck_() const override;

    std::string Name;
    std::vector<std::pair<std::string, Type>> Args;
    Type ReturnType;

    friend class FunctionAST;
};

/// FunctionAST - This class represents a function definition itself.
class FunctionAST : public ExprAST {
public:
    FunctionAST(CodegenContext& ctx, size_t gid, Prototype Proto, Expr Body, fileloc pos)
        : ExprAST(ctx, gid, pos)
        , Proto(Proto), Body(Body)
    {}

    llvm::Function* codegen() const override;

protected:
    virtual Type Typecheck_() const override;

    Prototype Proto;
    Expr Body;
};

class ConvertAST : public ExprAST {
public:
    ConvertAST(CodegenContext& ctx, size_t gid, Expr Expression, Type ResultingType, std::function<llvm::Value*(llvm::Value*)> Converter, fileloc pos)
        : ExprAST(ctx, gid, pos)
        , Expression(Expression)
        , ResultingType(ResultingType)
        , Converter(Converter)
    {}

    llvm::Value* codegen() const override;

protected:
    virtual Type Typecheck_() const override;

    Expr Expression;
    Type ResultingType;
    std::function<llvm::Value*(llvm::Value*)> Converter;
};


// I really hate having to add this operator, but maphoon insists on printing all token arguments...
inline std::ostream& operator<<(std::ostream& s, const std::list<Expr>& l){
    s << "A list of " << l.size() << " statements." << std::endl;
    return s;
}
inline std::ostream& operator<<(std::ostream& s, const std::list<std::pair<std::string,Type>>& l){
    s << "A list of " << l.size() << " function arguments." << std::endl;
    return s;
}
inline std::ostream& operator<<(std::ostream& s, const std::pair<std::string,Type>& l){
    s << "Identifier " << l.first << "and its type." << std::endl;
    return s;
}

inline std::ostream& operator<<(std::ostream& s, const std::vector<Expr>& l){
    s << "List of  " << l.size() << " arguments for a function call." << std::endl;
    return s;
}

inline std::ostream& operator<<(std::ostream& s, const Expr& l){
    s << "Expression" << std::endl;
    return s;
}

inline std::ostream& operator<<(std::ostream& s, const Type& l){
    s << "Type" << std::endl;
    return s;
}

} // namespace ccoscope

#endif // __TREE_H__
