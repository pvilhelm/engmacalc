%{
#include <string>
#include <cctype>
#include "emc.hh"
#include "emc.tab.h"

static void update_loc(struct YYLTYPE * yylloc_param, char *yytext_arg);
void yyerror(const char *s);

#define YY_USER_ACTION update_loc(yylloc_param, yytext);

/*typedef struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
} YYLTYPE;*/

%}

%option noyywrap nodefault
%option bison-bridge reentrant bison-locations

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
"ELSE" return ELSE;
"WHILE" return WHILE;
"FUNC" return FUNC;    

 /* Symbol names */
[a-z][a-z0-9\-_]*    { yylval->s = new std::string{yytext}; return NAME; }

 /* Types */
[A-Z][a-z0-9\-_]*     { yylval->s = new std::string{yytext}; return TYPENAME; }

[0-9]+"."[0-9]*{EXP}? |
"."?[0-9]+{EXP}?        { yylval->s = new std::string{yytext}; return NUMBER; }

\"(\\.|[^"\\])*\"		{ 
							yylval->s = new std::string{yytext + 1, strlen(yytext) - 2}; 
							deescape_string(*yylval->s);
							return ESC_STRING; 
						}

"//".*
{WS}                    /* ignore white space */
\\\n                    /* ignore line continuation */
 /*^{WS}*\n*/                /* ignore empty new lines */
"\r\n"         			{ return EOL; }
"\n"         			{ return EOL; }
<<EOF>>                 { return ENDOFFILE; }
.                       { fprintf(stderr, "Mystery character %c %i\n", *yytext, (int)*yytext); }

%%



static void update_loc(struct YYLTYPE * yylloc_param, char *yytext_arg) {
	static int curr_line = 1;
	static int curr_col = 1;
	
	if (yylloc_param->first_line == 0) {
		curr_line = 1;
		curr_col = 1;
	}
	yylloc_param->first_line = curr_line;
	yylloc_param->first_column = curr_col;

	{
		char *s;
		for (s = yytext_arg; *s != '\0'; s++) {
			if (*s == '\n') {
				curr_line++;
				curr_col = 1;
			} else {
				curr_col++;
			}
		}
	}

	yylloc_param->last_line = curr_line;
	yylloc_param->last_column = curr_col - 1;
}

