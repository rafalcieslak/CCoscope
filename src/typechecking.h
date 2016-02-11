#ifndef __TYPECHECKING_H__
#define __TYPECHECKING_H__

#include <typeinfo>

/*
enum datatype{
    DATATYPE_int,
    DATATYPE_double,
    DATATYPE_bool,
    DATATYPE_void, // Not available for variables, but can be returned by a function
};*/
class CCType;

CCType stodatatype (std::string s);

llvm::Type* datatype2llvmType(CCType d);

llvm::Value* createDefaultValue(CCType d);

class CCType {
protected:
    CCType(std::vector<CCType> operands)
        : operands_(operands)
    {}
    
    virtual bool equal (const CCType &other) const;
    //bool typeidequal (const CCType &other) const;
    
public:
    virtual ~CCType() {}
    
    bool operator== (const CCType &other) const;
    bool operator!= (const CCType &other) const;
    
    CCType operand(size_t i) const { return operands_[i]; }
    size_t size() const { return operands_.size(); }

protected:
    std::vector<CCType> operands_;
};

class CCPrimitiveType : public CCType {
public:
    CCPrimitiveType()
        : CCType({})
    {}
};

class CCVoidType : public CCPrimitiveType {
public:
    CCVoidType()
        : CCPrimitiveType()
    {}
};

class CCArithmeticType : public CCPrimitiveType {
public:
    CCArithmeticType()
        : CCPrimitiveType()
    {}
};

class CCIntegerType : public CCArithmeticType {
public:
    CCIntegerType()
        : CCArithmeticType()
    {}
};

class CCDoubleType : public CCArithmeticType {
public:
    CCDoubleType()
        : CCArithmeticType()
    {}
};

class CCBooleanType : public CCPrimitiveType {
public:
    CCBooleanType()
        : CCPrimitiveType()
    {}
};

class CCFunctionType : public CCType {
public:
    CCFunctionType(CCType ret, std::vector<CCType> args)
        : CCType({})
    {
        args.insert(args.begin(), ret);
        operands_ = args;
    }
    
    CCType returnType () const { return operand(0); }
    // Mind you -- one-based argument list
    CCType argument (size_t i) const { return operand(i); }
};

class CCReferenceType : public CCType {
public:
    CCReferenceType(CCType of)
        : CCType({of})
    {}
    
    CCType of () const { return operand(0); }
};

#endif
