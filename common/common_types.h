#ifndef __COMMON_TYPES_H__
#define __COMMON_TYPES_H__

#include <cstddef>

struct __cco_complex{
    double re;
    double im;
};

struct __cco_string {
    char* s;
    size_t length;
};

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/DerivedTypes.h"

using namespace llvm;
// LLVM Type definitions. Used to describe structures to LLVM.
// These must perfectly match corresponding C types.

template<typename T> inline llvm::StructType* __cco_type_to_LLVM();

template<>
inline llvm::StructType* __cco_type_to_LLVM<__cco_complex>(){
    auto double_type = llvm::Type::getDoubleTy(llvm::getGlobalContext());
    // NOTE: The following variable is static, because each call to
    // ...::create introduces a new copy of the same type, which
    // results in duplicate types (they are not uniqued).
    static auto t = llvm::StructType::create(
       llvm::getGlobalContext(),
       {double_type, double_type},
       "__cco_complex" // Let's keep the names matching the libcco type names
       );
    return t;
}

template<>
inline llvm::StructType* __cco_type_to_LLVM<__cco_string>() {
    auto char_type = llvm::Type::getInt8Ty(llvm::getGlobalContext());
    auto ptr_to_char = char_type->getPointerTo(0);
    auto size_type = llvm::IntegerType::get(llvm::getGlobalContext(), sizeof(size_t) * 8);
    static auto t = llvm::StructType::create(
        llvm::getGlobalContext(),
        {ptr_to_char, size_type},
        "__cco_string"
        );
    return t;
};

#include "llvm/IR/TypeBuilder.h"
// LLVM TypeBuilder template specifications. Used to create LLVM structure types on compile-time (see http://llvm.org/docs/doxygen/html/classllvm_1_1TypeBuilder.html).

namespace llvm {
    template<bool xcompile> class TypeBuilder<__cco_complex, xcompile> {
    public:
        static StructType *get(LLVMContext &Context) {
            return __cco_type_to_LLVM<__cco_complex>();
        }
    };

    template<bool xcompile> class TypeBuilder<__cco_string, xcompile> {
    public:
        static StructType* get(LLVMContext& Context) {
            return __cco_type_to_LLVM<__cco_string>();
        }
    };
}  // namespace llvm

#endif // __COMMON_TYPES_H__
