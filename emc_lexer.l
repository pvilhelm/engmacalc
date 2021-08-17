%{
#include <string>
#include <cctype>
#include "emc.hh"
#include "emc.tab.h"

static void update_loc(struct YYLTYPE * yylloc_param, char *yytext_arg);
void yyerror(const char *s);

#define YY_USER_ACTION update_loc(yylloc_param, yytext);

int n_nested_comments = 0;

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
EXP [Ee][-+]?[0-9]+
WS [ \t]
ENDLN "\r\n"|"\n"

%x IN_COMMENT

%%

    /* Single character operators */

"+"({WS}*{ENDLN})? |
"-"({WS}*{ENDLN})? |
"*"({WS}*{ENDLN})? |
"/"({WS}*{ENDLN})? |
"="({WS}*{ENDLN})? |
"&"({WS}*{ENDLN})? |
"@"({WS}*{ENDLN})? |
"%"({WS}*{ENDLN})? |
"|" |
","({WS}*{ENDLN})? |
"."({WS}*{ENDLN})? |
";" |
"{" |
"}" |
"(" |
")" |
"^" |
"<"{WS}*{ENDLN}? |
">"{WS}*{ENDLN}? { return yytext[0]; }

"<=>"({WS}*{ENDLN})? return CMP;
"<="({WS}*{ENDLN})?  return LEQ;
">="({WS}*{ENDLN})?  return GEQ;
"=="({WS}*{ENDLN})?  return EQU;
"!="({WS}*{ENDLN})?  return NEQ;

"IF" return IF;
"DO" return DO;
"END" return END;
"ELSE" return ELSE;
"WHILE" return WHILE;
"FUNC" return FUNC;  
"ALSO" return ALSO;   
"RETURN" return RETURN;
"TYPE" return TYPE;
"STRUCT" return STRUCT;
"AND" return AND;
"OR" return OR;
"XOR" return XOR;
"XNOR" return XNOR;
"NAND" return NAND;
"NOR" return NOR;
"NOT" return NOT;
"NAMESPACE" return NAMESPACE;
"USING" return USING;
"IMPORT" return IMPORT;

"c::" return CLINKAGE;

 /* Symbol names */
[a-z][a-z0-9\-_]*    { yylval->s = new std::string{yytext}; return NAME; }

 /* Types */
[A-Z][a-z0-9\-_]*     { yylval->s = new std::string{yytext}; return TYPENAME; }

[0-9]+"."[0-9]*{EXP}? |
"."[0-9]+{EXP}?         { 
							yylval->node = new ast_node_double_literal{std::string{yytext}};
							return NUMBER; 
						}

	/* TODO: Borde göra egen parsning för att tex. tillåta 1'000'000 och 09 som inte 
	 * oktal ... */
[0-9]+ |
"0"[xX][0-9a-fA-F]+     { 
							yylval->node = new ast_node_int_literal{std::string{yytext}};
							return NUMBER; 
						}


\"(\\.|[^"\\])*\"		{ 
							yylval->s = new std::string{yytext + 1, strlen(yytext) - 2}; 
							deescape_string(*yylval->s);
							return ESC_STRING; 
						}

<INITIAL>"/*"         {n_nested_comments++; BEGIN(IN_COMMENT);}
<IN_COMMENT>"/*"      {n_nested_comments++;}
<IN_COMMENT>"*/"      {n_nested_comments--; if (n_nested_comments == 0) BEGIN(INITIAL);}
<IN_COMMENT>[^/*\n]+  // eat comment in chunks
<IN_COMMENT>"*"       // eat the lone star
<IN_COMMENT>\n         
<IN_COMMENT>"/" 

"//".*
{WS}                    /* ignore white space */
\\\n                    /* ignore line continuation */
 /*^{WS}*\n*/           /* ignore empty new lines */
{ENDLN}        			{ return EOL; }
 
<<EOF>>                 { return ENDOFFILE; }
.                       { fprintf(stderr, "Mystery character %c %i\n", *yytext, (int)*yytext); }

%%

int curr_line = 1;
int curr_col = 1;

static void update_loc(struct YYLTYPE * yylloc_param, char *yytext_arg) {
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

