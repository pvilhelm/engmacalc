
%option noyywrap nodefault yylineno

%{

#include <string>

#include "emc.hh"
#include "emc.tab.h"

void yyerror(const char *s);

%}

    /* float exponent */
EXP([Ee][-+]?[0-9]+)

%%

    /* Single character operators */
"+" |
"-" |
"*" |
"/" |
"=" |
"|" |
"," |
";" |
"{" |
"}" |
"(" |
")" |
"<" |
">" { return yytext[0]; }

"<=>" return CMP;
"<="  return LEQ;
">="  return GEQ;
"=="  return EQU;
"!="  return NEQ;

        /* names */
[a-z][a-z0-9\-_]*    { yylval.s = new std::string{yytext}; return NAME; }

[0-9]+"."[0-9]*{EXP}? |
"."?[0-9]+{EXP}?        { yylval.s = new std::string{yytext}; return NUMBER; }

"//".*
[ \t]                   /* ignore white space */
\\\n                    /* ignore line continuation */
"\n"                    { return EOL; }

.                       { fprintf(stderr, "Mystery character %c\n", *yytext); }

%%
