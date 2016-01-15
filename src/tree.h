#ifndef __TREE_H__
#define __TREE_H__

#include <string>
#include <memory>
#include <vector>
#include <cassert>
#include <list>

/// ExprAST - Base class for all expression nodes.
class ExprAST {
 public:
    virtual ~ExprAST() {}
};

/// NumberExprAST - Expression class for numeric literals like "1.0".
class NumberExprAST : public ExprAST {
    double Val;

 public:
 NumberExprAST(double Val) : Val(Val) {}
};

/// VariableExprAST - Expression class for referencing a variable, like "a".
class VariableExprAST : public ExprAST {
    std::string Name;

public:
    VariableExprAST(const std::string &Name) : Name(Name) {}
};

/// BinaryExprAST - Expression class for a binary operator.
class BinaryExprAST : public ExprAST {
    std::string Opcode;
    std::shared_ptr<ExprAST> LHS, RHS;

public:
    BinaryExprAST(std::string Op, std::shared_ptr<ExprAST> LHS,
                  std::shared_ptr<ExprAST> RHS)
        : Opcode(Op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}
};

/// ReturnExprAST - Represents a value return expression
class ReturnExprAST : public ExprAST {
    std::shared_ptr<ExprAST> Expr;

public:
    ReturnExprAST(std::shared_ptr<ExprAST> expr)
        : Expr(std::move(expr)) {}
};

/// StatementListAST - Represents a list of statements executed in order
class StatementListAST : public ExprAST {
    std::list<std::shared_ptr<ExprAST>> Statements;

public:
    StatementListAST(){}
    void Prepend(std::shared_ptr<ExprAST> t){
        Statements.push_front(t);
    }
};



/// PrototypeAST - This class represents the "prototype" for a function,
/// which captures its name, and its argument names (thus implicitly the number
/// of arguments the function takes).
class PrototypeAST {
    std::string Name;
    std::vector<std::string> Args;

 public:
 PrototypeAST(const std::string &Name, std::vector<std::string> Args)
     : Name(Name), Args(std::move(Args)) {}
    //Function *codegen();
    const std::string &getName() const { return Name; }
};

/// FunctionAST - This class represents a function definition itself.
class FunctionAST {
    std::shared_ptr<PrototypeAST> Proto;
    std::shared_ptr<ExprAST> Body;

 public:
 FunctionAST(std::shared_ptr<PrototypeAST> Proto,
             std::shared_ptr<ExprAST> Body)
     : Proto(std::move(Proto)), Body(std::move(Body)) {}
    //Function *codegen();
};

#endif // __TREE_H__
