#ifndef __COMPLEX_H__
#define __COMPLEX_H__

#include "../common/common_types.h"

struct __cco_complex __cco_complex_new(double, double);

struct __cco_complex __cco_complex_add(struct __cco_complex,
                                       struct __cco_complex);
struct __cco_complex __cco_complex_sub(struct __cco_complex,
                                       struct __cco_complex);
struct __cco_complex __cco_complex_mult(struct __cco_complex,
                                        struct __cco_complex);
struct __cco_complex __cco_complex_div(struct __cco_complex,
                                       struct __cco_complex);
struct __cco_complex __cco_complex_div_double(struct __cco_complex,
                                              double);

int __cco_complex_equal(struct __cco_complex,
                        struct __cco_complex);

#endif // __COMPLEX_H__
