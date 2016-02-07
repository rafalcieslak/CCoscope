// -*- mode: c++; fill-column: 80 -*-
#ifndef __TREE_H__
#define __TREE_H__

#include "codegencontext.h"
#include "typechecking.h"

#include <string>
#include <memory>
#include <vector>
#include <cassert>
#include <list>
#include <iostream>

using namespace llvm;

enum class keyword {
    Break,
    Continue
};

// Forward declaration for BlockAST <-> ForExprAST dependency
class ForExprAST;
class ExprAST; 

using ExprType = std::pair<std::shared_ptr<ExprAST>, datatype/*CCType*/>;

/// ExprAST - Base class for all expression nodes.
class ExprAST {
public:
    virtual ~ExprAST() {}
    virtual Value* codegen(CodegenContext& ctx) const = 0;
    virtual ExprType maintype(CodegenContext& ctx) const;

};

/// PrimitiveExprAST - Expression class for numeric literals like "1.0"
/// as well as boolean constants
template<typename T>
class PrimitiveExprAST : public ExprAST {
protected:
    T Val;

public:
    PrimitiveExprAST(T v) : Val(v) {}
    
    Value* codegen(CodegenContext& ctx) const override;
    virtual ExprType maintype (CodegenContext& ctx) const override;
};

// For some reason putting the implementation below to the .cpp file
// yields a compiler error.
// TODO: understand why :)
template<typename T>
ExprType PrimitiveExprAST<T>::maintype(CodegenContext& ctx) const {
    return {std::make_shared<PrimitiveExprAST<int>>(42), DATATYPE_void};//CCVoidType()};
}

/// VariableExprAST - Expression class for referencing a variable, like "a".
class VariableExprAST : public ExprAST {
    std::string Name;

public:
    VariableExprAST(const std::string &Name) : Name(Name) {}
    
    Value* codegen(CodegenContext& ctx) const override;
    virtual ExprType maintype (CodegenContext& ctx) const override;
};

/// BinaryExprAST - Expression class for a binary operator.
class BinaryExprAST : public ExprAST {
    std::string Opcode;
    std::shared_ptr<ExprAST> LHS, RHS;

public:
    BinaryExprAST(std::string Op, std::shared_ptr<ExprAST> LHS,
                  std::shared_ptr<ExprAST> RHS)
        : Opcode(Op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}
    
    Value* codegen(CodegenContext& ctx) const override;
    virtual ExprType maintype (CodegenContext& ctx) const override;
};

/// ReturnExprAST - Represents a value return expression
class ReturnExprAST : public ExprAST {
    std::shared_ptr<ExprAST> Expr;

public:
    ReturnExprAST(std::shared_ptr<ExprAST> expr)
        : Expr(std::move(expr)) {}
    Value* codegen(CodegenContext& ctx) const override;
};

/// BlockAST - Represents a list of variable definitions and a list of
/// statements executed in a particular order
class BlockAST : public ExprAST {
    class ScopeManager {
    public:
        ScopeManager(const BlockAST* parent, CodegenContext& ctx)
            : parent(parent)
            , ctx(ctx)
        {}
        
        ~ScopeManager() {
            for (auto& var : parent->Vars) {
                auto it = std::find_if(ctx.VarsInScope.begin(),
                                       ctx.VarsInScope.end(),
                                       [&var](auto& p) {
                                           return p.first == var.first;
                                       });
                ctx.VarsInScope.erase(it);
            }
        }
        
    protected:
        const BlockAST* parent;
        CodegenContext& ctx;
    };
    
    std::vector<std::pair<std::string,datatype>> Vars;
    std::list<std::shared_ptr<ExprAST>> Statements;

public:
    BlockAST(const std::vector<std::pair<std::string,datatype>> &vars, 
             const std::list<std::shared_ptr<ExprAST>>& s)
        : Vars(vars), Statements(s) {}
    Value* codegen(CodegenContext& ctx) const override;
    
    friend ForExprAST;
};

/// AssignmentAST - Represents an assignment operations
class AssignmentAST : public ExprAST {
    std::string Name;
    std::shared_ptr<ExprAST> Expr;
public:
    AssignmentAST(const std::string& Name, std::shared_ptr<ExprAST> Expr) :
        Name(Name), Expr(Expr) {}
    Value* codegen(CodegenContext& ctx) const override;
    virtual ExprType maintype (CodegenContext& ctx) const override;
};

