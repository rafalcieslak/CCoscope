#ifndef __COMPILER_H__
#define __COMPILER_H__

#include "misc/utils.h"

namespace ccoscope {

int Compile(std::string infile, std::string outfile, unsigned int optlevel=0);

}

#endif // __COMPILER_H__
