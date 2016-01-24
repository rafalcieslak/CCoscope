
    #include "token.h"
    #include "tokenizer.h"

    int curr_line, curr_column, line, column;
    #define YY_DECL int custom_yylex(std::list<std::pair<token,fileloc>> &target)

    // Called on each target, even if no action is assigned or it's implicitly dropped
    #define YY_USER_ACTION \
    line = curr_line; \
    column = curr_column; \
    for(int i = 0; yytext[i] != '\0'; i++) { \
        if(yytext[i] == '\n') { \
            curr_line++; \
            curr_column = 0; \
        } \
        else { \
            curr_column++; \
        } \
    }

    #define PUTTOK(tokentype) target.push_back(std::make_pair(tokentype,fileloc(line,column)))
    #define TOKADD(field, value) target.back().first.field.push_back(value)

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
{basetype}      PUTTOK(tkn_TYPE); TOKADD(id,yytext);
extern          PUTTOK(tkn_KEYWORD_EXTERN);
fun             PUTTOK(tkn_KEYWORD_FUN);
var             PUTTOK(tkn_KEYWORD_VAR);
return          PUTTOK(tkn_KEYWORD_RETURN);
while           PUTTOK(tkn_KEYWORD_WHILE);
if              PUTTOK(tkn_KEYWORD_IF);
else            PUTTOK(tkn_KEYWORD_ELSE);
for             PUTTOK(tkn_KEYWORD_FOR);
break           PUTTOK(tkn_KEYWORD_BREAK);
continue        PUTTOK(tkn_KEYWORD_CONTINUE);
:=              PUTTOK(tkn_ASSIGN);
{int}/{literalend}   PUTTOK(tkn_LITERAL_INT);   TOKADD(value_int  , std::stod(yytext));
{float}/{literalend} PUTTOK(tkn_LITERAL_FLOAT); TOKADD(value_float, std::stof(yytext));
:               PUTTOK(tkn_COLON);
==              PUTTOK(tkn_EQUAL);
!=              PUTTOK(tkn_NEQUAL);
\>=             PUTTOK(tkn_GREATEREQ);
\<=             PUTTOK(tkn_LESSEQ);
\<              PUTTOK(tkn_LESS);
\>              PUTTOK(tkn_GREATER);
&&              PUTTOK(tkn_AND);
\|\|            PUTTOK(tkn_OR);
\+              PUTTOK(tkn_ADD);
-               PUTTOK(tkn_SUB);
\*              PUTTOK(tkn_MULT);
\/              PUTTOK(tkn_DIV);
%               PUTTOK(tkn_MOD);
;               PUTTOK(tkn_SEMICOLON);
,               PUTTOK(tkn_COMMA);
\(              PUTTOK(tkn_LPAR);
\)              PUTTOK(tkn_RPAR);
\{              PUTTOK(tkn_LBRACKET);
\}              PUTTOK(tkn_RBRACKET);
\|              PUTTOK(tkn_PIPE);

{identifier}    PUTTOK(tkn_IDENTIFIER); TOKADD(id, yytext);
{whitespace}    {}
%%


void lexer(FILE* input,std::list<std::pair<token,fileloc>> &target){
    yyin = input;
    column = line = curr_column = curr_line = 1;
    custom_yylex(target);
}
