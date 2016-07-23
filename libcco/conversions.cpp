#include "conversions.h"
#include <cstring>
#include <cstdio>

struct __cco_string __cco_int_to_string(int i) {
    auto s = std::to_string(i);
    auto ps = new char [s.length()];
    std::memcpy(ps, s.data(), sizeof(char) * s.length());
    __cco_string cs;
    cs.length = s.length();
    cs.s = ps;
    return cs;
}

struct __cco_string __cco_double_to_string(double d) {
    auto s = std::to_string(d);
    auto ps = new char [s.length()];
    std::memcpy(ps, s.data(), sizeof(char) * s.length());
    __cco_string cs;
    cs.length = s.length();
    cs.s = ps;
    return cs;
}

struct __cco_string __cco_bool_to_string(bool b) {
    __cco_string cs;
    if(b) {
        auto ps = new char [4];
        std::memcpy(ps, "true", sizeof(char) * 4);
       // ps[0] = 't'; ps[1] = 'r'; ps[2] = 'u'; ps[3]  = 'e';
        cs.length = 4;
        cs.s = ps;
    } else {
        auto ps = new char [5];
        ps[0] = 'f'; ps[1] = 'a'; ps[2] = 'l'; ps[3]  = 's'; ps[4] = 'e';
        cs.length = 5;
        cs.s = ps;
    }
    return cs;
}

struct __cco_string __cco_complex_to_string(struct __cco_complex c) {
    __cco_string cs, csre, csim;
    csre = __cco_double_to_string(c.re);
    csim = __cco_double_to_string(c.im);
    size_t out_length = 13 + csre.length + 7 + csim.length + 1;
    auto ps = new char [out_length];
    char* aux = ps;
    std::memcpy(aux, "Complex[Re = ", sizeof(char) * 13);
    aux += 13;
    std::memcpy(aux, csre.s, sizeof(char) * csre.length);
    aux += csre.length;
    std::memcpy(aux, "; Im = ", sizeof(char) * 7);
    aux += 7;
    std::memcpy(aux, csim.s, sizeof(char) * csim.length);
    aux += csim.length;
    std::memcpy(aux, "]", sizeof(char) * 1);

    cs.length = out_length;
    cs.s = ps;
    return cs;
}
