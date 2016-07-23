// Terminal symbols:
%token EOF SCANERROR
%token SEMICOLON COLON COMMA PIPE
%token KEYWORD_EXTERN KEYWORD_FUN KEYWORD_VAR KEYWORD_RETURN
%token TYPE
%token KEYWORD_IF KEYWORD_ELSE KEYWORD_WHILE KEYWORD_FOR KEYWORD_BREAK KEYWORD_CONTINUE
%token LPAR RPAR
%token LBRACKET RBRACKET LSQUAREBRACKET RSQUAREBRACKET
%token IDENTIFIER
%token ASSIGN
%token ADD SUB MULT DIV MOD
%token EQUAL NEQUAL LESS LESSEQ GREATER GREATEREQ
%token LITERAL_INT LITERAL_DOUBLE LITERAL_BOOL LITERAL_STRING

// Non-terminal symbols:
%token E F G H LISTARGS
%token Module Command
%token Start FuncDecl FuncDef ReturnType ProtoArgList
%token Block Statement StatementList
//%token VarList VarDef
%token VarDef
%token If While For
%token Return
%token Expression Expr10 Expr20 Expr30 Expr40 Expr50 Expr100

%startsymbol Start EOF

%attribute value_bool     bool
%attribute value_double   double
%attribute value_int      int
%attribute value_string   std::string
%attribute id             std::string
%attribute reason         std::string
 /* This is an aux used by StatementList and Block */
%attribute statement_list std::list<ccoscope::Expr>
 /* This is an aux used by ProtoArgList and ProtoArgListL */
%attribute protoarglist   std::list<std::pair<std::string,ccoscope::Type>>
%attribute arglist        std::vector<ccoscope::Expr>
 /* This is an aux used by TypedIdentifier */
%attribute typedident     std::pair<std::string, ccoscope::Type>
%attribute returntype     ccoscope::Type

%attribute tree           ccoscope::Expr

%attribute loc            fileloc

     /* Constraints for AST attribute */
%constraint Expr10     tree 1 2
%constraint Expr20     tree 1 2
%constraint Expr30     tree 1 2
%constraint Expr40     tree 1 2
%constraint Expr50     tree 1 2
%constraint Expr100    tree 1 2
%constraint Expression tree 1 2
%constraint Return     tree 1 2
%constraint If         tree 1 2
%constraint While      tree 1 2
%constraint For        tree 1 2
%constraint Statement  tree 1 2
%constraint Block      tree 1 2
%constraint Assignment tree 1 2
%constraint FuncCall   tree 1 2
%constraint VarDef     tree 1 2

     /* Constraints for file location attribute */
%constraint Expr10     loc 1 2
%constraint Expr20     loc 1 2
%constraint Expr30     loc 1 2
%constraint Expr40     loc 1 2
%constraint Expr50     loc 1 2
%constraint Expr100    loc 1 2
%constraint Expression loc 1 2
%constraint Return     loc 1 2
%constraint If         loc 1 2
%constraint While      loc 1 2
%constraint For        loc 1 2
%constraint Statement  loc 1 2
%constraint Block      loc 1 2
%constraint Assignment loc 1 2
%constraint FuncCall   loc 1 2
%constraint VarDef     loc 1 2

