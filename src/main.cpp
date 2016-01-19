#include "tokenizer.h"
#include "parser.h"
#include "codegencontext.h"
#include "tree.h"

#include "llvm/IR/Module.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/Transforms/Scalar.h"

#include "llvm/Support/raw_os_ostream.h"

#include <iostream>
#include <fstream>

#include <cstdlib>

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

bool get_file_exists(std::string name)
{
  // For compilers that support C++14 experimental TS:
  // std::experimental::filesystem::exists(name);
  // For C++17:
  // std::filesystem::exists(name);
  return (bool)std::ifstream(name);
}


std::vector<std::string> split_string(std::string str, std::string delimiter){
    std::vector<std::string> res;
    size_t pos = 0;
    std::string token;
    while ((pos = str.find(delimiter)) != std::string::npos) {
        token = str.substr(0, pos);
        res.push_back(token);
        str.erase(0, pos + delimiter.length());
    }
    res.push_back(str);
    return res;
}

std::string find_llvm_executable(std::string name){
    std::vector<std::string> suffixes = {"-3.7", "-3.6", "-3.5", ""};
    const char* c = getenv("PATH");
    if(!c) return "";
    std::string path(c);
    auto paths = split_string(path, ":");
    for(const std::string& p : paths){
        for(const std::string& s : suffixes){
            std::string file = p + "/" + name + s;
            if(get_file_exists(file)){
                std::cout << "Found " << name << " at " << file << std::endl;
                return file;
            }
        }
    }
    return "";
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

    // Create a new pass manager attached to it.
    auto TheFPM = std::make_shared<legacy::FunctionPassManager>(ctx.TheModule.get());
    // mem2reg
    TheFPM->add(createPromoteMemoryToRegisterPass());
    // Do simple "peephole" optimizations and bit-twiddling optzns.
    TheFPM->add(createInstructionCombiningPass());
    // Reassociate expressions.
    TheFPM->add(createReassociatePass());
    // Eliminate Common SubExpressions.
    TheFPM->add(createGVNPass());
    // Simplify the control flow graph (deleting unreachable blocks, etc).
    TheFPM->add(createCFGSimplificationPass());

    TheFPM->doInitialization();


    for(const auto& protoAST : prototypes){
        Function* func = protoAST->codegen(ctx);
        if(!func) return -1;
        func->dump();
    }
    for(const auto& functionAST : definitions){
        Function* func = functionAST->codegen(ctx);
        if(!func) return -1;
        TheFPM->run(*func);
        func->dump();
    }

    // TODO: Keep these files on some tmpdir (mkstemp?)
    std::string  ll_file_path = "out.ll";
    std::string   s_file_path = "out.s";
    std::string exe_file_path = "out.a";

    // Write the resulting llvm ir to some output file
    std::ofstream lloutfile(ll_file_path);
    raw_os_ostream raw_lloutfile(lloutfile);
    ctx.TheModule->print(raw_lloutfile, nullptr);
    lloutfile.close();


    // Compile the ir file to assembly
    std::string llc_path = find_llvm_executable("llc");
    if(llc_path.empty()) return -1;

    std::string command = llc_path + " " + ll_file_path + " -o " + s_file_path;
    std::cout << "Executing: '" << command << "'..." << std::endl;

    // TODO: Use some wiser mechanism than system()
    int n = system(command.c_str());
    if(n < 0) return -1; // We hope that the failed command will print some output.


    // Compile and link assembly to an executable file

    // The hard way would be to manually call as (which is simple),
    // and then manually call ld. But this gets very tricky, because
    // in order to correctly locate which extra libs need to be linked
    // on that particular system (dynamic linker loader, crtX), which
    // architecture is present, etc. a lot of information about the
    // system would need to be collected.

    // The easy way would be to ask GCC to link everything for us the
    // way it normally does. From my understanding, this is what the
    // llvm linker does! So apparently even clang uses GCC for
    // linking. Another option would be to call clang to do that for
    // us. This may seem like cheating, but doing so avoids creating
    // new dependencies, as clang is a llvm tool.

    std::string clang_path = find_llvm_executable("clang");
    if(clang_path.empty()) return -1;

    command = clang_path + " " + s_file_path + " -o " + exe_file_path;
    std::cout << "Executing: '" << command << "'..." << std::endl;

    // TODO: Use some wiser mechanism than system()
    n = system(command.c_str());
    if(n < 0) return -1; // We hope that the failed command will print some output.


    return 0;

}
