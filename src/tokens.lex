
/* NOTE:
 * The rules in this file IN NO WAY correspond to the desired language specs.
 * I just imported it as an example to prepare build scripts, and will edit it * later on.
*/

digit	[0-9]
alpha	[a-zA-Z]
alphanum	{digit}|{alpha}

/* Floats */
exponent	[eE]-?{digit}+
base	({digit}+"."{digit}*)|({digit}*"."{digit}+)
float	-?{base}{exponent}?

/* Integers */
int	    -?{digit}+

literalend  [^0-9a-zA-Z.]

/* Strings */
string	   \"([^"]*)\"

/* Reserved words */
reserved    if|while|do|struct|class|int|double|else

/* Identifiers */
identifier_char	{alphanum}|"_"
identifier	{alpha}({identifier_char})*

/* Operators */
operator_sc  "+"|"-"|">"|"<"|"/"|"*"|"="|"%"|"&"|"^"|"|"|","|";"
operator_dc  "=="|"++"|"--"|"<="|">="|"+="|"-="|"*="|"/="|"%="|"&&"|"||"|"^="|"&="|">>"|"<<"
operator     {operator_sc}|{operator_dc}

/* Brackets */
bracket	    [\{\}]
parent	    [\(\)]

/* Whitespaces */
whitechar [ \n\t]
whitespace {whitechar}+

/* Comments */
ol_comment	"//"[^\n]*"\n"
bl_comment	"/*"[^*]*("*"+)([^*/][^*]*"*"+)*"/"
comment		{ol_comment}|{bl_comment}


%%
{comment}	printf("token: comment %s\n", yytext);
{parent}	printf("token: parent %s\n", yytext);
{bracket}	printf("token: bracket %s\n", yytext);
{float}/{literalend}	printf("token: float %s\n", yytext);
{int}/{literalend}	printf("token: int %s\n", yytext);
{string}	printf("token: string %s\n", yytext);
{reserved}	printf("token: reserved %s\n", yytext);
{identifier}	printf("token: identifier %s\n", yytext);
{operator}	printf("token: operator %s\n", yytext);
{whitespace}	{}
.		printf("token: unknown %s\n", yytext);
%%

void lexme()
{
  yylex();
}
