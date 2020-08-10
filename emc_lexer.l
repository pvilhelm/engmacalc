
%option noyywrap nodefault yylineno

%{

#include <string>

#include "emc.hh"
#include "emc.tab.h"

void yyerror(const char *s);

%}

    /* float exponent */
EXP([Ee][-+]?[0-9]+)
WS([ \t])
%%

    /* Single character operators */

"+"({WS}*\n)? |
"-"({WS}*\n)? |
"*"({WS}*\n)? |
"/"({WS}*\n)? |
"="({WS}*\n)? |
"|" |
","({WS}*\n)? |
";" |
"{" |
"}" |
"(" |
")" |
"^" |
"<"{WS}*\n? |
">"{WS}*\n? { return yytext[0]; }

"<=>"({WS}*\n)? return CMP;
"<="({WS}*\n)?  return LEQ;
">="({WS}*\n)?  return GEQ;
"=="({WS}*\n)?  return EQU;
"!="({WS}*\n)?  return NEQ;

"IF" return IF;
"DO" return DO;
"END" return END;

 /* Symbol names */
[a-z][a-z0-9\-_]*    { yylval.s = new std::string{yytext}; return NAME; }

 /* Types */
[A-Z][a-z0-9\-_]*     { yylval.s = new std::string{yytext}; return TYPENAME; }

[0-9]+"."[0-9]*{EXP}? |
"."?[0-9]+{EXP}?        { yylval.s = new std::string{yytext}; return NUMBER; }

"//".*
{WS}                    /* ignore white space */
\\\n                    /* ignore line continuation */
"\n"                    { return EOL; }

.                       { fprintf(stderr, "Mystery character %c\n", *yytext); }

%%
