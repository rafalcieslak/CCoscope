

#ifndef TOKENIZER_INCLUDED
#define TOKENIZER_INCLUDED    1


#include <list>

#include "token.h"


struct tokenizer
{
   std::list< token > lookahead;

   void scan( );
      // Gets a token from somewhere and appends it to lookahead.

   void syntaxerror( );
      // Report syntax error to the user.

};



#endif