%constraint TYPE             loc 1 2
%constraint KEYWORD_EXTERN   loc 1 2
%constraint KEYWORD_FUN      loc 1 2
%constraint KEYWORD_VAR      loc 1 2
%constraint KEYWORD_RETURN   loc 1 2
%constraint KEYWORD_WHILE    loc 1 2
%constraint KEYWORD_IF       loc 1 2
%constraint KEYWORD_ELSE     loc 1 2
%constraint KEYWORD_FOR      loc 1 2
%constraint KEYWORD_BREAK    loc 1 2
%constraint KEYWORD_CONTINUE loc 1 2
%constraint ASSIGN           loc 1 2
%constraint LITERAL_INT      loc 1 2
%constraint LITERAL_DOUBLE   loc 1 2
%constraint LITERAL_BOOL     loc 1 2
%constraint LITERAL_STRING   loc 1 2
%constraint COLON     loc 1 2
%constraint EQUAL     loc 1 2
%constraint NEQUAL    loc 1 2
%constraint GREATEREQ loc 1 2
%constraint LESSEQ    loc 1 2
%constraint GREATER   loc 1 2
%constraint LESS      loc 1 2
%constraint AND       loc 1 2
%constraint OR        loc 1 2
%constraint ADD       loc 1 2
%constraint SUB       loc 1 2
%constraint MULT      loc 1 2
%constraint DIV       loc 1 2
%constraint MOD       loc 1 2
%constraint SEMICOLON loc 1 2
%constraint COMMA     loc 1 2
%constraint LPAR      loc 1 2
%constraint RPAR      loc 1 2
%constraint LBRACKET  loc 1 2
%constraint RBRACKET  loc 1 2
%constraint LSQUAREBRACKET loc 1 2
%constraint RSQUAREBRACKET loc 1 2
%constraint PIPE           loc 1 2
%constraint IDENTIFIER     loc 1 2

     /* Constraints for literal values */
%constraint IDENTIFIER  id 1 2
%constraint TYPE        id 1 2

%constraint LITERAL_INT value_int 1 2
%constraint LITERAL_DOUBLE value_double 1 2
%constraint LITERAL_BOOL value_bool 1 2
%constraint LITERAL_STRING value_string 1 2

     /* Constraints for other attributes */
%constraint StatementList statement_list 1 2
%constraint ProtoArgList  protoarglist 1 2
%constraint ProtoArgListL protoarglist 1 2
%constraint ArgList  arglist 1 2
%constraint ArgListL arglist 1 2
%constraint TypedIdentifier typedident 1 2
%constraint ReturnType returntype 1 2



%intokenheader #include <memory>
%intokenheader #include "../ast/ast.h"
%intokenheader #include "../types/types.h"

%global ctx ccoscope::CodegenContext

#include <cassert>
#define ASSERT( X ) { assert( ( X ) ); }
#include <cstdio>
#include "../ast/ast.h"
#include "../types/types.h"
#include "../world/codegencontext.h"
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
    std::vector<std::pair<std::string, ccoscope::Type>> v{ std::begin(ProtoArgList5->protoarglist.front()), std::end(ProtoArgList5->protoarglist.front()) };
    ctx.makePrototype(
         IDENTIFIER3->id.front(),
        // ProtoArgList5->protoarglist.front(),
        v,
         ReturnType7->returntype.front(),
         KEYWORD_FUN2->loc.front()
         );

}
%          ;

// Function definition
% FuncDef : KEYWORD_FUN IDENTIFIER LPAR ProtoArgList RPAR ReturnType Block
{
    std::vector<std::pair<std::string, ccoscope::Type>> v{ std::begin(ProtoArgList4->protoarglist.front()), std::end(ProtoArgList4->protoarglist.front()) };
    auto Prototype = ctx.makePrototype(
         IDENTIFIER2->id.front(),
        // ProtoArgList4->protoarglist.front(),
        v,
         ReturnType6->returntype.front(),
         KEYWORD_FUN1->loc.front()
         );
     ctx.makeFunction(
         Prototype,
         Block7->tree.front(),
         KEYWORD_FUN1->loc.front()
         );

}
%         ;


// The return type of a function
% ReturnType : COLON TYPE
{   token t(tkn_ReturnType);
    t.returntype.push_back(ccoscope::str2type(ctx, TYPE2->id.front()));
    return t;
}
%            |
{   token t(tkn_ReturnType);
    t.returntype.push_back(ctx.getVoidTy());
    return t;
}
%            ;