/// CallExprAST - Expression class for function calls.
class CallExprAST : public ExprAST {
    std::string Callee;
    std::vector<std::shared_ptr<ExprAST>> Args;

public:
    CallExprAST(const std::string &Callee,
                std::vector<std::shared_ptr<ExprAST>> Args)
        : Callee(Callee), Args(std::move(Args)) {}
    Value* codegen(CodegenContext& ctx) const override;
    virtual ExprType maintype (CodegenContext& ctx) const override;
};

/// IfExprAST - Expression class for if/then/else.
class IfExprAST : public ExprAST {
    std::shared_ptr<ExprAST> Cond, Then, Else;

public:
    IfExprAST(std::shared_ptr<ExprAST> Cond, std::shared_ptr<ExprAST> Then,
              std::shared_ptr<ExprAST> Else)
        : Cond(std::move(Cond)), Then(std::move(Then)), Else(std::move(Else)) {}
    Value* codegen(CodegenContext& ctx) const override;
};

/// WhileExprAST - Expression class for while.
class WhileExprAST : public ExprAST {
    std::shared_ptr<ExprAST> Cond, Body;

public:
    WhileExprAST(std::shared_ptr<ExprAST> Cond,
                 std::shared_ptr<ExprAST> Body)
        : Cond(std::move(Cond)), Body(std::move(Body)) {}
    Value* codegen(CodegenContext& ctx) const override;
};

/// ForExprAST - Expression class for for.
class ForExprAST : public ExprAST {
    std::shared_ptr<ExprAST> Init, Cond;
    std::list<std::shared_ptr<ExprAST>> Step;
    std::shared_ptr<ExprAST> Body;

public:
    ForExprAST(std::shared_ptr<ExprAST> Init,
                 std::shared_ptr<ExprAST> Cond,
                 std::list<std::shared_ptr<ExprAST>> Step,
                 std::shared_ptr<ExprAST> Body)
        : Init(std::move(Init)), Cond(std::move(Cond)),
          Step(std::move(Step)), Body(std::move(Body)) {}
    Value* codegen(CodegenContext& ctx) const override;
};

class KeywordAST : public ExprAST {
    keyword which;
    
public:
    KeywordAST(keyword which) : which(which) {}
    Value* codegen(CodegenContext& ctx) const override;
};

// --------------------------------------------------------------------------------

/// PrototypeAST - This class represents the "prototype" for a function,
/// which captures its name, and its argument names (thus implicitly the number
/// of arguments the function takes).
class PrototypeAST {
    std::string Name;
    std::vector<std::pair<std::string, datatype>> Args;
    datatype ReturnType;

public:
    PrototypeAST(const std::string &Name, std::vector<std::pair<std::string, datatype>> Args, datatype ReturnType)
        : Name(Name), Args(std::move(Args)), ReturnType(ReturnType) {}
    const std::string &getName() const { return Name; }
    const std::vector<std::pair<std::string, datatype>>& getArgs() const { return Args; }
    Function* codegen(CodegenContext& ctx) const;
    virtual ExprType maintype (CodegenContext& ctx) const;
};

/// FunctionAST - This class represents a function definition itself.
class FunctionAST {
    std::shared_ptr<PrototypeAST> Proto;
    std::shared_ptr<ExprAST> Body;

public:
    FunctionAST(std::shared_ptr<PrototypeAST> Proto,
                std::shared_ptr<ExprAST> Body)
        : Proto(std::move(Proto)), Body(std::move(Body)) {}
    Function* codegen(CodegenContext& ctx) const;
    virtual ExprType maintype (CodegenContext& ctx) const;
};


// I really hate having to add this operator, but maphoon insists on printing all token arguments...
inline std::ostream& operator<<(std::ostream& s, const std::list<std::shared_ptr<ExprAST>>& l){
    s << "A list of " << l.size() << " statements." << std::endl;
    return s;
}
inline std::ostream& operator<<(std::ostream& s, const std::vector<std::pair<std::string,datatype>>& l){
    s << "A list of " << l.size() << " function arguments." << std::endl;
    return s;
}
inline std::ostream& operator<<(std::ostream& s, const std::pair<std::string,datatype>& l){
    s << "Identifier " << l.first << "and its type." << std::endl;
    return s;
}
inline std::ostream& operator<<(std::ostream& s, const std::vector<std::shared_ptr<ExprAST>>& l){
    s << "List of  " << l.size() << " arguments for a function call." << std::endl;
    return s;
}

#endif // __TREE_H__
