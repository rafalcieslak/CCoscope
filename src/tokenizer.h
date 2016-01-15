

#ifndef TOKENIZER_INCLUDED
#define TOKENIZER_INCLUDED    1


#include <list>

#include "token.h"


struct tokenizer
{
    // The lookahead buffer.
    // Required by maphoon.
    std::list< token > lookahead;

    // The list of tokens, egerly evaluated.
    std::list< token > tokenlist;

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
