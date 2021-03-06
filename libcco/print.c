#include "print.h"
#include <stdio.h>

void __cco_print_int(int v){
    printf("%d\n",v);
}
void __cco_print_double(double v){
    printf("%f\n",v);
}
void __cco_print_bool(int v){
    if(v) printf("True\n");
    else  printf("False\n");
}

void __cco_print_cstr(char* v) {
    printf("%s\n", v);
}

void __cco_print_complex(struct __cco_complex c){
    printf("Complex[Re = %f; Im = %f]\n", c.re, c.im);
}
