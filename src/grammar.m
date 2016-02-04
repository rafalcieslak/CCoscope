// Terminal symbols:
%token EOF SCANERROR
%token SEMICOLON COLON COMMA PIPE
%token KEYWORD_EXTERN KEYWORD_FUN KEYWORD_VAR KEYWORD_RETURN
%token TYPE
%token KEYWORD_IF KEYWORD_ELSE KEYWORD_WHILE KEYWORD_FOR KEYWORD_BREAK KEYWORD_CONTINUE
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
%token Start FuncDecl FuncDef ReturnType ProtoArgList
%token Block Statement StatementList
%token VarList VarDef
%token If While For
%token Return
%token Expression Expr10 Expr20 Expr30 Expr40 Expr50 Expr100

%startsymbol Start EOF

%attribute value_float   float
%attribute value_int     int
%attribute id            std::string
%attribute reason        std::string
 /* This is an aux used by StatementList and Block */
%attribute statement_list std::list<std::shared_ptr<ExprAST>>
 /* This is an aux used by ProtoArgList and ProtoArgListL */
%attribute protoarglist std::vector<std::pair<std::string,datatype>>
%attribute arglist std::vector<std::shared_ptr<ExprAST>>
 /* This is an aux used by TypedIdentifier */
%attribute typedident std::pair<std::string,datatype>
%attribute returntype       datatype

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
%constraint If tree 1 2
%constraint While tree 1 2
%constraint For tree 1 2
%constraint Statement tree 1 2
%constraint StatementList statement_list 1 2
%constraint Block tree 1 2
%constraint Assignment tree 1 2
%constraint FuncCall tree 1 2
%constraint ProtoArgList  protoarglist 1 2
%constraint ProtoArgListL protoarglist 1 2
%constraint VarList  protoarglist 1 2
%constraint ArgList  arglist 1 2
%constraint ArgListL arglist 1 2
%constraint TypedIdentifier typedident 1 2
%constraint VarDef typedident 1 2
%constraint ReturnType returntype 1 2
/* %constraint KeywordType keywordtype 1 2 */

%intokenheader #include <memory>
%intokenheader #include "tree.h"

%global prototypes  std::list<std::shared_ptr<PrototypeAST>>
%global definitions std::list<std::shared_ptr<FunctionAST>>

#include <cassert>
#define ASSERT( X ) { assert( ( X ) ); }
#include <cstdio>
#include "tree.h"
#include <memory>

% Start : Module
%       ;

// Entire input file
% Module : FuncDef Module
%         | FuncDecl Module
%         |
%         ;

// Function declaration
% FuncDecl : KEYWORD_EXTERN KEYWORD_FUN IDENTIFIER LPAR ProtoArgList RPAR ReturnType SEMICOLON
{
    auto Prototype = std::make_shared<PrototypeAST>(
         IDENTIFIER3->id.front(),
         ProtoArgList5->protoarglist.front(),
         ReturnType7->returntype.front()
         );

    prototypes.push_back(Prototype);
}
%          ;

// Function definition
% FuncDef : KEYWORD_FUN IDENTIFIER LPAR ProtoArgList RPAR ReturnType Block
{
    auto Prototype = std::make_shared<PrototypeAST>(
         IDENTIFIER2->id.front(),
         ProtoArgList4->protoarglist.front(),
         ReturnType6->returntype.front()
         );
    auto Function = std::make_shared<FunctionAST>(
         Prototype,
         Block7->tree.front()
         );

    definitions.push_back(Function);
}
%         ;


// The return type of a function
/* WARNING
 * WARNING
 * Currently all types are assumed to be ints.
 * WARNING
 * WARNING
 */
% ReturnType : COLON TYPE
{   token t(tkn_ReturnType);
    t.returntype.push_back(DATATYPE_int);
    return t;
}
%            |
{   token t(tkn_ReturnType);
    t.returntype.push_back(DATATYPE_void);
    return t;
}
%            ;

// An identifier with type, eg.    x : int
/* WARNING
 * WARNING
 * Currently all types are assumed to be ints.
 * WARNING
 * WARNING
 */
% TypedIdentifier : IDENTIFIER COLON TYPE
{  token t(tkn_TypedIdentifier);
   t.typedident.push_back(std::make_pair(IDENTIFIER1->id.front(), DATATYPE_int));
   return t;
}
%                 ;

% Block : LBRACKET VarList StatementList RBRACKET
{   token t(tkn_Block);
    t.tree.push_back( std::make_shared<BlockAST>(
       VarList2->protoarglist.front(),
       StatementList3->statement_list.front())
    );
    return t;
}
%           ;

// List of variable definitions (at the start of a function body)
% VarList : VarList VarDef
{   VarList1->protoarglist.front().push_back( VarDef2->typedident.front() );
    return VarList1;
}
%         |
{   token t(tkn_VarList);
    t.protoarglist.push_back( std::vector<std::pair<std::string,datatype>>() );
    return t;
}
%         ;

% VarDef : KEYWORD_VAR TypedIdentifier SEMICOLON
{   TypedIdentifier2->type = tkn_VarDef;
    return TypedIdentifier2;
}
%        ;


// A list of statements, like in a function body.
% StatementList : Statement StatementList
{
    StatementList2->statement_list.front().push_front( Statement1->tree.front() );
    return StatementList2;
}
%               |
{   token t(tkn_StatementList);
    t.statement_list.push_back( std::list<std::shared_ptr<ExprAST>>() );
    return t;
}
%               ;


