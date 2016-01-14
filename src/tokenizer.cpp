#include "tokenizer.h"

// A temporary fake tokenizer.

void tokenizer::scan(){
    // this should be linked to flex.

    // The easiest way to chain these two together that i see would be
    // to have scan() read the ENTIRE file into the lookahead when
    // first called, and then do nothing on further calls.
}


void tokenizer::syntaxerror(){
    // I have no idea when this gets called.
}
