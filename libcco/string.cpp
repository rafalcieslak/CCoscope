#include "string.h"

struct __cco_string __cco_string_new(){
    struct __cco_string s;
    s.length = 0;
    return s;
}

struct __cco_string __cco_string_new(char* source){
    struct __cco_string s;
    size_t len = 0;
    while(*source != '\0') {
        len++;
        source++;
    }
    s.length = len;
    s.s = source;
    return s;
}

struct __cco_string __cco_string_concat(struct __cco_string a,
                                       struct __cco_string b){
    struct __cco_string s;
    s.length = a.length + b.length;
    char* nspace = new char [s.length];
    // TODO: do sth when std::bad_alloc appears
    char* i = nspace;
    char* j = a.s;
    while(*j != '\0') {
        *i = *j;
        i++;
        j++;
    }
    j = b.s;
    while(*j != '\0') {
        *i = *j;
        i++;
        j++;
    }
    s.s = nspace;
    return s;
}

int __cco_string_equal(struct __cco_string a,
                         struct __cco_string b){
    if(a.length != b.length)
        return 0;
    char* ia = a.s;
    char* ib = b.s;
    size_t i = 0;
    while(i < a.length) {
        if(*ia != *ib)
            return 0;
        i++;
        ia++;
        ib++;
    }
    return 1;
}
