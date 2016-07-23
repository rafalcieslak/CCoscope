// -*- mode: c++; fill-column: 80 -*-
#ifndef __CODEGEN_CONTEXT_H__
#define __CODEGEN_CONTEXT_H__

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

#include <memory>
#include <unordered_set>
#include <list>
#include <deque>
#include "ast/ast.h"
#include "types/types.h"
#include "types/typematcher.h"

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

class CodegenContext
{
public:
    CodegenContext();
    ~CodegenContext();

    // ==---------------------------------------------------------------
    // Factory methods for AST nodes

    VariableOccExpr makeVariableOcc(std::string name, fileloc pos);
    VariableDeclExpr makeVariableDecl(std::string name, Type type, fileloc pos);
    PrimitiveExpr<int> makeInt(int value, fileloc pos);
    PrimitiveExpr<double> makeDouble(double value, fileloc pos);
    PrimitiveExpr<bool> makeBool(bool value, fileloc pos);
    ComplexValue makeComplex(Expr re, Expr im, fileloc pos);
    StringValue makeString(std::string s, fileloc pos);
    BinaryExpr makeBinary(std::string Op, Expr LHS, Expr RHS, fileloc pos);
    ReturnExpr makeReturn(Expr expr, fileloc pos);
    Block makeBlock(const std::list<Expr>& s, fileloc pos);
    CallExpr makeCall(const std::string& Callee, std::vector<Expr> Args, fileloc pos);
    IfExpr makeIf(Expr Cond, Expr Then, Expr Else, fileloc pos);
    WhileExpr makeWhile(Expr Cond, Expr Body, fileloc pos);
    ForExpr makeFor(Expr Init, Expr Cond, std::list<Expr> Step, Expr Body, fileloc pos);
    LoopControlStmt makeLoopControlStmt(loopControl which, fileloc pos);
    Prototype makePrototype(const std::string& Name,
        std::vector<std::pair<std::string, Type>> Args, Type ReturnType, fileloc pos);
    Function makeFunction(Prototype Proto, Expr Body, fileloc pos);
    Convert makeConvert(Expr Expression, Type ResultingType, std::function<llvm::Value*(llvm::Value*)> Converter, fileloc pos);
    // ==---------------------------------------------------------------

    // ==---------------------------------------------------------------
    // Factory methods for Types

    VoidType getVoidTy();
    IntegerType getIntegerTy();
    DoubleType getDoubleTy();
    BooleanType getBooleanTy();
    ComplexType getComplexTy();
    StringType getStringTy();
    FunctionType getFunctionTy(Type ret, std::vector<Type> args);
    ReferenceType getReferenceTy(Type of);
    // ==---------------------------------------------------------------

    llvm::Module* TheModule () const { return theModule_.get(); }
    llvm::IRBuilder<>& Builder () const { return builder_; }
    llvm::Function* CurrentFunc () const { return currentFunc_; }
    Type CurrentFuncReturnType () const { return currentFuncReturnType_; }

    void EnterScope () const { varsInScope_.push_back({}); }
    void CloseScope () const { varsInScope_.pop_back(); }
    bool IsVarInCurrentScope (std::string s) const { return varsInScope_.rbegin()->find(s) != varsInScope_.rbegin()->end(); }
    bool IsVarInSomeEnclosingScope (std::string s) const;
    std::pair<llvm::AllocaInst*, Type> GetVarInfo (std::string s);
    void SetVarInfo (std::string s, std::pair<llvm::AllocaInst*, Type> info) const { (*varsInScope_.rbegin())[s] = info; }
    void ClearVarsInfo () const { varsInScope_.clear(); }

    void PutLoopInfo (llvm::BasicBlock* headerBB, llvm::BasicBlock* postBB) const { loopsBBHeaderPost_.push_back({headerBB, postBB}); }
    void PopLoopInfo () const { loopsBBHeaderPost_.pop_back(); }
    bool IsInsideLoop () const { return !loopsBBHeaderPost_.empty(); }
    std::pair<llvm::BasicBlock*, llvm::BasicBlock*> GetCurrentLoopInfo () const { return loopsBBHeaderPost_.back(); }

