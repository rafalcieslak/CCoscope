#ifndef __TYPES_H__
#define __TYPES_H__

#include <typeinfo>
#include <vector>
#include <string>

#include "llvm/IR/Module.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/TypeBuilder.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

#include "proxy.h"
#include "cast.h"

namespace ccoscope {

class CodegenContext; // forward declaration needed

class TypeAST;            using Type            = Proxy<TypeAST>;
class PrimitiveTypeAST;   using PrimitiveType   = Proxy<PrimitiveTypeAST>;
class VoidTypeAST;        using VoidType        = Proxy<VoidTypeAST>;
class ArithmeticTypeAST;  using ArithmeticType  = Proxy<ArithmeticTypeAST>;
class IntegerTypeAST;     using IntegerType     = Proxy<IntegerTypeAST>;
class DoubleTypeAST;      using DoubleType      = Proxy<DoubleTypeAST>;
class BooleanTypeAST;     using BooleanType     = Proxy<BooleanTypeAST>;
class FunctionTypeAST;    using FunctionType    = Proxy<FunctionTypeAST>;
class ReferenceTypeAST;   using ReferenceType   = Proxy<ReferenceTypeAST>;

Type str2type (CodegenContext& ctx, std::string s);

class TypeAST : public MagicCast<TypeAST> { 
public:
    TypeAST(CodegenContext& ctx, size_t gid, std::vector<Type> operands)
        : ctx_(ctx)
        , gid_(gid)
        , operands_(operands)
        , representative_(this)
    {}
    virtual ~TypeAST() {}
    
    virtual bool equal (const TypeAST& other) const;
    virtual llvm::Type* toLLVMs () const;
    virtual llvm::Value* defaultLLVMsValue () const;
    
    Type operand(size_t i) const { return operands_[i]; }
    size_t size() const { return operands_.size(); }
    
    CodegenContext& ctx () const { return ctx_; }
    size_t gid () const { return gid_; }
    bool is_proxy () const { return representative_ != this; }
    
    bool operator < (const TypeAST& other) const { return gid() < other.gid(); }
    
protected:
    CodegenContext& ctx_;
    size_t gid_;
    std::vector<Type> operands_;
    mutable const TypeAST* representative_;
    
    template<class T> friend class Proxy;
};

class PrimitiveTypeAST : public TypeAST {
public:
    PrimitiveTypeAST(CodegenContext& ctx, size_t gid)
        : TypeAST(ctx, gid, {})
    {}
};

class VoidTypeAST : public PrimitiveTypeAST {
public:
    VoidTypeAST(CodegenContext& ctx, size_t gid)
        : PrimitiveTypeAST(ctx, gid)
    {}
    
    llvm::Type* toLLVMs () const override;
};

class ArithmeticTypeAST : public PrimitiveTypeAST {
public:
    ArithmeticTypeAST(CodegenContext& ctx, size_t gid)
        : PrimitiveTypeAST(ctx, gid)
    {}
};

class IntegerTypeAST : public ArithmeticTypeAST {
public:
    IntegerTypeAST(CodegenContext& ctx, size_t gid)
        : ArithmeticTypeAST(ctx, gid)
    {}
    
    llvm::Type* toLLVMs () const override;
    llvm::Value* defaultLLVMsValue () const;
};

class DoubleTypeAST : public ArithmeticTypeAST {
public:
    DoubleTypeAST(CodegenContext& ctx, size_t gid)
        : ArithmeticTypeAST(ctx, gid)
    {}
    
    llvm::Type* toLLVMs () const override;
    llvm::Value* defaultLLVMsValue () const;
};

class BooleanTypeAST : public PrimitiveTypeAST {
public:
    BooleanTypeAST(CodegenContext& ctx, size_t gid)
        : PrimitiveTypeAST(ctx, gid)
    {}
    
    llvm::Type* toLLVMs () const override;
    llvm::Value* defaultLLVMsValue () const;
};

class FunctionTypeAST : public TypeAST {
public:
    FunctionTypeAST(CodegenContext& ctx, size_t gid, 
                    Type ret, std::vector<Type> args)
        : TypeAST(ctx, gid, {})
    {
        args.insert(args.begin(), ret);
        operands_ = args;
    }
    
    bool equal (const TypeAST& other) const override;
    llvm::FunctionType* toLLVMs () const override;
    
    Type returnType () const { return operand(0); }
    // Mind you -- one-based argument list
    Type argument (size_t i) const { return operand(i); }
};

class ReferenceTypeAST : public TypeAST {
public:
    ReferenceTypeAST(CodegenContext& ctx, size_t gid, Type of)
        : TypeAST(ctx, gid, {of})
    {}
    
    bool equal (const TypeAST& other) const override;
    llvm::Type* toLLVMs () const override;
    llvm::Value* defaultLLVMsValue () const;
    
    Type of () const { return operand(0); }
};

}

#endif
