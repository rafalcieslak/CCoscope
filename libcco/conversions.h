#ifndef __CONVERSIONS_H__
#define __CONVERSIONS_H__

#include "../common/common_types.h"

extern "C" {

struct __cco_string __cco_int_to_string(int);

struct __cco_string __cco_double_to_string(double);

struct __cco_string __cco_bool_to_string(bool);

struct __cco_string __cco_complex_to_string(struct __cco_complex);

}

#endif // __COMPLEX_H__
