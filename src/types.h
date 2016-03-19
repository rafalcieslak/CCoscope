#ifndef __TYPES_H__
#define __TYPES_H__

#include "llvm/IR/Module.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/TypeBuilder.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

#include "proxy.h"
#include "conversions.h"

#include <typeinfo>
#include <vector>
#include <list>
#include <string>
#include <unordered_set>

namespace ccoscope {

class CodegenContext; // forward declaration needed

class TypeAST;            using Type            = Proxy<TypeAST>;
class PrimitiveTypeAST;   using PrimitiveType   = Proxy<PrimitiveTypeAST>;
class VoidTypeAST;        using VoidType        = Proxy<VoidTypeAST>;
class ArithmeticTypeAST;  using ArithmeticType  = Proxy<ArithmeticTypeAST>;
class IntegerTypeAST;     using IntegerType     = Proxy<IntegerTypeAST>;
class DoubleTypeAST;      using DoubleType      = Proxy<DoubleTypeAST>;
class BooleanTypeAST;     using BooleanType     = Proxy<BooleanTypeAST>;
class ComplexTypeAST;     using ComplexType     = Proxy<ComplexTypeAST>;
class FunctionTypeAST;    using FunctionType    = Proxy<FunctionTypeAST>;
class ReferenceTypeAST;   using ReferenceType   = Proxy<ReferenceTypeAST>;

Type str2type (const CodegenContext& ctx, std::string s);

class TypeAST : public MagicCast<TypeAST> {
public:
    TypeAST(const CodegenContext& ctx, size_t gid, std::vector<Type> operands)
        : ctx_(ctx)
        , gid_(gid)
        , operands_(operands)
        , representative_(this)
        , cache_(nullptr)
    {}
    virtual ~TypeAST() {}

    virtual bool equal (const TypeAST& other) const;
    virtual llvm::Type* toLLVMs () const;
    virtual llvm::Value* defaultLLVMsValue () const;

    Type operand(size_t i) const { return operands_[i]; }
    size_t size() const { return operands_.size(); }

    const CodegenContext& ctx () const { return ctx_; }
    size_t gid () const { return gid_; }
    bool is_proxy () const { return representative_ != this; }

    bool operator < (const TypeAST& other) const { return gid() < other.gid(); }
    virtual std::string name() const {return "NoType";}

    virtual std::list<Conversion> ListConversions() const {
        return std::list<Conversion>();
    };

protected:
    virtual llvm::Type* toLLVMs_ () const;

    const CodegenContext& ctx_;
    size_t gid_;
    std::vector<Type> operands_;
    mutable const TypeAST* representative_;
    mutable llvm::Type* cache_;

    template<class T> friend class Proxy;
};

class PrimitiveTypeAST : public TypeAST {
public:
    PrimitiveTypeAST(const CodegenContext& ctx, size_t gid)
        : TypeAST(ctx, gid, {})
    {}

    virtual std::string name() const {return "Primitive";}
};

class VoidTypeAST : public PrimitiveTypeAST {
public:
    VoidTypeAST(const CodegenContext& ctx, size_t gid)
        : PrimitiveTypeAST(ctx, gid)
    {}

    virtual std::string name() const {return "Void";}

protected: 
    llvm::Type* toLLVMs_ () const override;
};

class ArithmeticTypeAST : public PrimitiveTypeAST {
public:
    ArithmeticTypeAST(const CodegenContext& ctx, size_t gid)
        : PrimitiveTypeAST(ctx, gid)
    {}

    virtual std::string name() const {return "Arihmetic";}
};

class IntegerTypeAST : public ArithmeticTypeAST {
public:
    IntegerTypeAST(const CodegenContext& ctx, size_t gid)
        : ArithmeticTypeAST(ctx, gid)
    {}

    virtual std::string name() const {return "Integer";}
    llvm::Value* defaultLLVMsValue () const;

    virtual std::list<Conversion> ListConversions() const override;

protected:
    llvm::Type* toLLVMs_ () const override;
};

class DoubleTypeAST : public ArithmeticTypeAST {
public:
    DoubleTypeAST(const CodegenContext& ctx, size_t gid)
        : ArithmeticTypeAST(ctx, gid)
    {}

    virtual std::string name() const {return "Double";}
    llvm::Value* defaultLLVMsValue () const;

    virtual std::list<Conversion> ListConversions() const override;

protected:
    llvm::Type* toLLVMs_ () const override;
};

class BooleanTypeAST : public PrimitiveTypeAST {
public:
    BooleanTypeAST(const CodegenContext& ctx, size_t gid)
        : PrimitiveTypeAST(ctx, gid)
    {}

    virtual std::string name() const {return "Boolean";}
    llvm::Value* defaultLLVMsValue () const;

protected:
    llvm::Type* toLLVMs_ () const override;
};

class ComplexTypeAST : public ArithmeticTypeAST {
public:
    ComplexTypeAST(const CodegenContext& ctx, size_t gid)
        : ArithmeticTypeAST(ctx, gid)
    {}

    virtual std::string name() const {return "Complex";}
    llvm::StructType* toLLVMs () const override;
    llvm::Value* defaultLLVMsValue () const;

protected:
    llvm::StructType* toLLVMs_ () const override;
};

class FunctionTypeAST : public TypeAST {
public:
    FunctionTypeAST(const CodegenContext& ctx, size_t gid,
                    Type ret, std::vector<Type> args)
        : TypeAST(ctx, gid, {})
    {
        args.insert(args.begin(), ret);
        operands_ = args;
    }

    virtual std::string name() const;
    bool equal (const TypeAST& other) const override;
    llvm::FunctionType* toLLVMs () const override;

    Type returnType () const { return operand(0); }
    // Mind you -- one-based argument list
    Type argument (size_t i) const { return operand(i); }

protected:
    llvm::FunctionType* toLLVMs_ () const override;
};

class ReferenceTypeAST : public TypeAST {
public:
    ReferenceTypeAST(const CodegenContext& ctx, size_t gid, Type of)
        : TypeAST(ctx, gid, {of})
    {}

    virtual std::string name() const {return "Ref(" + of()->name() + ")";}
    bool equal (const TypeAST& other) const override;
    llvm::Value* defaultLLVMsValue () const;

    Type of () const { return operand(0); }

    virtual std::list<Conversion> ListConversions() const override;

protected:
    llvm::Type* toLLVMs_ () const override;
};


// naiive for now
struct TypeHash { size_t operator () (const TypeAST* t) const { return 1; } };
struct TypeEqual {
    bool operator () (const TypeAST* t1, const TypeAST* t2) const {
        return t1->equal(*t2);
    }
};

using TypeSet = std::unordered_set<const TypeAST*, TypeHash, TypeEqual>;

struct TypeCmp {
    bool operator () (const Type& lhs,
                      const Type& rhs) const;
};


}

#endif
