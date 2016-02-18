// -*- mode: c++; fill-column: 80 -*-
#ifndef __CODEGEN_CONTEXT_H__
#define __CODEGEN_CONTEXT_H__

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

#include <memory>
#include <unordered_set>
#include <list>

#include "tree.h"
#include "types.h"

namespace ccoscope {

class CodegenContext;

/// CreateEntryBlockAlloca - Create an alloca instruction in the entry block of
/// the function.  This is used for mutable variables etc.
static llvm::AllocaInst *CreateEntryBlockAlloca(llvm::Function *TheFunction,
                                          const std::string &VarName,
                                          llvm::Type* type) {
  llvm::IRBuilder<> TmpB(&TheFunction->getEntryBlock(),
                 TheFunction->getEntryBlock().begin());
  return TmpB.CreateAlloca(type, 0, (VarName + "_addr").c_str());
}

llvm::Constant* CreateI8String(char const* str, CodegenContext& ctx);

template<class T>
struct GIDHash {
    size_t operator () (T n) const { return n->gid(); }
};

template<class T>
struct GIDEq {
    size_t operator () (T n1, T n2) const { return n1->gid() == n2->gid(); }
};

template<typename T>
using GIDSet = std::unordered_set<const T*, GIDHash<const T*>, GIDEq<const T*>>;

template<typename T>
struct GIDCmp {
    bool operator () (const T& lhs, const T& rhs) const {
        return lhs.gid() < rhs.gid();
    }
};

// naiive for now
struct TypeHash { size_t operator () (const TypeAST* t) const { return 1; } };
struct TypeEqual {
    bool operator () (const TypeAST* t1, const TypeAST* t2) const {
        return t1->equal(*t2);
    }
};

using TypeSet = std::unordered_set<const TypeAST*, TypeHash, TypeEqual>;

struct TTypeCmp {
    bool operator () (const std::tuple<std::string, Type, Type>& lhs,
                      const std::tuple<std::string, Type, Type>& rhs) const;
};

class CodegenContext
{
public:
    CodegenContext();
    ~CodegenContext();

    // ==---------------------------------------------------------------
    // Factory methods for AST nodes

    VariableExpr makeVariable(std::string name);
    PrimitiveExpr<int> makeInt(int value);
    PrimitiveExpr<double> makeDouble(double value);
    PrimitiveExpr<bool> makeBool(bool value);
    BinaryExpr makeBinary(std::string Op, Expr LHS, Expr RHS);
    ReturnExpr makeReturn(Expr expr);
    Block makeBlock(const std::vector<std::pair<std::string, Type>> &vars,
                    const std::list<Expr>& s);
    Assignment makeAssignment(const std::string& Name, Expr expr);
    CallExpr makeCall(const std::string &Callee, std::vector<Expr> Args);
    IfExpr makeIf(Expr Cond, Expr Then, Expr Else);
    WhileExpr makeWhile(Expr Cond, Expr Body);
    ForExpr makeFor(Expr Init, Expr Cond, std::list<Expr> Step, Expr Body);
    Keyword makeKeyword(keyword which);
    Prototype makePrototype(const std::string &Name,
        std::vector<std::pair<std::string, Type>> Args, Type ReturnType);
    Function makeFunction(Prototype Proto, Expr Body);

    // ==---------------------------------------------------------------

    // ==---------------------------------------------------------------
    // Factory methods for Types

    VoidType getVoidTy();
    IntegerType getIntegerTy();
    DoubleType getDoubleTy();
    BooleanType getBooleanTy();
    FunctionType getFunctionTy(Type ret, std::vector<Type> args);
    ReferenceType getReferenceTy(Type of);

    // ==---------------------------------------------------------------

    mutable GIDSet<FunctionAST> definitions;
    mutable GIDSet<PrototypeAST> prototypes;
    mutable std::map<std::string, Prototype> prototypesMap;
    mutable GIDSet<ExprAST> expressions;
    mutable TypeSet types;

    std::shared_ptr<llvm::Module> TheModule;
    llvm::IRBuilder<> Builder;
    llvm::Function* CurrentFunc;
    mutable std::map<std::string, std::pair<llvm::AllocaInst*, Type>> VarsInScope;

    std::map<std::tuple<std::string, Type, Type>,
            std::pair<std::function<llvm::Value*(llvm::Value*, llvm::Value*)>,
                      Type>,
            TTypeCmp> BinOpCreator;

    // For tracking in which loop we are currently in
    // .first -- headerBB, .second -- postBB
    mutable std::list<std::pair<llvm::BasicBlock*, llvm::BasicBlock*>> LoopsBBHeaderPost;

    bool is_inside_loop () const { return !LoopsBBHeaderPost.empty(); }

    // Special function handles
    llvm::Function* GetStdFunction(std::string name) const;

    void SetModuleAndFile(std::shared_ptr<llvm::Module> module, std::string infile);

    // Stores an error-message. TODO: Add file positions storage.
    void AddError(std::string text);

    // Returns true iff no errors were stored.
    bool IsErrorFree();

    // Prints all stored errors to stdout.
    void DisplayErrors();

protected:
    // Storage for error messages.
    mutable std::list<std::pair<std::string, std::string>> errors;

    // The map storing special function handlers, use GetStdFunctions to search for a handle.
    std::map<std::string, llvm::Function*> stdlib_functions;
    // Initializes the above map, called as soon as a module is set.
    void PrepareStdFunctionPrototypes();

    // The base input source file for this module
    std::string filename;

    // Is there a way to merge these two "introduces"? TODO: find a way!
    template<class T>
    const T* introduceE(const T* node) { return introduce_expr(node)->template as<T>(); }
    template<class T>
    const T* introduceT(const T* node) { return introduce_type(node)->template as<T>(); }
    const ExprAST* introduce_expr(const ExprAST*);
    const TypeAST* introduce_type(const TypeAST*);
    const PrototypeAST* introduce_prototype(const PrototypeAST*);
    const FunctionAST* introduce_function(const FunctionAST*);

    mutable size_t gid_;
};

}

#endif // __CODEGEN_CONTEXT_H__
