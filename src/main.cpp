#include "tokenizer.h"
#include "parser.h"

#include <iostream>

int main(){


    tokenizer tok;
    tok.prepare();

    std::cout << "Parsing..." << std::endl;

    parser(tok,tkn_Start,0);
}
