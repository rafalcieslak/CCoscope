#ifndef __STRING_H__
#define __STRING_H__

#include "../common/common_types.h"

struct __cco_string __cco_string_new();

struct __cco_string __cco_string_new(char* source);

struct __cco_string __cco_string_concat(struct __cco_string,
                                       struct __cco_string);

int __cco_string_equal(struct __cco_string,
                        struct __cco_string);

#endif // __STRING_H__
