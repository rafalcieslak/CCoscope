#ifndef __UTILS_H__
#define __UTILS_H__

#include <string>
#include <vector>

template<typename T>
class Maybe {
public:
    Maybe() : val(nullptr) {}
    Maybe(T v) : val(&v) {}
    
    bool is_empty () const { return !val; }
    T get () const { return *val; }
    
    void operator = (const Maybe<T>& other) const {
        val = other.val;
    }
    
protected:
    mutable T* val;
};

bool FileExists(std::string name);
std::string GetTmpFile(std::string suffix = "");
// Returns the path to CCoscope executable
std::string GetExecutablePath();

void CopyFile(std::string src, std::string dest);
std::string SubstFileExt(std::string filename, std::string ext);

std::vector<std::string> SplitString(std::string str, std::string delimiter);
std::string JoinString(std::vector<std::string> str, std::string c = "");

inline bool StringEndsWith(std::string const & value, std::string const & ending)
{
    if (ending.size() > value.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

std::string FindLLVMExecutable(std::string name);

// Colorizing strings for terminal output

enum class Color : int{
    Black = 0,
    Red,
    Green,
    Yellow,
    Blue,
    Magenta,
    Cyan,
    White
};

class ColorStrings{
public:
    static std::string Reset();
    static std::string Color(Color c, bool bold = false);
};

#endif // __UTILS_H__
