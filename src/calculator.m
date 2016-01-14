
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
%attribute t             tree
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

%global memory varstore
   // Contains the stored variables.

#include "varstore.h"
#include "tree.h"

#include "assert.h"
#include <math.h>
%intokenheader #include "tree.h"


void FinalizeTree(tree t, varstore& mem){
  std::cout << "Printing tree: " << std::endl;
  std::cout << t << std::endl;
  std::cout << "Evaluating tree: " << std::endl;
  try{
	std::cout << t.eval(mem) << std::endl;
  }catch(std::runtime_error ex){
	std::cout << "Evaluation failed: " << ex.what() << std::endl;
  }
}

% Start : Session EOF
   std::cout << "bye bye\n";
%   ;

% Session : Session Command
%         |
%         ;


% Command : E SEMICOLON
{
  FinalizeTree( E1->t.front(), memory);
}
%         | IDENTIFIER BECOMES E SEMICOLON
{
  tree t(":ASSIGN", {tree(IDENTIFIER1->id.front()), E3->t.front()});
  FinalizeTree(t, memory);
}
%         | _recover SEMICOLON
{
  std::cout << "Recovered from error" << std::endl;
}
%         ;

% E   : E PLUS F
{
  token tk(tkn_E);
  tk.t.push_back(tree(":ADD", {E1->t.front(), F3->t.front()}));
  return tk;
}
%     | E MINUS F
{
  token tk(tkn_E);
  tk.t.push_back(tree(":SUB", {E1->t.front(), F3->t.front()}));
  return tk;
}
%     | F
{
  F1->type = tkn_E;
  return F1;
}
%     ;

% F   : F TIMES G
{
  token tk(tkn_F);
  tk.t.push_back(tree(":MUL", {F1->t.front(), G3->t.front()}));
  return tk;
}
%     | F DIVIDES G
{
  token tk(tkn_F);
  tk.t.push_back(tree(":DIV", {F1->t.front(), G3->t.front()}));
  return tk;
}
%     | G
{
  G1->type = tkn_F;
  return G1;
}
%     ;


%  G : MINUS G
{
  token tk(tkn_G);
  tk.t.push_back(tree(":MINUS",{G2->t.front()}));
  return tk;
}
%    | PLUS G
{
  token tk(tkn_G);
  tk.t.push_back(tree(":PLUS",{G2->t.front()}));
  return tk;
}
%    | H
{
  H1->type = tkn_G;
  return H1;
}
%    ;


% H   : H FACTORIAL
{
  token tk(tkn_H);
  tk.t.push_back(tree(":FACT",{H1->t.front()}));
  return tk;
}
%     | LPAR E RPAR
{
  E2->type = tkn_H;
  return E2;
}
%     | IDENTIFIER
{
  token tk(tkn_H);
  tk.t.push_back(tree(":IDENTIFIER",{IDENTIFIER1->id.front()}));
  return tk;
}
%     | NUMBER
{
  token tk(tkn_H);
  tk.t.push_back(tree(":NUMBER",{NUMBER1->value.front()}));
  return tk;
}
%     | IDENTIFIER LPAR LISTARGS RPAR
{
  token tk(tkn_H);
  std::vector<tree> subtrees = {IDENTIFIER1->id.front()};
  subtrees.insert(subtrees.end(),LISTARGS3->t.begin(),LISTARGS3->t.end());
  tk.t.push_back(tree(":EVAL",subtrees));
  return tk;
}
%     ;


% LISTARGS : E
{
  E1->type = tkn_LISTARGS;
  return E1;
}
%          | LISTARGS COMMA E
{
  LISTARGS1->t.push_back(E3->t.front());
  return LISTARGS1;
}
%          ;
