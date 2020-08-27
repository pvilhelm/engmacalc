
%code top {
#include <cstdio>
#include <cstdlib>
#include <iostream>

#include "emc.hh"

ast_node *ast_root; /* Bison parser writes answer to this. */
bool parsed_eol = false;

typedef void* yyscan_t;
}

%define api.pure
%lex-param {yyscan_t scanner}
%parse-param {yyscan_t scanner}
%locations

%union {
    ast_node *node;
    std::string *s;
}

%token <node> NUMBER
%token <s> NAME
%token <s> TYPENAME 
%token <s> ESC_STRING

%token EOL IF DO END ELSE WHILE ENDOFFILE FUNC ELSEIF ALSO RETURN

%right '='
%left CMP LEQ GEQ EQU NEQ '>' '<'
%left '+' '-'
%left '*' '/'
%right '^'

%nonassoc DO
%nonassoc END        
%nonassoc '|' UMINUS

%start program

%type <node> exp cmp_exp e se cse exp_list code_block arg_list param_list vardef elseif_list sl_elseif_list vardef_list

%define parse.trace
    

%code {
int yylex (YYSTYPE * yylval_param, YYLTYPE * yylloc_param , void * yyscanner);
int yyerror(struct YYLTYPE * yylloc_param, void *scanner, const char *s);
}    
    
    
%%

program:              
		/*| program*/ 
     	 cse
                        { 	
                        	ast_root = $1;
                        	YYACCEPT;
                        }
     	| ENDOFFILE { ast_root = nullptr; parsed_eol = true; YYACCEPT;}
     	;

    /* List of expressions */
exp_list: cse           		{ast_node* p;
                                 if ($1) /* If $1 is null the cse was an EOL */
                                    p = new ast_node_explist{$1};
                                 else
                                    p = new ast_node_explist{}; 
                                 $$ = p;}
     	| exp_list cse 	    {
	                            if ($2) {
	                               auto p_exl = static_cast<ast_node_explist*>($1);
	                               p_exl->append_node($2);
                                }
	                            $$ = $1; 
                        	}
     	;                       

/* Closed statement expression. */
cse: EOL                    {$$ = 0;}
    |se EOL                 {$$ = $1;}

 /* Statement expression */
se: e                      {$$ = $1;}
    | RETURN e             {$$ = new ast_node_return{$2};}
    /* IFs with ELSE:s are a headache to make grammar for. So ... */
    /* These with se are for one line if:s */
	| IF e DO se END { $$ = new ast_node_if{$2, $4};}
    | IF e DO se ELSE DO se END { $$ = new ast_node_if{$2, $4, $7};}
    | IF e DO se ELSE sl_elseif_list END { $$ = new ast_node_if{$2, $4, $6};}
        /* The also statement expression is executed if atleast of IF condition was true. */
    | IF e DO se ELSE sl_elseif_list ALSO DO se END { $$ = new ast_node_if{$2, $4, $6, $9};}
    | IF e DO se ELSE sl_elseif_list ELSE DO se END 
                            { 
                                $$ = new ast_node_if{$2, $4, $6};
                                auto p = dynamic_cast<ast_node_if*>($6);
                                p->append_linked_if($9);
                            }    
    | IF e DO se ELSE sl_elseif_list ALSO DO se ELSE DO se END 
                            { 
                                $$ = new ast_node_if{$2, $4, $9};
                                auto p = dynamic_cast<ast_node_if*>($6);
                                p->append_linked_if($12);
                            }  
    /* Multiline if:s with ELSE IFs */
    | IF e DO exp_list END { $$ = new ast_node_if{$2, $4};}
    | IF e DO exp_list ELSE DO exp_list END { $$ = new ast_node_if{$2, $4, $7};}
    | IF e DO exp_list ELSE elseif_list END { $$ = new ast_node_if{$2, $4, $6};}
    | IF e DO exp_list ELSE elseif_list ALSO DO exp_list END 
                                            { $$ = new ast_node_if{$2, $4, $6, $9};}
    | IF e DO exp_list ELSE elseif_list ELSE DO exp_list END
                            { 
                                $$ = new ast_node_if{$2, $4, $6};
                                auto p = dynamic_cast<ast_node_if*>($6);
                                p->append_linked_if($9);
                            } 
    | IF e DO exp_list ELSE elseif_list ALSO DO exp_list ELSE DO exp_list END
                            { 
                                $$ = new ast_node_if{$2, $4, $6, $9};
                                auto p = dynamic_cast<ast_node_if*>($6);
                                p->append_linked_if($12);
                            }                             

    | code_block
    | FUNC vardef_list '=' NAME '(' vardef_list ')' code_block
                            {
                                auto p = new ast_node_funcdec{$6, $8, *$4, "", $2};
                                delete $4;
                                $$ = p;
                            }
    | WHILE e DO exp_list END 
    						{ $$ = new ast_node_while{$2, $4};}
    | WHILE e DO exp_list ELSE exp_list END 
    						{ $$ = new ast_node_while{$2, $4, $6};}
    | vardef '=' se         { $$ = $1; dynamic_cast<ast_node_def*>($1)->value_node = $3;}   						
    ;
    
/* A list of ifelses */
elseif_list: IF e DO exp_list              { $$ = new ast_node_if{$2, $4, 0};}
       | elseif_list ELSE IF e DO exp_list { 
                                             auto p = new ast_node_if{$4, $6, 0};
                                             auto pp = dynamic_cast<ast_node_if*>($1);
                                             pp->append_linked_if(p);
                                             $$ = $1;
                                           }
