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
%token ADD SUB MULT DIV
%token EQUAL NEQUAL LESS LESSEQ GREATER GREATEREQ

// Non-terminal symbols:
%token E F G H LISTARGS
%token Session Command
%token Start FuncDecl FuncDef ReturnType FuncArgs FuncBlock

%startsymbol Start EOF

%attribute value_double  double
%attribute value_int     int
%attribute id            std::string
%attribute reason        std::string

%constraint IDENTIFIER  id 1 2
%constraint TYPE        id 1 2

#include <cassert>
#define ASSERT( X ) { assert( ( X ) ); }
#include <cstdio>
void parseparse(){
    printf("I did not do any parse!");
}


% Start : Session
%       ;

% Session : Session FuncDef
%         | Session FuncDecl
%         |
%         ;

% FuncDecl : KEYWORD_EXTERN KEYWORD_FUN IDENTIFIER LPAR FuncArgs RPAR ReturnType SEMICOLON
%          ;

% FuncDef : KEYWORD_FUN IDENTIFIER LBRACKET FuncArgs RBRACKET ReturnType FuncBlock
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

% FuncBlock : LBRACKET RBRACKET
%           ;
