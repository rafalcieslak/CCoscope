#ifndef __TREE_H__
#define __TREE_H__

#include <string>
#include <memory>
#include <vector>
#include <cassert>
#include <list>
#include <iostream>

enum datatype{
    DATATYPE_int,
    DATATYPE_float,
    DATATYPE_bool,
    DATATYPE_void, // Not available for variables, but can be returned by a function
};

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

/// BlockAST - Represents a list of variable definitions and a list of
/// statements executed in a particular order
class BlockAST : public ExprAST {
    std::list<std::shared_ptr<ExprAST>> Statements;

public:
    BlockAST(const std::list<std::shared_ptr<ExprAST>>& s) : Statements(s) {}
};

/// AssignmentAST - Represents an assignment operations
class AssignmentAST : public ExprAST {
    std::string Name;
    std::shared_ptr<ExprAST> Expr;
public:
    AssignmentAST(const std::string& Name, std::shared_ptr<ExprAST> Expr) :
        Name(Name), Expr(Expr) {}
};

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

#endif // __TREE_H__
