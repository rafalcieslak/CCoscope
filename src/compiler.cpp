#include "compiler.h"
#include "utils.h"
#include "codegencontext.h"
#include "tree.h"
#include "token.h"
#include "tokenizer.h"
#include "parser.h"

#include "llvm/IR/Module.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/TypeBuilder.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/Transforms/Scalar.h"

#include "llvm/Support/raw_os_ostream.h"

#include <fstream>

using namespace llvm;

bool WasParserSuccessful(tokenizer& tok){\

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

void DeclareCFunctions(CodegenContext& ctx){
    std::vector<Type *> putchar_args = {Type::getInt32Ty(getGlobalContext())};
    FunctionType *putchar_type = FunctionType::get(Type::getInt32Ty(getGlobalContext()), putchar_args, false);
    Function::Create(putchar_type, Function::ExternalLinkage, "putchar", ctx.TheModule.get());


    FunctionType *printf_type = llvm::TypeBuilder<int(char *, ...), false>::get(getGlobalContext());
    Function *f = Function::Create(printf_type, Function::ExternalLinkage, "printf", ctx.TheModule.get());
    ctx.func_printf = f;
}

std::shared_ptr<legacy::FunctionPassManager> PreparePassManager(Module * m){

    // Create a new pass manager attached to it.
    auto TheFPM = std::make_shared<legacy::FunctionPassManager>(m);
    // mem2reg
   /* TheFPM->add(createPromoteMemoryToRegisterPass());
    // Do simple "peephole" optimizations and bit-twiddling optzns.
    TheFPM->add(createInstructionCombiningPass());
    // Reassociate expressions.
    TheFPM->add(createReassociatePass());
    // Eliminate Common SubExpressions.
    TheFPM->add(createGVNPass());
    // Simplify the control flow graph (deleting unreachable blocks, etc).
    TheFPM->add(createCFGSimplificationPass());
*/
    TheFPM->doInitialization();

    return TheFPM;
}

int Compile(std::string infile, std::string outfile){

    if(!FileExists(infile)){
        std::cout << "File " << infile << " does not exist." << std::endl;
        return -1;
    }

    tokenizer tok;
    tok.prepare(infile);

    std::cout << "Parsing " << infile << std::endl;

    std::list<std::shared_ptr<PrototypeAST>> prototypes;
    std::list<std::shared_ptr<FunctionAST>> definitions;

    parser(tok,prototypes,definitions,tkn_Start,0);

    // Instead of returning an exit status, the parser returns
    // nothing, and we need to determine if parsing was successful by
    // examining the lookahead buffer.
    if( !WasParserSuccessful(tok) ){
        std::cout << "The parser failed to parse input. " << std::endl;
        return -1;
    }

    std::cout << "Found " << prototypes.size() << " prototypes and " << definitions.size() << " function definitions. " << std::endl;

    auto module = std::make_shared<Module>("CCoscope compiler", getGlobalContext());
    CodegenContext ctx(module);

    auto TheFPM = PreparePassManager(ctx.TheModule.get());

    DeclareCFunctions(ctx);

    for(const auto& protoAST : prototypes){
        Function* func = protoAST->codegen(ctx);
        if(!func) return -1;
        // func->dump();
    }
    for(const auto& functionAST : definitions){
        Function* func = functionAST->codegen(ctx);
        if(!func) return -1;
        // Optimize the function
        TheFPM->run(*func);
        // func->dump();
    }

    // Write the resulting llvm ir to some output file
    std::cout << "Writing out IR for module " << infile << " to " << outfile << std::endl;

    std::ofstream lloutfile(outfile);
    raw_os_ostream raw_lloutfile(lloutfile);
    ctx.TheModule->print(raw_lloutfile, nullptr);
    lloutfile.close();

    return 0;
}
