// Terminal symbols:
%token EOF SCANERROR
%token SEMICOLON COLON COMMA
%token KEYWORD_EXTERN KEYWORD_FUN KEYWORD_VAR KEYWORD_RETURN
%token TYPE
%token KEYWORD_IF KEYWORD_ELSE KEYWORD_WHILE KEYWORD_FOR
%token LPAR RPAR
%token LBRACKET RBRACKET
%token IDENTIFIER
%token ASSIGN
%token ADD SUB MULT DIV MOD
%token EQUAL NEQUAL LESS LESSEQ GREATER GREATEREQ
%token LITERAL_INT LITERAL_FLOAT

// Non-terminal symbols:
%token E F G H LISTARGS
%token Module Command
%token Start FuncDecl FuncDef ReturnType FuncArgs
%token Block Statement StatementList
%token VarList VarDef
%token If While
%token Return

%startsymbol Start EOF

%attribute value_float  float
%attribute value_int     int
%attribute id            std::string
%attribute reason        std::string

%constraint IDENTIFIER  id 1 2
%constraint TYPE        id 1 2

%constraint LITERAL_INT value_int 1 2
%constraint LITERAL_FLOAT value_float 1 2

#include <cassert>
#define ASSERT( X ) { assert( ( X ) ); }
#include <cstdio>

% Start : Module
%       ;


% Module : FuncDef Module
%         | FuncDecl Module
%         |
%         ;


% FuncDecl : KEYWORD_EXTERN KEYWORD_FUN IDENTIFIER LPAR FuncArgs RPAR ReturnType SEMICOLON
{
    std::cout << "Function declaration found!" << std::endl;
}
%          ;


% FuncDef : KEYWORD_FUN IDENTIFIER LPAR FuncArgs RPAR ReturnType Block
    std::cout << "Complete function definition found!" << std::endl;
%         ;


% ReturnType : COLON TYPE
%            |
%            ;

% FuncArgs : TypedIdentifier FuncArgsRest
%          | IDENTIFIER      FuncArgsRest
%          |
%          ;

% FuncArgsRest : COMMA TypedIdentifier FuncArgsRest
%              | COMMA IDENTIFIER      FuncArgsRest
%              |
%              ;

% TypedIdentifier : IDENTIFIER COLON TYPE
%                 ;

% Block : LBRACKET VarList StatementList RBRACKET
%           ;

% VarList : VarDef VarList
%         |
%         ;

% VarDef : KEYWORD_VAR IDENTIFIER COLON TYPE SEMICOLON
%        ;

% StatementList : Statement StatementList
%               |
%               ;

% Statement : Assignment
%           | If
%           | While
%           | For
%           | Return
%           | Block
%           ;

% Assignment : IDENTIFIER ASSIGN Expression SEMICOLON
%            ;

% If : KEYWORD_IF LPAR Expression RPAR Block
%    | KEYWORD_IF LPAR Expression RPAR Block KEYWORD_ELSE Block
%    ;

% While : KEYWORD_WHILE LPAR Expression RPAR Block
%       ;


% While : KEYWORD_WHILE LPAR Expression RPAR Block
%       ;

% Return : KEYWORD_RETURN Expression SEMICOLON
%        ;

% Expression : LITERAL_INT
%            | IDENTIFIER
%            ;
