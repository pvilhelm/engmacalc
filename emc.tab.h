/* A Bison parser, made by GNU Bison 3.7.5.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

#ifndef YY_YY_EMC_TAB_H_INCLUDED
# define YY_YY_EMC_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 1
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    YYEOF = 0,                     /* "end of file"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    NUMBER = 258,                  /* NUMBER  */
    NAME = 259,                    /* NAME  */
    TYPENAME = 260,                /* TYPENAME  */
    ESC_STRING = 261,              /* ESC_STRING  */
    EOL = 262,                     /* EOL  */
    IF = 263,                      /* IF  */
    DO = 264,                      /* DO  */
    END = 265,                     /* END  */
    ELSE = 266,                    /* ELSE  */
    WHILE = 267,                   /* WHILE  */
    ENDOFFILE = 268,               /* ENDOFFILE  */
    FUNC = 269,                    /* FUNC  */
    ELSEIF = 270,                  /* ELSEIF  */
    ALSO = 271,                    /* ALSO  */
    RETURN = 272,                  /* RETURN  */
    STRUCT = 273,                  /* STRUCT  */
    TYPE = 274,                    /* TYPE  */
    CLINKAGE = 275,                /* CLINKAGE  */
    NAMESPACE = 276,               /* NAMESPACE  */
    USING = 277,                   /* USING  */
    IMPORT = 278,                  /* IMPORT  */
    OR = 279,                      /* OR  */
    NOR = 280,                     /* NOR  */
    XOR = 281,                     /* XOR  */
    XNOR = 282,                    /* XNOR  */
    AND = 283,                     /* AND  */
    NAND = 284,                    /* NAND  */
    NOT = 285,                     /* NOT  */
    CMP = 286,                     /* CMP  */
    LEQ = 287,                     /* LEQ  */
    GEQ = 288,                     /* GEQ  */
    EQU = 289,                     /* EQU  */
    NEQ = 290,                     /* NEQ  */
    INTDIV = 291,                  /* INTDIV  */
    UMINUS = 292                   /* UMINUS  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 20 "emc.y"

    ast_node *node;
    std::string *s;

#line 106 "emc.tab.h"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif

/* Location type.  */
#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE YYLTYPE;
struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
};
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif



int yyparse (yyscan_t scanner);

#endif /* !YY_YY_EMC_TAB_H_INCLUDED  */
