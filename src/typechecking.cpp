
#include <typeinfo>

CCType stodatatype (std::string s) {
    if (s == "int")
        return CCIntegerType();
    else if (s == "double")
        return CCDoubleType();
    else if (s == "bool")
        return CCBooleanType;
    else
        return CCVoidType();
}

Type* datatype2llvmType(CCType d) {
    if(d == CCIntegerType())
        return llvm::Type::getInt32Ty(getGlobalContext());
    else if(d == CCBooleanType())
        return llvm::Type::getInt1Ty(getGlobalContext());
    else if(d == CCDoubleType())
        return llvm::Type::getDoubleTy(getGlobalContext());
    else if(d == CCVoidType())
        return llvm::Type::getVoidTy(getGlobalContext());
    
    return nullptr;
}

Value* createDefaultValue(CCType d) {
    if(d == CCIntegerType())
        return llvm::ConstantInt::get(getGlobalContext(), APInt(32, 0, 1));
    else if(d == CCDoubleType())
        return llvm::ConstantFP::get(getGlobalContext(), APFloat(0.0));
    else if(d == CCBooleanType())
        return llvm::ConstantInt::getFalse(getGlobalContext());
    
    return nullptr;
}

bool CCType::operator== (const CCType &other) const {
    return this->equal(other);
}

bool CCType::operator!= (const CCType &other) const {
    return !(*this == other);
}

bool CCType::/*typeid*/equal (const CCType &other) const {
    return typeid(*this) == typeid(other);
}

/*bool CCVoidType::equal (const CCType &other) const {
    return this->typeidequal(other);
}*/

/*bool CCIntegerType::equal (const CCType &other) const {
    return this->typeidequal(other);
}

bool CCDoubleType::equal (const CCType &other) const {
    return this->typeidequal(other);
}

bool CCBooleanType::equal (const CCType &other) const {
    return this->typeidequal(other);
}*/

bool CCFunctionType::equal (const CCType &other) const {
    if(size() != other.size())
        return false;
    
    CCFunctionType* otherptr;
    if(CCType::equal(other))
        otherptr = dynamic_cast<CCFunctionType*>(&other);
    else
        return false;
    
    for(size_t i = 0; i < size(); i++) {
        if(operand(i) != otherptr->operand(i))
            return false;
    }
    
    return true;
}

bool CCReferenceType::equal (const CCType &other) const {
    CCReferenceType* otherptr;
    if(CCType::equal(other))
        otherptr = dynamic_cast<CCReferenceType*>(&other);
    else
        return false;
    
    return of() == other->of();
}

