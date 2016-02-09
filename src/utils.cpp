#include "utils.h"
#include <fstream>
#include <iostream>

bool FileExists(std::string name)
{
  // For compilers that support C++14 experimental TS:
  // std::experimental::filesystem::exists(name);
  // For C++17:
  // std::filesystem::exists(name);
  return (bool)std::ifstream(name);
}

std::string GetTmpFile(std::string suffix){
    /* I KNOW THAT USING tmpnam IS DANGEROUS, but the linker is still
     * mad at me.  Sorry, but the safe alternatives return an open
     * file descriptor.  And I need a file name to pass to
     * subprocesses. There is no other way to get such name.  */
    return tmpnam(nullptr) + suffix;
}

void CopyFile(std::string src, std::string dest){
    std::cout << "Copying file " << src << " to " << dest << std::endl;
    std::ifstream  s(src , std::ios::binary);
    std::ofstream  d(dest, std::ios::binary);
    d << s.rdbuf();
}

std::string SubstFileExt(std::string filename, std::string ext){
    auto pos = filename.rfind(".");
    if(pos == std::string::npos) return filename + "." + ext;
    else return filename.substr(0,pos) + "." + ext;
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

std::string ColorStrings::Reset(){
    return "\x1b[0m";
}

std::string ColorStrings::Color(enum Color c, bool bold){
    return "\x1b[" + std::to_string((int)c + 30) + ((bold)?";1m":"m");
}
