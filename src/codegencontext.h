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
#include "typematcher.h"

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

template<typename T>
struct GIDCmp {
    bool operator () (const T& lhs, const T& rhs) const {
        return lhs.gid() < rhs.gid();
    }
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
    ComplexValue makeComplex(Expr re, Expr im);
    BinaryExpr makeBinary(std::string Op, Expr LHS, Expr RHS);
    ReturnExpr makeReturn(Expr expr);
    Block makeBlock(const std::vector<std::pair<std::string, Type>> &vars,
                    const std::list<Expr>& s);
    CallExpr makeCall(const std::string &Callee, std::vector<Expr> Args);
    IfExpr makeIf(Expr Cond, Expr Then, Expr Else);
    WhileExpr makeWhile(Expr Cond, Expr Body);
    ForExpr makeFor(Expr Init, Expr Cond, std::list<Expr> Step, Expr Body);
    LoopControlStmt makeLoopControlStmt(loopControl which);
    Prototype makePrototype(const std::string &Name,
        std::vector<std::pair<std::string, Type>> Args, Type ReturnType);
    Function makeFunction(Prototype Proto, Expr Body);
    Convert makeConvert(Expr Expression, Type ResultingType, std::function<llvm::Value*(llvm::Value*)> Converter);
    // ==---------------------------------------------------------------
    
    // ==---------------------------------------------------------------
    // Factory methods for Types

    VoidType getVoidTy();
    IntegerType getIntegerTy();
    DoubleType getDoubleTy();
    BooleanType getBooleanTy();
    ComplexType getComplexTy();
    FunctionType getFunctionTy(Type ret, std::vector<Type> args);
    ReferenceType getReferenceTy(Type of);
    // ==---------------------------------------------------------------
    
    llvm::Module* TheModule () const { return theModule_.get(); }
    llvm::IRBuilder<> Builder () const { return builder_; }
    llvm::Function* CurrentFunc () const { return currentFunc_; }
    Type CurrentFuncReturnType () const { return currentFuncReturnType_; }
    
    bool IsVarInScope (std::string s) const { return varsInScope_.find(s) != varsInScope_.end(); }
    std::pair<llvm::AllocaInst*, Type> GetVarInfo (std::string s) const { return varsInScope_[s]; }
    void SetVarInfo (std::string s, std::pair<llvm::AllocaInst*, Type> info) const { varsInScope_[s] = info; }
    bool RemoveVarInfo (std::string s) const { auto it = varsInScope_.find(s); if(it != varsInScope_.end()) { varsInScope_.erase(it); return true; } return false; } 
    void ClearVarsInfo () const { varsInScope_.clear(); }
    
    void PutLoopInfo (llvm::BasicBlock* headerBB, llvm::BasicBlock* postBB) const { loopsBBHeaderPost_.push_back({headerBB, postBB}); }
    void PopLoopInfo () const { loopsBBHeaderPost_.pop_back(); }
    bool IsInsideLoop () const { return !loopsBBHeaderPost_.empty(); }
    std::pair<llvm::BasicBlock*, llvm::BasicBlock*> GetCurrentLoopInfo () const { return loopsBBHeaderPost_.back(); }
    
    void SetCurrentFunc (llvm::Function* f) const { currentFunc_ = f; }
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

    // Stores an error-message. TODO: Add file positions storage.
    void AddError(std::string text) const;
    // Returns true iff no errors were stored.
    bool IsErrorFree();
    // Prints all stored errors to stdout.
    void DisplayErrors();
    
    TypeMatcher typematcher;
    
protected:
    std::shared_ptr<llvm::Module> theModule_;
    mutable llvm::IRBuilder<> builder_;
    mutable llvm::Function* currentFunc_;
    mutable Type currentFuncReturnType_;
    mutable std::map<std::string, std::pair<llvm::AllocaInst*, Type>> varsInScope_;

    std::map<std::string, std::list<MatchCandidateEntry>> availableBinOps_;
    std::map<std::pair<std::string, MatchCandidateEntry>, std::function<llvm::Value*(std::vector<llvm::Value*>)>> binOpCreator_;

    // For tracking in which loop we are currently in
    // .first -- headerBB, .second -- postBB
    mutable std::list<std::pair<llvm::BasicBlock*, llvm::BasicBlock*>> loopsBBHeaderPost_;

    // Storage for error messages.
    mutable std::list<std::pair<std::string, std::string>> errors_;

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