    void SetCurrentFunc (llvm::Function* f) const { currentFunc_ = f; currentFuncName_ = f->getName(); }
    void SetCurrentFuncName (std::string name) const { currentFuncName_ = name; }
    void SetCurrentFuncReturnType (Type t) const { currentFuncReturnType_ = t; }

    int NumberOfPrototypes () const { return prototypes_.size(); }
    int NumberOfDefinitions () const { return definitions_.size(); }
    bool HasPrototype (std::string s) const { return prototypesMap_.find(s) != prototypesMap_.end(); }
    Prototype GetPrototype (std::string s) const { return prototypesMap_[s]; }

    bool IsBinOpAvailable (std::string opcode) const { return availableBinOps_.find(opcode) != availableBinOps_.end(); }
    std::list<MatchCandidateEntry> VariantsForBinOp (std::string opcode) { return availableBinOps_[opcode]; }

    // Special function handles
    llvm::Function* GetStdFunction(std::string name) const;
    std::string GetPrintFunctionName(Type type) ;

    void SetModuleAndFile(std::shared_ptr<llvm::Module> module, std::string infile);

    // Stores an error-message.
    void AddError(std::string text, fileloc loc = fileloc(-1,-1)) const;
    // Returns true iff no errors were stored.
    bool IsErrorFree();
    // Prints all stored errors to stdout.
    void DisplayErrors();

    TypeMatcher typematcher;

protected:
    std::shared_ptr<llvm::Module> theModule_;
    mutable llvm::IRBuilder<> builder_;
    mutable llvm::Function* currentFunc_; // Used during codegen phase
    mutable std::string currentFuncName_; // Used during both codegen and typechecking for error messages
    mutable Type currentFuncReturnType_; // Used during both codegen and typechecking
    mutable std::deque<std::map<std::string, std::pair<llvm::AllocaInst*, Type>>> varsInScope_;

    std::map<std::string, std::list<MatchCandidateEntry>> availableBinOps_;
    std::map<std::pair<std::string, MatchCandidateEntry>, std::function<llvm::Value*(std::vector<llvm::Value*>)>> binOpCreator_;

    // For tracking in which loop we are currently in
    // .first -- headerBB, .second -- postBB
    mutable std::list<std::pair<llvm::BasicBlock*, llvm::BasicBlock*>> loopsBBHeaderPost_;

    // Storage for error messages.
    mutable std::list<std::tuple<std::string, std::string, fileloc>> errors_;

    // The map storing special function handlers, use GetStdFunctions to search for a handle.
    std::map<std::string, llvm::Function*> stdlib_functions_;
    // Initializes the above map, called as soon as a module is set.
    void PrepareStdFunctionPrototypes_();

    // The base input source file for this module
    std::string filename_;

    // Is there a way to merge these two "introduces"? TODO: find a way!
    template<class T>
    const T* IntroduceE_(const T* node) const { return IntroduceExpr_(node)->template as<T>(); }
    template<class T>
    const T* IntroduceT_(const T* node) const { return IntroduceType_(node)->template as<T>(); }
    const ExprAST* IntroduceExpr_(const ExprAST*) const;
    const TypeAST* IntroduceType_(const TypeAST*) const;
    const PrototypeAST* IntroducePrototype_(const PrototypeAST*) const;
    const FunctionAST* IntroduceFunction_(const FunctionAST*) const;

    mutable GIDSet<FunctionAST> definitions_;
    mutable GIDSet<PrototypeAST> prototypes_;
    mutable std::map<std::string, Prototype> prototypesMap_;
    mutable GIDSet<ExprAST> expressions_;
    mutable TypeSet types_;
    mutable size_t gid_;

    friend class BinaryExprAST; // uses binOpCreator_
    friend int Compile(std::string infile, std::string outfile, unsigned int optlevel);
};

}

#endif // __CODEGEN_CONTEXT_H__
