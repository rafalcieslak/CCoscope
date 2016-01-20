#include "tokenizer.h"
#include "parser.h"
#include "codegencontext.h"
#include "tree.h"

#include "llvm/IR/Module.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/TypeBuilder.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/Transforms/Scalar.h"

#include "llvm/Support/raw_os_ostream.h"

#include <iostream>
#include <fstream>

#include <cstdlib>
#include <getopt.h>

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

bool FileExists(std::string name)
{
  // For compilers that support C++14 experimental TS:
  // std::experimental::filesystem::exists(name);
  // For C++17:
  // std::filesystem::exists(name);
  return (bool)std::ifstream(name);
}


std::vector<std::string> SplitString(std::string str, std::string delimiter){
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

std::string JoinString(std::vector<std::string> str, std::string c){
	std::string buf = "";
	for(unsigned int i = 0; i < str.size(); i++){
		buf += str[i];
		if(i < str.size()-1) buf += c;
	}
	return buf;
}

std::string FindLLVMExecutable(std::string name){
    std::vector<std::string> suffixes = {"-3.7", "-3.6", "-3.5", ""};
    const char* c = getenv("PATH");
    if(!c) return "";
    std::string path(c);
    auto paths = SplitString(path, ":");
    for(const std::string& p : paths){
        for(const std::string& s : suffixes){
            std::string file = p + "/" + name + s;
            if(FileExists(file)){
                // std::cout << "Found " << name << " at " << file << std::endl;
                return file;
            }
        }
    }
    return "";
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

int AssembleIR(std::string input_file, std::string outfile){

    // Compile the ir file to assembly
    std::string llc_path = FindLLVMExecutable("llc");
    if(llc_path.empty()) return -1;

    std::string command = llc_path + " " + input_file + " -o " + outfile;
    std::cout << "Executing: '" << command << "'..." << std::endl;

    int n = system(command.c_str());
    if(n < 0) return -1; // We hope that the failed command will print some output.

    return 0;
}

int AssembleAS(std::string infile, std::string outfile){

    // Assemble the file into an object file
    std::string clang_path = FindLLVMExecutable("clang");
    if(clang_path.empty()) return -1;

    std::string command = clang_path + " -c " + infile + " -o " + outfile;
    std::cout << "Executing: '" << command << "'..." << std::endl;

    int n = system(command.c_str());
    if(n < 0) return -1; // We hope that the failed command will print some output.

    return 0;
}

int Link(std::vector<std::string> input_files, std::string outfile){

    // Link object file(s) to an executable file

    // The hard way would be to manually call ld. But this gets very
    // tricky, because in order to correctly locate which extra libs
    // need to be linked on that particular system (dynamic linker
    // loader, crtX), which architecture is present, etc. a lot of
    // information about the system would need to be collected.

    // The easy way would be to ask GCC to link everything for us the
    // way it normally does. From my understanding, this is what the
    // llvm linker does! So apparently even clang uses GCC for
    // linking. Another option would be to call clang to do that for
    // us. This may seem like cheating, but doing so avoids creating
    // new dependencies, as clang is a llvm tool.

    std::string clang_path = FindLLVMExecutable("clang");
    if(clang_path.empty()) return -1;

    std::string command = clang_path + " " + JoinString(input_files, " ") + " -o " + outfile;
    std::cout << "Executing: '" << command << "'..." << std::endl;

    // TODO: Use some wiser mechanism than system()
    int n = system(command.c_str());
    if(n < 0) return -1; // We hope that the failed command will print some output.

    return 0;
}

inline bool ends_with(std::string const & value, std::string const & ending)
{
    if (ending.size() > value.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

std::string GetTmpFile(std::string suffix = ""){
    /* I KNOW THAT USING tmpnam IS DANGEROUS, but the linker is still
     * mad at me.  Sorry, but the safe alternatives return an open
     * file descriptor.  And I need a file name to pass to
     * subprocesses. There is no other way to get such name.  */
    return tmpnam(nullptr) + suffix;
}
std::string SubstFileExt(std::string filename, std::string ext){
    auto pos = filename.rfind(".");
    if(pos == std::string::npos) return filename + "." + ext;
    else return filename.substr(0,pos) + "." + ext;
}

void CopyFile(std::string src, std::string dest){
    std::cout << "Copying file " << src << " to " << dest << std::endl;
    std::ifstream  s(src , std::ios::binary);
    std::ofstream  d(dest, std::ios::binary);
    d << s.rdbuf();
}

enum class FileType{
    Unknown,
    CCO,
    LL,
    AS,
    OBJ,
};

FileType GuessFileType(std::string filename){
    if(ends_with(filename, ".cco")) return FileType::CCO;
    if(ends_with(filename, ".ll"))  return FileType::LL;
    if(ends_with(filename, ".s"))   return FileType::AS;
    if(ends_with(filename, ".obj")) return FileType::OBJ;
    if(ends_with(filename, ".o"))   return FileType::OBJ;
    return FileType::Unknown;
}

static struct option long_opts[] =
    {
        {"no-link", no_argument, 0, 'c'},
        {"no-assemble", no_argument, 0, 'S'},
        {"opt-level", required_argument,0 , 'O'},
        {"output", required_argument, 0, 'o'},
        {"help", no_argument, 0, 'h'},
        {0,0,0,0}
    };

void usage(const char* prog){
    std::cout << "Usage: " << prog << " [OPTION]... [FILE]... [-o OUT_FILE] \n";
    std::cout << "\n";
    std::cout << "Compiles and links CCoscope source files.\n";
    std::cout << " -h, --help         Prints out this message.\n";
    std::cout << " -c, --no-link      Compiles and assembles files without linking them.\n";
    std::cout << " -S, --no-assemble  Compiles files to LLVM IR, without assembling or linking them.\n";
    std::cout << "\n";
    exit(0);
}

enum OperationMode{
    CompileOnly,
    CompileAndAssemble,
    CompileAssembleAndLink,
};


int main(int argc, char** argv){

    enum OperationMode opmode = OperationMode::CompileAssembleAndLink;

    std::string config_outfile;
    std::vector<std::string> config_infiles;

    // Command-line argument recognition
    int c;
    int opt_index = 0;
    while((c = getopt_long(argc,argv,"hcSO:o:l:L:",long_opts,&opt_index)) != -1){
        switch (c){
        case 'h':
            usage(argv[0]);
            break;
        case 'o':
            config_outfile = optarg;
            break;
        case 'c':
            opmode = OperationMode::CompileAndAssemble;
            break;
        case 'S':
            opmode = OperationMode::CompileOnly;
            break;
        default:
            std::cout << "Unrecognized option " << (char)c << std::endl;
            std::cout << "Use " << argv[0] << " --help for usage options" << std::endl;
            return 1;
        }
    }

    while(optind < argc){
        config_infiles.push_back(argv[optind]);
        optind++;
    }

    // Check whether the number of input files is correct.
    if(config_infiles.size() == 0){
        std::cout << "No input files." << std::endl;
        return 0;
    }
    if( (opmode == OperationMode::CompileOnly || opmode == OperationMode::CompileAndAssemble) &&
        config_infiles.size() > 1){
        std::cout << "Only one input file allowed in -S mode." << std::endl;
        return 1;
    }

    // Create a default output filename
    if(config_outfile == ""){
        // TODO: Actually, the default output file should always be in the current working directory.
        // This temporary trick places them in their source dir.
        if(opmode == CompileOnly) config_outfile = SubstFileExt(config_infiles[0], "ll");
        if(opmode == CompileAndAssemble) config_outfile = SubstFileExt(config_infiles[0], "o");
        if(opmode == CompileAssembleAndLink) config_outfile = "a.out";
    }

    // Stores a list of files that need linking together.
    std::vector<std::string> files_to_be_linked;

    // Stores the temporary file name.
    std::string tmpfile;

    int res;

    for(auto& file : config_infiles){
        FileType t = GuessFileType(file);
        switch(t){
            // TODO: Search for unknown files even before processing any of files on the list.
        case FileType::Unknown:
            std::cout << "Unable to recognize file format for " << file << "." << std::endl;
            return 1;
        case FileType::CCO:
            tmpfile = GetTmpFile(".ll");
            res = Compile(file, tmpfile);
            if(res < 0) return -1;
            file = tmpfile;
            /* FALLTHROUGH */
        case FileType::LL:
            if( opmode == CompileOnly ) // No assembling
                break;
            tmpfile = GetTmpFile(".s");
            res = AssembleIR(file, tmpfile);
            if(res < 0) return -1;
            file = tmpfile;
            /* FALLTHROUGH */
        case FileType::AS:
            if( opmode == CompileOnly ) // No assembling
                break;
            tmpfile = GetTmpFile(".o");
            res = AssembleAS(file, tmpfile);
            if(res < 0) return -1;
            file = tmpfile;
            /* FALLTHROUGH */
        case FileType::OBJ:
            if( opmode == CompileAndAssemble || opmode == CompileOnly ) // No linking
                break;
            files_to_be_linked.push_back(file);
        }

    }

    if( opmode == CompileAssembleAndLink ){
        // Link o files together
        res = Link(files_to_be_linked, config_outfile);
        if(res < 0) return -1;
    }else if( opmode == CompileAndAssemble ){
        // Only one file was processed and it's output name is stored in tmpfile.
        CopyFile(tmpfile, config_outfile);
    }else if( opmode == CompileOnly ){
        // Only one file was processed and it's output name is stored in tmpfile.
        CopyFile(tmpfile, config_outfile);
    }

    return 0;

}