// An identifier with type, eg.    x : int
% TypedIdentifier : IDENTIFIER COLON TYPE
{  token t(tkn_TypedIdentifier);
   t.typedident.push_back(std::make_pair(IDENTIFIER1->id.front(),
    ccoscope::str2type(ctx, TYPE3->id.front())));
   return t;
}
%                 ;

% Block : LBRACKET StatementList RBRACKET
//% Block : LBRACKET VarList StatementList RBRACKET
{   token t(tkn_Block);
    t.tree.push_back( ctx.makeBlock(
       StatementList2->statement_list.front(),
       LBRACKET1->loc.front()
    ));
    t.loc.push_back( LBRACKET1->loc.front() );
    return t;
}
%           ;

// A list of statements, like in a function body.
% StatementList : Statement StatementList
{
    StatementList2->statement_list.front().push_front( Statement1->tree.front() );
    return StatementList2;
}
%               | VarDef StatementList
{
    StatementList2->statement_list.front().push_front(VarDef1->tree.front());
    return StatementList2;
}
%               |
{   token t(tkn_StatementList);
    t.statement_list.push_back( std::list<ccoscope::Expr>() );
    return t;
}
%               ;

% VarDef : KEYWORD_VAR IDENTIFIER COLON TYPE SEMICOLON
{   token t(tkn_VarDef);
    t.tree.push_back( ctx.makeVariableDecl(
      IDENTIFIER2->id.front(),
      ccoscope::str2type(ctx, TYPE4->id.front()),
      KEYWORD_VAR1->loc.front()
    ));
    t.loc.push_back( KEYWORD_VAR1->loc.front() );
    return t;
//TypedIdentifier2->type = tkn_VarDef;
//   return TypedIdentifier2;
}
%        ;

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
    t.tree.push_back(ctx.makeLoopControlStmt(
          ccoscope::loopControl::Break,
          KEYWORD_BREAK1->loc.front()
          ));
    t.loc.push_back( KEYWORD_BREAK1->loc.front() );
    return t;
}
%           | KEYWORD_CONTINUE SEMICOLON
{   token t(tkn_Statement);
    t.tree.push_back(ctx.makeLoopControlStmt(
          ccoscope::loopControl::Continue,
          KEYWORD_CONTINUE1->loc.front()
          ));
    t.loc.push_back( KEYWORD_CONTINUE1->loc.front() );
    return t;
}
%           ;


// Assignment statement
//% Assignment : IDENTIFIER ASSIGN Expression SEMICOLON
% Assignment : Expression ASSIGN Expression SEMICOLON
{   token t(tkn_Assignment);
    t.tree.push_back( ctx.makeBinary(
         "ASSIGN",
         Expression1->tree.front(),
         Expression3->tree.front(),
         ASSIGN2->loc.front()
        ) );
    t.loc.push_back( ASSIGN2->loc.front() );
    return t;
}
%            ;

// IF statement
% If : KEYWORD_IF LPAR Expression RPAR Block
{   token t(tkn_If);
    t.tree.push_back( ctx.makeIf(
      Expression3->tree.front(),
      Block5->tree.front(),
      nullptr,
      KEYWORD_IF1->loc.front()
     ) );
    t.loc.push_back( KEYWORD_IF1->loc.front() );
    return t;
}
%    | KEYWORD_IF LPAR Expression RPAR Block KEYWORD_ELSE Block
{   token t(tkn_If);
    t.tree.push_back( ctx.makeIf(
      Expression3->tree.front(),
      Block5->tree.front(),
      Block7->tree.front(),
      KEYWORD_IF1->loc.front()
     ) );
    t.loc.push_back( KEYWORD_IF1->loc.front() );
    return t;
}
%    ;

% While : KEYWORD_WHILE LPAR Expression RPAR Block
{   token t(tkn_While);
    t.tree.push_back( ctx.makeWhile(
      Expression3->tree.front(),
      Block5->tree.front(),
      KEYWORD_WHILE1->loc.front()
     ) );
    t.loc.push_back( KEYWORD_WHILE1->loc.front() );
    return t;
}
%       ;

