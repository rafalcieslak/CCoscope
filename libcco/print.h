#ifndef __CCO_PRINT_H__
#define __CCO_PRINT_H__

// Eventually, these definitions will have to be wrapped in macros, so
// that they can either define these functions, or unwrap into a C++
// code that inserts their prototypes to the llvm::Module.

void __cco_print_int(int);
void __cco_print_double(double);
void __cco_print_bool(int);

#endif // __CCO_PRINT_H__