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
%token Expression Expr10 Expr20 Expr30 Expr40 Expr50 Expr100

%startsymbol Start EOF

%attribute value_float   float
%attribute value_int     int
%attribute id            std::string
%attribute reason        std::string

%attribute tree          std::shared_ptr<ExprAST>

%constraint IDENTIFIER  id 1 2
%constraint TYPE        id 1 2

%constraint LITERAL_INT value_int 1 2
%constraint LITERAL_FLOAT value_float 1 2

%constraint Expr10 tree 1 2
%constraint Expr20 tree 1 2
%constraint Expr30 tree 1 2
%constraint Expr40 tree 1 2
%constraint Expr50 tree 1 2
%constraint Expr100 tree 1 2
%constraint Expression tree 1 2
%constraint Return tree 1 2
%constraint Statement tree 1 2

%intokenheader #include <memory>
%intokenheader #include "tree.h"

#include <cassert>
#define ASSERT( X ) { assert( ( X ) ); }
#include <cstdio>
#include "tree.h"
#include <memory>

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
{   auto statementlistAST = std::dynamic_pointer_cast<StatementListAST>( StatementList2->tree.front() );
    statementlistAST->Prepend(Statement1->tree.front());
    return StatementList2;
}
%               |
{   token t(tkn_StatementList);
    t.tree.push_back(std::make_shared<StatementListAST>());
    return t;
}
%               ;


% Statement : Assignment
%           | If
%           | While
%           | For
%           | Return
{   Return1->type = tkn_Statement;
    return Return1;
}
%           | Block
%           ;


% Assignment : IDENTIFIER ASSIGN Expression SEMICOLON
%            ;

% If : KEYWORD_IF LPAR Expression RPAR Block
%    | KEYWORD_IF LPAR Expression RPAR Block KEYWORD_ELSE Block
%    ;

% While : KEYWORD_WHILE LPAR Expression RPAR Block
%       ;


% Return : KEYWORD_RETURN Expression SEMICOLON
{   token t(tkn_Return);
    t.tree.push_back(std::make_shared<ReturnExprAST>(Expression2->tree.front()));
    return t;
}
%        ;


% Expression  : Expr20
{   Expr201->type = tkn_Expression;
    return Expr201;
}
%             ;


% Expr20     : Expr20 AND Expr30
{   token t(tkn_Expr20);
    t.tree.push_back(std::make_shared<BinaryExprAST>("LOGICAL_AND",
        Expr201->tree.front(),
        Expr303->tree.front() ));
    return t;
}
%            | Expr20 OR  Expr30
{   token t(tkn_Expr20);
    t.tree.push_back(std::make_shared<BinaryExprAST>("LOGICAL_OR",
        Expr201->tree.front(),
        Expr303->tree.front() ));
    return t;
}
%            | Expr30
{   Expr301->type = tkn_Expr20;
    return Expr301;
}
%            ;

/* Note that both sides of operator == and similar are of the same LOWER precedence.
 * This forbids " x == y == z " or " a < b < c " which would be very confusing. */
% Expr30     : Expr40 EQUAL     Expr40
{   token t(tkn_Expr30);
    t.tree.push_back(std::make_shared<BinaryExprAST>("EQUAL",
        Expr401->tree.front(),
        Expr403->tree.front() ));
    return t;
}
%            | Expr40 NEQUAL    Expr40
{   token t(tkn_Expr30);
    t.tree.push_back(std::make_shared<BinaryExprAST>("NEQUAL",
        Expr401->tree.front(),
        Expr403->tree.front() ));
    return t;
}
%            | Expr40 LESS      Expr40
{   token t(tkn_Expr30);
    t.tree.push_back(std::make_shared<BinaryExprAST>("LESS",
        Expr401->tree.front(),
        Expr403->tree.front() ));
    return t;
}
%            | Expr40 GREATER   Expr40
{   token t(tkn_Expr30);
    t.tree.push_back(std::make_shared<BinaryExprAST>("GREATER",
        Expr401->tree.front(),
        Expr403->tree.front() ));
    return t;
}
%            | Expr40 LESSEQ    Expr40
{   token t(tkn_Expr30);
    t.tree.push_back(std::make_shared<BinaryExprAST>("LESSEQ",
        Expr401->tree.front(),
        Expr403->tree.front() ));
    return t;
}
%            | Expr40 GREATEREQ Expr40
{   token t(tkn_Expr30);
    t.tree.push_back(std::make_shared<BinaryExprAST>("GREATEREQ",
        Expr401->tree.front(),
        Expr403->tree.front() ));
    return t;
}
%            | Expr40
{   Expr401->type = tkn_Expr30;
    return Expr401;
}
%            ;

% Expr40     : Expr40 ADD Expr50
{   token t(tkn_Expr40);
    t.tree.push_back(std::make_shared<BinaryExprAST>("ADD",
        Expr401->tree.front(),
        Expr503->tree.front() ));
    return t;
}
%            | Expr40 SUB Expr50
{   token t(tkn_Expr40);
    t.tree.push_back(std::make_shared<BinaryExprAST>("SUB",
        Expr401->tree.front(),
        Expr503->tree.front() ));
    return t;
}
%            | Expr50
{   Expr501->type = tkn_Expr40;
    return Expr501;
}
%            ;

% Expr50     : Expr50 MULT Expr100
{   token t(tkn_Expr50);
    t.tree.push_back(std::make_shared<BinaryExprAST>("MULT",
        Expr501 ->tree.front(),
        Expr1003->tree.front() ));
    return t;
}
%            | Expr50 DIV  Expr100
{   token t(tkn_Expr50);
    t.tree.push_back(std::make_shared<BinaryExprAST>("DIV",
        Expr501 ->tree.front(),
        Expr1003->tree.front() ));
    return t;
}
%            | Expr50 MOD  Expr100
{   token t(tkn_Expr50);
    t.tree.push_back(std::make_shared<BinaryExprAST>("MOD",
        Expr501 ->tree.front(),
        Expr1003->tree.front() ));
    return t;
}
%            | Expr100
{   Expr1001->type = tkn_Expr50;
    return Expr1001;
}
%            ;


% Expr100    : LITERAL_INT
{   token t(tkn_Expr100);
    t.tree.push_back(std::make_shared<NumberExprAST>(LITERAL_INT1->value_int.front()));
    return t;
}
%            | IDENTIFIER
{   token t(tkn_Expr100);
    t.tree.push_back(std::make_shared<VariableExprAST>(IDENTIFIER1->id.front()));
    return t;
}
%            | LPAR Expression RPAR
{   Expression2->type = tkn_Expr100;
    return Expression2;
}
%            ;
