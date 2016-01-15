    #include "token.h"
    #define YY_DECL int custom_yylex(std::list<token> &target)

digit    [0-9]
alpha    [a-zA-z]
alphanum {digit}|{alpha}

basetype int|bool|double

/* Floats */
exponent	[eE]-?{digit}+
base	({digit}+"."{digit}*)|({digit}*"."{digit}+)
float	-?{base}{exponent}?

/* Integers */
int	    -?{digit}+

literalend  [^0-9a-zA-Z.]

/* Identifiers */
identifier_char	{alphanum}|"_"
identifier	{alpha}({identifier_char})*

/* Comments */
ol_comment	"//"[^\n]*"\n"
bl_comment	"/*"[^*]*("*"+)([^*/][^*]*"*"+)*"/"
comment		{ol_comment}|{bl_comment}

/* Whitespaces */
whitechar [ \n\t]
whitespace {whitechar}+

%%
{comment}       {}
{basetype}      target.push_back(tkn_TYPE); target.back().id.push_back(yytext);
extern          target.push_back(tkn_KEYWORD_EXTERN);
fun             target.push_back(tkn_KEYWORD_FUN);
var             target.push_back(tkn_KEYWORD_VAR);
return          target.push_back(tkn_KEYWORD_RETURN);
while           target.push_back(tkn_KEYWORD_WHILE);
:=              target.push_back(tkn_ASSIGN);
{int}/{literalend}   target.push_back(tkn_LITERAL_INT);   target.back().value_int  .push_back(std::stod(yytext));
{float}/{literalend} target.push_back(tkn_LITERAL_FLOAT); target.back().value_float.push_back(std::stof(yytext));
:               target.push_back(tkn_COLON);
==              target.push_back(tkn_EQUAL);
!=              target.push_back(tkn_NEQUAL);
\+              target.push_back(tkn_ADD);
-               target.push_back(tkn_SUB);
\*              target.push_back(tkn_MULT);
\/              target.push_back(tkn_DIV);
%               target.push_back(tkn_MOD);
;               target.push_back(tkn_SEMICOLON);
,               target.push_back(tkn_COMMA);
\(              target.push_back(tkn_LPAR);
\)              target.push_back(tkn_RPAR);
\{              target.push_back(tkn_LBRACKET);
\}              target.push_back(tkn_RBRACKET);

{identifier}    target.push_back(tkn_IDENTIFIER); target.back().id.push_back(yytext);
{whitespace}    {}
%%