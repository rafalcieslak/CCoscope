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

/* Operators */
operator_sc  "+"|"-"|">"|"<"|"/"|"*"|","|";"|":"
operator_mc  ":="|"=="|"<="|">="|"&&"|"||"
operator     {operator_sc}|{operator_mc}

/* Identifiers */
identifier_char	{alphanum}|"_"
identifier	{alpha}({identifier_char})*

/* Comments */
ol_comment	"//"[^\n]*"\n"
bl_comment	"/*"[^*]*("*"+)([^*/][^*]*"*"+)*"/"
comment		{ol_comment}|{bl_comment}


%%
{basetype}      target.push_back(tkn_TYPE); target.back().id.push_back(yytext);
extern          target.push_back(tkn_KEYWORD_EXTERN);
fun             target.push_back(tkn_KEYWORD_FUN);
:               target.push_back(tkn_COLON);
;               target.push_back(tkn_SEMICOLON);
\(              target.push_back(tkn_LPAR);
\)              target.push_back(tkn_RPAR);
\{              target.push_back(tkn_LBRACKET);
\}              target.push_back(tkn_RBRACKET);

{identifier}    target.push_back(tkn_IDENTIFIER); target.back().id.push_back(yytext);

%%

void lexme(std::list<token> &target_list)
{
  custom_yylex(target_list);
}
