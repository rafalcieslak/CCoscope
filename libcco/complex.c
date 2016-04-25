#include "complex.h"

struct __cco_complex __cco_complex_new(double re, double im){
    struct __cco_complex c;
    c.re = re;
    c.im = im;
    return c;
}

struct __cco_complex __cco_complex_add(struct __cco_complex a,
                                       struct __cco_complex b){
    struct __cco_complex c;
    c.re = a.re + b.re;
    c.im = a.im + b.im;
    return c;
}
struct __cco_complex __cco_complex_sub(struct __cco_complex a,
                                       struct __cco_complex b){
    struct __cco_complex c;
    c.re = a.re - b.re;
    c.im = a.im - b.im;
    return c;
}
struct __cco_complex __cco_complex_mult(struct __cco_complex a,
                                        struct __cco_complex b){
    struct __cco_complex c;
    c.re = a.re*b.re - a.im*b.im;
    c.im = a.im*b.re + a.re*b.im;
    return c;
}
struct __cco_complex __cco_complex_mult_double(struct __cco_complex a,
                                               double b){
    struct __cco_complex c;
    c.re = a.re*b;
    c.im = a.im*b;
    return c;
}
struct __cco_complex __cco_complex_div(struct __cco_complex a,
                                       struct __cco_complex b){
    struct __cco_complex c;
    double sumsq = b.re*b.re + b.im*b.im;
    double re = a.re*b.re + a.im*b.im;
    double im = a.im*b.re - a.re*b.im;
    c.re = re/sumsq;
    c.im = im/sumsq;
    return c;
}
struct __cco_complex __cco_complex_div_double(struct __cco_complex a,
                                              double b){
    struct __cco_complex c;
    c.re = a.re/b;
    c.im = a.im/b;
    return c;
}
int __cco_complex_equal(struct __cco_complex a,
                         struct __cco_complex b){
    return a.re == b.re && a.im == b.im;
}
