#ifndef __COMMON_TYPES_H__
#define __COMMON_TYPES_H__

#ifdef __cplusplus
extern "C"{
#endif

// ANSI C definitions. Used to compile the runtime library.

struct __cco_complex{
    double re;
    double im;
};

#ifdef __cplusplus
} // extern "C"

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/TypeBuilder.h"

using namespace llvm;
// LLVM Type definitions. Used to describe structures to LLVM.
// These must perfectly match corresponding C types.

template<typename T> inline llvm::StructType* __cco_type_to_LLVM();

template<>
inline llvm::StructType* __cco_type_to_LLVM<__cco_complex>(){
    auto double_type = llvm::Type::getDoubleTy(llvm::getGlobalContext());
    auto t = llvm::StructType::get(
       llvm::getGlobalContext(),
       {double_type, double_type},
       "__cco_complex" // The name must be matching, too
       );
    return t;
}

// LLVM TypeBuilder template specifications. Used to create LLVM structure types on compile-time (see http://llvm.org/docs/doxygen/html/classllvm_1_1TypeBuilder.html).

namespace llvm {
    template<bool xcompile> class TypeBuilder<__cco_complex, xcompile> {
    public:
        static StructType *get(LLVMContext &Context) {
            return __cco_type_to_LLVM<__cco_complex>();
        }
    };
}  // namespace llvm

#endif

#endif // __COMMON_TYPES_H__