% For : KEYWORD_FOR LPAR VarList StatementList PIPE Expression PIPE StatementList RPAR Block
{   token t(tkn_For);
    t.tree.push_back( ctx.makeFor(
      ctx.makeBlock(
       StatementList4->statement_list.front(),
       LPAR2->loc.front()),
      Expression6->tree.front(),
      StatementList8->statement_list.front(),
      Block10->tree.front(),
      KEYWORD_FOR1->loc.front()
     ) );
    t.loc.push_back( KEYWORD_FOR1->loc.front() );
    return t;
}
%       ;


// Return from function statement
% Return : KEYWORD_RETURN Expression SEMICOLON
{   token t(tkn_Return);
    t.tree.push_back(ctx.makeReturn(
      Expression2->tree.front(),
      KEYWORD_RETURN1->loc.front()
    ) );
    t.loc.push_back( KEYWORD_RETURN1->loc.front() );
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
    t.tree.push_back(ctx.makeBinary("LOGICAL_AND",
        Expr201->tree.front(),
        Expr303->tree.front(),
        AND2->loc.front() ));
    t.loc.push_back( AND2->loc.front() );
    return t;
}
%            | Expr20 OR  Expr30
{   token t(tkn_Expr20);
    t.tree.push_back(ctx.makeBinary("LOGICAL_OR",
        Expr201->tree.front(),
        Expr303->tree.front(),
        OR2->loc.front() ));
    t.loc.push_back( OR2->loc.front() );
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
    t.tree.push_back(ctx.makeBinary("EQUAL",
        Expr401->tree.front(),
        Expr403->tree.front(),
        EQUAL2->loc.front() ));
    t.loc.push_back( EQUAL2->loc.front() );
    return t;
}
%            | Expr40 NEQUAL    Expr40
{   token t(tkn_Expr30);
    t.tree.push_back(ctx.makeBinary("NEQUAL",
        Expr401->tree.front(),
        Expr403->tree.front(),
        NEQUAL2->loc.front() ));
    t.loc.push_back( NEQUAL2->loc.front() );
    return t;
}
%            | Expr40 LESS      Expr40
{   token t(tkn_Expr30);
    t.tree.push_back(ctx.makeBinary("LESS",
        Expr401->tree.front(),
        Expr403->tree.front(),
        LESS2->loc.front() ));
    t.loc.push_back( LESS2->loc.front() );
    return t;
}
%            | Expr40 GREATER   Expr40
{   token t(tkn_Expr30);
    t.tree.push_back(ctx.makeBinary("GREATER",
        Expr401->tree.front(),
        Expr403->tree.front(),
        GREATER2->loc.front() ));
    t.loc.push_back( GREATER2->loc.front() );
    return t;
}
%            | Expr40 LESSEQ    Expr40
{   token t(tkn_Expr30);
    t.tree.push_back(ctx.makeBinary("LESSEQ",
        Expr401->tree.front(),
        Expr403->tree.front(),
        LESSEQ2->loc.front() ));
    t.loc.push_back( LESSEQ2->loc.front() );
    return t;
}
%            | Expr40 GREATEREQ Expr40
{   token t(tkn_Expr30);
    t.tree.push_back(ctx.makeBinary("GREATEREQ",
        Expr401->tree.front(),
        Expr403->tree.front(),
        GREATEREQ2->loc.front() ));
    t.loc.push_back( GREATEREQ2->loc.front() );
    return t;
}
%            | Expr40
{   Expr401->type = tkn_Expr30;
    return Expr401;
}
%            ;

