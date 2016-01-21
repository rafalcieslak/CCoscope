#include "utils.h"
#include "compiler.h"

#include <iostream>

#include <cstdlib>
#include <getopt.h>

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

int Link(std::vector<std::string> input_files, std::string outfile,
         std::vector<std::string> linkerlibs, std::vector<std::string> linkerdirs){

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

    std::string dirs, libs;
    for(auto dir : linkerdirs)
        dirs += "-L" + dir;
    for(auto lib : linkerlibs)
        libs += "-l" + lib;

    std::string command = clang_path + " " + JoinString(input_files, " ") + " " + dirs + " " + libs + " -o " + outfile;
    std::cout << "Executing: '" << command << "'..." << std::endl;

    // TODO: Use some wiser mechanism than system()
    int n = system(command.c_str());
    if(n < 0) return -1; // We hope that the failed command will print some output.

    return 0;
}

enum class FileType{
    Unknown,
    CCO,
    LL,
    AS,
    OBJ,
};

FileType GuessFileType(std::string filename){
    if(StringEndsWith(filename, ".cco")) return FileType::CCO;
    if(StringEndsWith(filename, ".ll"))  return FileType::LL;
    if(StringEndsWith(filename, ".s"))   return FileType::AS;
    if(StringEndsWith(filename, ".obj")) return FileType::OBJ;
    if(StringEndsWith(filename, ".o"))   return FileType::OBJ;
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
    std::cout << " -l                 Passes additional flags to linker.\n";
    std::cout << " -L                 Passes additional flags to linker.\n";
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

    std::vector<std::string> config_linkerlibs;
    std::vector<std::string> config_linkerdirs;

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
        case 'l':
            config_linkerlibs.push_back(optarg);
            break;
        case 'L':
            config_linkerdirs.push_back(optarg);
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
        res = Link(files_to_be_linked, config_outfile, config_linkerlibs, config_linkerdirs);
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
