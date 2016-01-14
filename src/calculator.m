
// NOTE: This grammar is currenly unrelated to the desired language specs.
// I just copied it from a previous example as an example maphoon input file to test build scripts.

// Terminal symbols:

%token EOF SCANERROR
%token SEMICOLON BECOMES COMMA
%token IDENTIFIER NUMBER
%token PLUS TIMES MINUS DIVIDES
%token FACTORIAL
%token LPAR RPAR

// Non-terminal symbols:

%token E F G H LISTARGS
%token Session Command
%token Start

%startsymbol Session EOF

%attribute value         double
 // Temporarily
%attribute t             int
%attribute id            std::string
%attribute reason        std::string

%constraint IDENTIFIER  id 1 2
%constraint E t 1 2
%constraint F t 1 2
%constraint G t 1 2
%constraint H t 1 2
%constraint LISTARGS t 0
%constraint SCANERROR id 1 2
%constraint NUMBER value 1 2

#include <cassert>
#define ASSERT( X ) { assert( ( X ) ); }
#include <cstdio>
void parseparse(){
    printf("I did not do any parse!");
}

% Start : Session EOF
%   ;

% Session : Session Command
%         |
%         ;


% Command : E SEMICOLON
%         | IDENTIFIER BECOMES E SEMICOLON
%         | _recover SEMICOLON
%         ;

% E   : E PLUS F
%     | E MINUS F
%     | F
%     ;

% F   : F TIMES G
%     | F DIVIDES G
%     | G
%     ;


%  G : MINUS G
%    | PLUS G
%    | H
%    ;


% H   : H FACTORIAL
%     | LPAR E RPAR
%     | IDENTIFIER
%     | NUMBER
%     | IDENTIFIER LPAR LISTARGS RPAR
%     ;


% LISTARGS : E
%          | LISTARGS COMMA E
%          ;
