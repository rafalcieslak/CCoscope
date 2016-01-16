#include "tokenizer.h"
#include "parser.h"
#include "codegencontext.h"

#include "llvm/IR/Module.h"

#include <iostream>

using namespace llvm;

int main(){


    tokenizer tok;
    tok.prepare();

    std::cout << "Parsing..." << std::endl;

    parser(tok,tkn_Start,0);

    auto module = std::make_shared<Module>("CCoscope compiler", getGlobalContext());
    CodegenContext ctx(module);
}