// All kinds of statements
% Statement : Assignment
{   Assignment1->type = tkn_Statement;
    return Assignment1;
}
%           | If
{   If1->type = tkn_Statement;
    return If1;
}
%           | While
{   While1->type = tkn_Statement;
    return While1;
}
%           | For
{   For1->type = tkn_Statement;
    return For1;
}
%           | Return
{   Return1->type = tkn_Statement;
    return Return1;
}
%           | Block
{   Block1->type = tkn_Statement;
    return Block1;
}
%           | FuncCall SEMICOLON
{   FuncCall1->type = tkn_Statement;
    return FuncCall1;
}
%           | KEYWORD_BREAK SEMICOLON
{   token t(tkn_Statement);
    t.tree.push_back(std::make_shared<KeywordAST>(
          keyword::Break
          ));
    return t;
}
%           | KEYWORD_CONTINUE SEMICOLON
{   token t(tkn_Statement);
    t.tree.push_back(std::make_shared<KeywordAST>(
          keyword::Continue
          ));
    return t;
}
%           ;


// Assignment statement
% Assignment : IDENTIFIER ASSIGN Expression SEMICOLON
{   token t(tkn_Assignment);
    t.tree.push_back( std::make_shared<AssignmentAST>(
         IDENTIFIER1->id.front(),
         Expression3->tree.front()
        ) );
    return t;
}
%            ;

// IF statement
% If : KEYWORD_IF LPAR Expression RPAR Block
{   token t(tkn_If);
    t.tree.push_back( std::make_shared<IfExprAST>(
      Expression3->tree.front(),
      Block5->tree.front(),
      nullptr
     ) );
    return t;
}
%    | KEYWORD_IF LPAR Expression RPAR Block KEYWORD_ELSE Block
{   token t(tkn_If);
    t.tree.push_back( std::make_shared<IfExprAST>(
      Expression3->tree.front(),
      Block5->tree.front(),
      Block7->tree.front()
     ) );
    return t;
}
%    ;

% While : KEYWORD_WHILE LPAR Expression RPAR Block
{   token t(tkn_While);
    t.tree.push_back( std::make_shared<WhileExprAST>(
      Expression3->tree.front(),
      Block5->tree.front()
     ) );
    return t;
}
%       ;

% For : KEYWORD_FOR LPAR VarList StatementList PIPE Expression PIPE StatementList RPAR Block
{   token t(tkn_For);
    t.tree.push_back( std::make_shared<ForExprAST>(
      std::make_shared<BlockAST>(
       VarList3->protoarglist.front(),
       StatementList4->statement_list.front()),
      Expression6->tree.front(),
      StatementList8->statement_list.front(),
      Block10->tree.front()
     ) );
    return t;
}
%       ;


// Return from function statement
% Return : KEYWORD_RETURN Expression SEMICOLON
{   token t(tkn_Return);
    t.tree.push_back(std::make_shared<ReturnExprAST>(Expression2->tree.front()));
    return t;
}
%        ;


// Base expression
% Expression  : Expr20
{   Expr201->type = tkn_Expression;
    return Expr201;
}
%             ;


// Lowest priority operators
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

// Medium priority operators
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

// Highest priority operators / expressions
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
%            | FuncCall
{   FuncCall1->type = tkn_Expr100;
    return FuncCall1;
}
%            ;

% FuncCall  : IDENTIFIER LPAR ArgList RPAR
{   token t(tkn_FuncCall);
    t.tree.push_back(std::make_shared<CallExprAST>(
                                                   IDENTIFIER1->id.front(),
                                                   ArgList3->arglist.front()
    ));
    return t;
}
%           ;

// Arguments list for prototypes, eg.      x : int, y : bool
% ProtoArgList :  ProtoArgListL TypedIdentifier
{   ProtoArgListL1->type = tkn_ProtoArgList;
    ProtoArgListL1->protoarglist.front().push_back( TypedIdentifier2->typedident.front() );
    return ProtoArgListL1;
}
%          |
{   token t(tkn_ProtoArgList);
    t.protoarglist.push_back( std::vector<std::pair<std::string,datatype>>() );
    return t;
}
%          ;

% ProtoArgListL : ProtoArgListL TypedIdentifier COMMA
{   ProtoArgListL1->protoarglist.front().push_back( TypedIdentifier2->typedident.front() );
    return ProtoArgListL1;
}
%              |
{   token t(tkn_ProtoArgListL);
    t.protoarglist.push_back( std::vector<std::pair<std::string,datatype>>() );
    return t;
}
%              ;

// Argument list for function calls, eg.      x,y,z,2*w
% ArgList :  ArgListL Expression
{   ArgListL1->type = tkn_ArgList;
    ArgListL1->arglist.front().push_back( Expression2->tree.front() );
    return ArgListL1;
}
%          |
{   token t(tkn_ArgList);
    t.arglist.push_back( std::vector<std::shared_ptr<ExprAST>>() );
    return t;
}
%          ;

% ArgListL : ArgListL Expression COMMA
{   ArgListL1->arglist.front().push_back( Expression2->tree.front() );
    return ArgListL1;
}
%              |
{   token t(tkn_ArgListL);
    t.arglist.push_back( std::vector<std::shared_ptr<ExprAST>>() );
    return t;
}
%              ;
