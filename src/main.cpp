#include "tokenizer.h"
#include "parser.h"
#include "codegencontext.h"
#include "tree.h"

#include "llvm/IR/Module.h"

#include <iostream>

using namespace llvm;

bool was_parser_successful(tokenizer& tok){\

    // The logic for determining parsing success is not
    // straight-forward. The fact that the lookahaead buffer is an
    // std::list and has not element access operator makes this even
    // worse. Please consult maphoon manual, chapter 10.

    if(tok.lookahead.size() == 2){
        const token& t1 = tok.lookahead.front();
        const token& t2 = *(std::next(tok.lookahead.begin()));
        return t1.type == tkn_Start && t2.type == tkn_EOF;
    }else if(tok.lookahead.size() == 1){
        return tok.lookahead.front().type == tkn_Start;
    }else{
        return false;
    }
}

int main(){

    tokenizer tok;
    tok.prepare();

    std::cout << "Parsing..." << std::endl;

    std::list<std::shared_ptr<PrototypeAST>> prototypes;
    std::list<std::shared_ptr<FunctionAST>> definitions;

    parser(tok,prototypes,definitions,tkn_Start,0);

    // Instead of returning an exit status, the parser returns
    // nothing, and we need to determine if parsing was successful by
    // examining the lookahead buffer.
    if( !was_parser_successful(tok) ){
        std::cout << "The parser failed to parse input. " << std::endl;
        return -1;
    }
    std::cout << "The parser was successful!" << std::endl;

    std::cout << "Found " << prototypes.size() << " prototypes and " << definitions.size() << " function definitions. " << std::endl;

    auto module = std::make_shared<Module>("CCoscope compiler", getGlobalContext());
    CodegenContext ctx(module);

    for(const auto& protoAST : prototypes){
        Function* func = protoAST->codegen(ctx);
        if(!func) return -1;
        func->dump();
    }
    for(const auto& functionAST : definitions){
        Function* func = functionAST->codegen(ctx);
        if(!func) return -1;
        func->dump();
    }

    return 0;

}