/* A list of ifelsies for one liner. */                                         
sl_elseif_list: IF e DO se                 { $$ = new ast_node_if{$2, $4, 0};}
       | sl_elseif_list ELSE IF e DO se    { 
                                             auto p = new ast_node_if{$4, $6, 0};
                                             auto pp = dynamic_cast<ast_node_if*>($1);
                                             pp->append_linked_if(p);
                                             $$ = $1;
                                           }   
                                           
vardef_list: vardef                         {
                                                auto p = new ast_node_vardef_list;
                                                p->append($1); $$ = p;
                                            }
           | vardef_list ',' vardef         {
                                                auto p = dynamic_cast<ast_node_vardef_list*>($$);
                                                p->append($3);
                                            }                                    
 
vardef: TYPENAME NAME     {$$ = new ast_node_def{*$1, *$2, nullptr}; delete $1; delete $2; }    

code_block: DO exp_list END     { $$ = new ast_node_doblock{$2};}





param_list: NAME                        {
                                            auto p = new ast_node_parlist;
                                            $$ = p;
                                            p->append_parameter(*$1);
                                            delete $1; 
                                        }
        | param_list ',' NAME           {
                                            auto p = static_cast<ast_node_parlist*>($$);
                                            p->append_parameter(*$3); delete $3;
                                        }

arg_list: e                 {
                                auto p = new ast_node_arglist;
                                p->append_arg($1); $$ = p;
                            }
        | arg_list ',' e    {
                                auto p = static_cast<ast_node_arglist*>($$);
                                p->append_arg($3);
                            }
    
 /* Top expression */
e: exp
   | cmp_exp
   | e '=' e                 {$$ = new ast_node_assign{$1, $3};}
   ;

 /* Not compare or assign expressions */
exp: exp '+' exp             {$$ = new ast_node_add{$1, $3};}
     | exp '-' exp           {$$ = new ast_node_sub{$1, $3};}
     | exp '*' exp           {$$ = new ast_node_mul{$1, $3};}
     | exp '/' exp           {$$ = new ast_node_rdiv{$1, $3};}
     | exp CMP exp           {$$ = new ast_node_cmp{$1, $3};}
     | '-' exp %prec UMINUS  {$$ = new ast_node_uminus{$2};}
     | '(' exp ')'           {$$ = $2;}
     | '|' exp '|'           {$$ = new ast_node_abs{$2};}
     | exp '^' exp           {$$ = new ast_node_pow{$1, $3};}
     | NAME                  {$$ = new ast_node_var{*$1, ""}; delete $1;}
     | NUMBER                {$$ = $1;}
     | NAME '(' arg_list ')' {$$ = new ast_node_funccall{"", *$1, $3}; delete $1;}
     | ESC_STRING			 {$$ = new ast_node_string_literal{*$1}; delete $1;}
     ;
  
 /* Compare expressions. */
cmp_exp:  exp '>' exp        {
                                auto gre = new ast_node_gre{$1, $3};
                                $$ = new ast_node_andchain{gre};
                             }
         | exp '<' exp       {
                                auto cmp = new ast_node_les{$1, $3};
                                $$ = new ast_node_andchain{cmp};
                             }
         | exp EQU exp       {
                                auto cmp = new ast_node_equ{$1, $3};
                                $$ = new ast_node_andchain{cmp};
                             }
         | exp LEQ exp       {
                                auto cmp = new ast_node_leq{$1, $3};
                                $$ = new ast_node_andchain{cmp};
                             }
         | exp GEQ exp       {
                                auto cmp = new ast_node_geq{$1, $3};
                                $$ = new ast_node_andchain{cmp};
                             }
         | exp NEQ exp       {
                                auto cmp = new ast_node_neq{$1, $3};
                                $$ = new ast_node_andchain{cmp};
                             }
         | cmp_exp '>' exp   {
                                auto chain = dynamic_cast<ast_node_andchain*>($1);
                                auto gre = new ast_node_gre{nullptr, $3};
                                chain->append_next(gre);
                                $$ = chain;
                             }
         | cmp_exp '<' exp   {
                                auto chain = dynamic_cast<ast_node_andchain*>($1);
                                auto les = new ast_node_les{nullptr, $3};
                                chain->append_next(les);
                                $$ = chain;
                             }
         | cmp_exp EQU exp   {
                                auto chain = dynamic_cast<ast_node_andchain*>($1);
                                auto equ = new ast_node_equ{nullptr, $3};
                                chain->append_next(equ);
                                $$ = chain;
                             }
         | cmp_exp LEQ exp   {
                                auto chain = dynamic_cast<ast_node_andchain*>($1);
                                auto leq = new ast_node_leq{nullptr, $3};
                                chain->append_next(leq);
                                $$ = chain;
                             }
         | cmp_exp GEQ exp   {
                                auto chain = dynamic_cast<ast_node_andchain*>($1);
                                auto geq = new ast_node_geq{nullptr, $3};
                                chain->append_next(geq);
                                $$ = chain;
                             }
         | cmp_exp NEQ exp   {
                                auto chain = dynamic_cast<ast_node_andchain*>($1);
                                auto neq = new ast_node_neq{nullptr, $3};
                                chain->append_next(neq);
                                $$ = chain;
                             }                             
         ;

%%

int yyerror(struct YYLTYPE * yylloc_param, void *scanner, const char *s)
{
    printf("*** Lexical Error %s %d.%d-%d.%d\n", s, 
         yylloc_param->first_line, yylloc_param->first_column, 
         yylloc_param->last_line, yylloc_param->last_column);
         
    return 0;
}
