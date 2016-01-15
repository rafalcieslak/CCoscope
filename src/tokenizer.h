

#ifndef TOKENIZER_INCLUDED
#define TOKENIZER_INCLUDED    1


#include <list>
#include <algorithm>

#include "token.h"

struct fileloc{
fileloc(int l, int c) : line(l), column(c) {}
    int line;
    int column;
};

struct tokenizer
{
    // The lookahead buffer.
    // Required by maphoon.
    std::list< token > lookahead;

    // The list of tokens with their locations in the input file.
    // This list is prepared by flex before any parsing is started.
    std::list< std::pair<token,fileloc> > tokenlist;

    // This list stores the filelocations of tokens that were already
    // eaten by the parser.  The problem with filelocations is that
    // because it is maphoon who generates token class definition,
    // there is no way to append the file location info there. Maphoon
    // will not process it if we anyhow merge it with token info, so
    // we need to keep track of these numbers on our own. And
    // unluckily, it may happen that maphoon eats up N tokens into the
    // lookahead buffer, and then reporrs a syntax error on the first
    // token in the lookahead. So we need to store all token
    // locations. They are present in this list, stored in reverse
    // location (the most recent token is at the front of this list.
    std::list<fileloc> previous;

    // Parses entire input into tokens.
    void prepare();

    // Moves a single token from tokenlist to lookaheead.
    // Required by maphoon.
    void scan( );

    // Report syntax error to the user.
    // Required by maphoon.
    void syntaxerror( );

};



#endif
