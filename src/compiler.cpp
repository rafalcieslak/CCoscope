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

namespace ccoscope {

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
    using namespace llvm;
    
    std::vector<llvm::Type *> putchar_args = {llvm::Type::getInt32Ty(getGlobalContext())};
    auto putchar_type = llvm::FunctionType::get(llvm::Type::getInt32Ty(getGlobalContext()), putchar_args, false);
    llvm::Function::Create(putchar_type, llvm::Function::ExternalLinkage, "putchar", ctx.TheModule.get());


    auto printf_type = TypeBuilder<int(char *, ...), false>::get(getGlobalContext());
    auto f = llvm::Function::Create(printf_type, llvm::Function::ExternalLinkage, "printf", ctx.TheModule.get());
    ctx.func_printf = f;
}

std::shared_ptr<llvm::legacy::FunctionPassManager> PreparePassManager(llvm::Module * m, unsigned int lvl){
    using namespace llvm;
    
    // Create a new pass manager attached to it.
    auto TheFPM = std::make_shared<legacy::FunctionPassManager>(m);

    // mem2reg
    if(lvl >= 1)
      TheFPM->add(createPromoteMemoryToRegisterPass());
    // Do simple "peephole" optimizations and bit-twiddling optzns.
    if(lvl >= 1)
      TheFPM->add(createInstructionCombiningPass());
    // Reassociate expressions.
    if(lvl >= 2)
      TheFPM->add(createReassociatePass());
    // Eliminate Common SubExpressions.
    if(lvl >= 1)
      TheFPM->add(createGVNPass());
    // Simplify the control flow graph (deleting unreachable blocks, etc).
    if(lvl >= 2)
      TheFPM->add(createCFGSimplificationPass());

    TheFPM->doInitialization();

    return TheFPM;
}

int Compile(std::string infile, std::string outfile, unsigned int optlevel){
    using namespace llvm;
    
    if(!FileExists(infile)){
        std::cout << "File " << infile << " does not exist." << std::endl;
        return -1;
    }

    tokenizer tok;
    tok.prepare(infile);

    std::cout << ColorStrings::Color(Color::Cyan, true) << "Parsing " << infile << ColorStrings::Reset() << std::endl;

    //std::list<Prototype> prototypes;
    //std::list<Function> definitions;
    
    auto ctx = CodegenContext{};
    
    parser(tok, ctx, /*prototypes,definitions, */tkn_Start, 0);
    
    // Instead of returning an exit status, the parser returns
    // nothing, and we need to determine if parsing was successful by
    // examining the lookahead buffer.
    if( !WasParserSuccessful(tok) ){
        std::cout << "The parser failed to parse input. " << std::endl;
        return -1;
    }

    std::cout << "Found " << ctx.prototypes.size() << " prototypes and " << ctx.definitions.size() << " function definitions. " << std::endl;

    auto module = std::make_shared<Module>("CCoscope compiler", getGlobalContext());
    
    //std::unique_ptr<World> world(new World(*module, infile));
    //auto& ctx = world->ctx();
    //CodegenContext ctx(module, infile);
    ctx.SetModuleAndFile(module, infile);
    
    auto TheFPM = PreparePassManager(ctx.TheModule.get(), optlevel);

    DeclareCFunctions(ctx);

    bool errors = false;

    for(const auto& protoAST : ctx.prototypes){
        llvm::Function* func = protoAST->codegen();
        if(!func) {errors = true; continue;} // In case of an error, continue compiling other functions.
        // func->dump();
    }
    for(const auto& functionAST : ctx.definitions){
        llvm::Function* func = functionAST->codegen();
        if(!func) {errors = true; continue;} // In case of an error, continue compiling other functions.
        // Optimize the function
        TheFPM->run(*func);
        // func->dump();
    }

    if(errors || !ctx.IsErrorFree()){
        if(ctx.IsErrorFree())
            std::cout << "Failure: Some functions failed to compile, but no error messages were generated." << std::endl;
        else
            ctx.DisplayErrors();
        return -1;
    }

    // Write the resulting llvm ir to some output file
    std::cout << "Writing out IR for module " << infile << " to " << outfile << std::endl;

    std::ofstream lloutfile(outfile);
    raw_os_ostream raw_lloutfile(lloutfile);
    ctx.TheModule->print(raw_lloutfile, nullptr);
    lloutfile.close();

    return 0;
}

}
