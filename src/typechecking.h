
#include "tree.h"

class CCType {
public:
    CCType(std::vector<CCType> operands)
        : operands_(operands)
    {}
    
    virtual ~CCType() {}
    
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

class CCFloatType : public CCArithmeticType {
public:
    CCFloatType()
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