% Expr40     : Expr40 ADD Expr50
{   token t(tkn_Expr40);
    t.tree.push_back(ctx.makeBinary("ADD",
        Expr401->tree.front(),
        Expr503->tree.front(),
        ADD2->loc.front() ));
    t.loc.push_back( ADD2->loc.front() );
    return t;
}
%            | Expr40 SUB Expr50
{   token t(tkn_Expr40);
    t.tree.push_back(ctx.makeBinary("SUB",
        Expr401->tree.front(),
        Expr503->tree.front(),
        SUB2->loc.front() ));
    t.loc.push_back( SUB2->loc.front() );
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
    t.tree.push_back(ctx.makeBinary("MULT",
        Expr501 ->tree.front(),
        Expr1003->tree.front(),
        MULT2->loc.front() ));
    t.loc.push_back( MULT2->loc.front() );
    return t;
}
%            | Expr50 DIV  Expr100
{   token t(tkn_Expr50);
    t.tree.push_back(ctx.makeBinary("DIV",
        Expr501 ->tree.front(),
        Expr1003->tree.front(),
        DIV2->loc.front() ));
    t.loc.push_back( DIV2->loc.front() );
    return t;
}
%            | Expr50 MOD  Expr100
{   token t(tkn_Expr50);
    t.tree.push_back(ctx.makeBinary("MOD",
        Expr501 ->tree.front(),
        Expr1003->tree.front(),
        MOD2->loc.front() ));
    t.loc.push_back( MOD2->loc.front() );
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
    t.tree.push_back(ctx.makeInt(LITERAL_INT1->value_int.front(), LITERAL_INT1->loc.front()));
    t.loc.push_back( LITERAL_INT1->loc.front() );
    return t;
}
%            | LITERAL_DOUBLE
{   token t(tkn_Expr100);
    t.tree.push_back(ctx.makeDouble(LITERAL_DOUBLE1->value_double.front(), LITERAL_DOUBLE1->loc.front()));
    t.loc.push_back( LITERAL_DOUBLE1->loc.front() );
    return t;
}
%            | LITERAL_BOOL
{   token t(tkn_Expr100);
    t.tree.push_back(ctx.makeBool(LITERAL_BOOL1->value_bool.front(), LITERAL_BOOL1->loc.front()));
    t.loc.push_back( LITERAL_BOOL1->loc.front() );
    return t;
}
%            | LITERAL_STRING
{   token t(tkn_Expr100);
    t.tree.push_back(ctx.makeString(LITERAL_STRING1->value_string.front(), LITERAL_STRING1->loc.front()));
    t.loc.push_back( LITERAL_STRING1->loc.front() );
    return t;
}
%            | IDENTIFIER
{   token t(tkn_Expr100);
    t.tree.push_back(ctx.makeVariableOcc(IDENTIFIER1->id.front(), IDENTIFIER1->loc.front()));
    t.loc.push_back( IDENTIFIER1->loc.front() );
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
    t.tree.push_back(ctx.makeCall( IDENTIFIER1->id.front(),
                                   ArgList3->arglist.front(),
                                   LPAR2->loc.front()
    ));
    t.loc.push_back( LPAR2->loc.front() );
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
    t.protoarglist.push_back( std::list<std::pair<std::string,ccoscope::Type>>() );
    return t;
}
%          ;

% ProtoArgListL : ProtoArgListL TypedIdentifier COMMA
{   ProtoArgListL1->protoarglist.front().push_back( TypedIdentifier2->typedident.front() );
    return ProtoArgListL1;
}
%              |
{   token t(tkn_ProtoArgListL);
    t.protoarglist.push_back( std::list<std::pair<std::string,ccoscope::Type>>() );
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
    t.arglist.push_back( std::vector<ccoscope::Expr>() );
    return t;
}
%          ;

% ArgListL : ArgListL Expression COMMA
{   ArgListL1->arglist.front().push_back( Expression2->tree.front() );
    return ArgListL1;
}
%              |
{   token t(tkn_ArgListL);
    t.arglist.push_back( std::vector<ccoscope::Expr>() );
    return t;
}
%              ;
