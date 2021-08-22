
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

%token EOL IF DO END ELSE WHILE ENDOFFILE FUNC ELSEIF ALSO RETURN STRUCT TYPE CLINKAGE NAMESPACE USING IMPORT

%right '='
%left OR NOR XOR XNOR
%left AND NAND
%right NOT
%left CMP LEQ GEQ EQU NEQ '>' '<'
%left '+' '-' 
%left '*' '/' '%' '.' INTDIV
%right '^' '@' '&' 

%nonassoc DO
%nonassoc END        
%nonassoc '|' UMINUS  

%start program

%type <node> exp cmp_exp e se cse exp_list code_block arg_list
%type <node> vardef elseif_list sl_elseif_list vardef_list field_list struct_def
%type <node> ptrdef_list typedotchain typedotnamechain using usingchain

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
                            auto &cu = compilation_units.get_current_compilation_unit();
                        	cu.ast_root = $1;
                        	YYACCEPT;
                        }
        | ENDOFFILE     { 
                            auto &cu = compilation_units.get_current_compilation_unit();
                            cu.ast_root = nullptr;
                            cu.parsed_eol = true; 
                            YYACCEPT;
                        }
     	;

    /* List of expressions */
exp_list: cse           	{
                                ast_node* p;
                                if ($1) /* If $1 is null the cse was an EOL */
                                    p = new ast_node_explist{$1};
                                else
                                    p = new ast_node_explist{}; 
                                $$ = p;
                                $$->loc = @$;
                            }
     	| exp_list cse 	    {
	                            if ($2) {
	                               auto p_exl = static_cast<ast_node_explist*>($1);
	                               p_exl->append_node($2);
                                }
	                            $$ = $1; 
                                $$->loc = @$;
                        	}
     	;                       

/* Closed statement expression. */
cse: EOL                    {$$ = 0;}
    |se EOL                 {$$ = $1; $$->loc = @$;}

 /* Statement expression */
se: e                           {$$ = $1; $$->loc = @$;}
    | using                     {$$ = $1; $$->loc = @$;}
    | TYPE typedotchain '=' struct_def  {
                                            auto t = new ast_node_type{$2, $4};
                                            $$ = t; $$->loc = @$;
                                        }
    | RETURN                    {$$ = new ast_node_return{0}; $$->loc = @$;}
    | RETURN e                  {$$ = new ast_node_return{$2}; $$->loc = @$;}
    /* IFs with ELSE:s are a headache to make grammar for. So ... */
    /* These with se are for one line if:s */
	| IF e DO se END { $$ = new ast_node_if{$2, $4}; $$->loc = @$;}
    | IF e DO se ELSE DO se END { $$ = new ast_node_if{$2, $4, $7}; $$->loc = @$;}
    | IF e DO se ELSE sl_elseif_list END { $$ = new ast_node_if{$2, $4, 0, $6}; $$->loc = @$;}
        /* The also statement expression is executed if atleast of IF condition was true. */
    | IF e DO se ELSE sl_elseif_list ALSO DO se END { $$ = new ast_node_if{$2, $4, 0, $9 ,$6}; $$->loc = @$;}
    | IF e DO se ELSE sl_elseif_list ELSE DO se END 
                            { 
                                $$ = new ast_node_if{$2, $4, $9, 0, $6};
                                $$->loc = @$;
                            }    
    | IF e DO se ELSE sl_elseif_list ALSO DO se ELSE DO se END 
                            { 
                                $$ = new ast_node_if{$2, $4, $12, $9, $6};
                                $$->loc = @$;
                            }  
    /* Multiline if:s with ELSE IFs */
    | IF e DO exp_list END { $$ = new ast_node_if{$2, $4}; $$->loc = @$;}
    | IF e DO exp_list ELSE DO exp_list END { $$ = new ast_node_if{$2, $4, $7}; $$->loc = @$;}
    | IF e DO exp_list ALSO DO exp_list END { $$ = new ast_node_if{$2, $4, 0, $7}; $$->loc = @$;}
    | IF e DO exp_list ALSO DO exp_list ELSE DO exp_list END 
                                            { $$ = new ast_node_if{$2, $4, $10, $7}; $$->loc = @$;}
    | IF e DO exp_list ELSE DO exp_list ALSO DO exp_list END 
                                            { $$ = new ast_node_if{$2, $4, $7, $10}; $$->loc = @$;}
    | IF e DO exp_list ELSE elseif_list END { $$ = new ast_node_if{$2, $4, 0, 0, $6}; $$->loc = @$;}
    | IF e DO exp_list ELSE elseif_list ALSO DO exp_list END 
                                            { $$ = new ast_node_if{$2, $4, 0, $9, $6}; $$->loc = @$;}
    | IF e DO exp_list ELSE elseif_list ELSE DO exp_list END
                            { 
                                $$ = new ast_node_if{$2, $4, $9, 0, $6};
                                $$->loc = @$;
                            } 
    | IF e DO exp_list ELSE elseif_list ALSO DO exp_list ELSE DO exp_list END
                            { 
                                $$ = new ast_node_if{$2, $4, $12, $9, $6};
                                $$->loc = @$;
                            }  
    | IF e DO exp_list ELSE elseif_list ELSE DO exp_list ALSO DO exp_list END
                            { 
                                $$ = new ast_node_if{$2, $4, $9, $12, $6};
                                $$->loc = @$;
                            }                                         

    | code_block
    /* Function definition */
    | FUNC vardef_list '=' typedotnamechain '(' vardef_list ')' code_block
                            {
                                auto p = new ast_node_funcdef{$6, $8, $4, $2};
                                $$ = p; $$->loc = @$;
                            }
    /* Function definition without parameters that returns something */
    | FUNC vardef_list '=' typedotnamechain '(' ')' code_block
                            {
                                auto p = new ast_node_funcdef{nullptr, $7, $4, $2};
                                $$ = p; $$->loc = @$;
                            }
    /* Function definition without parameters that returns nothing */
    | FUNC typedotnamechain '(' ')' code_block
                            {
                                auto p = new ast_node_funcdef{nullptr, $5, $2, nullptr};
                                $$ = p; $$->loc = @$;
                            }
    /* Function definition with parameters that returns nothing */
    | FUNC typedotnamechain '(' vardef_list ')' code_block
                            {
                                auto p = new ast_node_funcdef{$4, $6, $2, nullptr};
                                $$ = p; $$->loc = @$;
                            }
    /* Function declaration */
    | FUNC vardef_list '=' typedotnamechain '(' vardef_list ')'
                            {
                                auto p = new ast_node_funcdec{$6, $4, $2, false};
                                $$ = p; $$->loc = @$;
                            }
    /* Function declaration without parameters that returns something */
    | FUNC vardef_list '=' typedotnamechain '(' ')'
                            {
                                auto p = new ast_node_funcdec{nullptr, $4, $2, false};
                                $$ = p; $$->loc = @$;
                            }
    /* Function declaration without parameters that returns nothing */
    | FUNC typedotnamechain '(' ')' 
                            {
                                auto p = new ast_node_funcdec{nullptr, $2, nullptr, false};
                                $$ = p; $$->loc = @$;
                            }
    /* Function declaration with parameters that returns nothing */
    | FUNC typedotnamechain '(' vardef_list ')'
                            {
                                auto p = new ast_node_funcdec{$4, $2, nullptr, false};
                                $$ = p; $$->loc = @$;
                            }

    /* Function declarations with c linkage */
    | FUNC vardef_list '=' CLINKAGE typedotnamechain '(' vardef_list ')'
                            {
                                auto p = new ast_node_funcdec{$7, $5, $2, true};
                                $$ = p; $$->loc = @$;
                            }
    /* Function declaration without parameters that returns something */
    | FUNC vardef_list '=' CLINKAGE typedotnamechain '(' ')'
                            {
                                auto p = new ast_node_funcdec{nullptr, $5, $2, true};
                                $$ = p; $$->loc = @$;
                            }
    /* Function declaration without parameters that returns nothing */
    | FUNC CLINKAGE typedotnamechain '(' ')' 
                            {
                                auto p = new ast_node_funcdec{nullptr, $3, nullptr, true};
                                $$ = p; $$->loc = @$;
                            }
    /* Function declaration with parameters that returns nothing */
    | FUNC CLINKAGE typedotnamechain '(' vardef_list ')'
                            {
                                auto p = new ast_node_funcdec{$5, $3, nullptr, true};
                                $$ = p; $$->loc = @$;
                            }

    | WHILE e DO exp_list END 
    						{ $$ = new ast_node_while{$2, $4}; $$->loc = @$;}
    | WHILE e DO exp_list ELSE DO exp_list END 
    						{ $$ = new ast_node_while{$2, $4, $7}; $$->loc = @$;}
    | vardef '=' se         { $$ = $1; dynamic_cast<ast_node_def*>($1)->value_node = $3; $$->loc = @$;}
    | vardef                { $$ = $1; $$->loc = @$;}
    | NAMESPACE typedotchain 
                            {
                                $$ = new ast_node_nspace{$2}; $$->loc = @$;
                            }
    ;
    
/* A list of ifelses */
elseif_list: IF e DO exp_list              { $$ = new ast_node_elseiflist{$2, $4}; $$->loc = @$;}
       | elseif_list ELSE IF e DO exp_list { 
                                             auto pp = dynamic_cast<ast_node_elseiflist*>($1);
                                             pp->append_elseif($4, $6);
                                             $$ = $1; $$->loc = @$;
                                           }
/* A list of ifelsies for one liner. */                                         
sl_elseif_list: IF e DO se                 { $$ = new ast_node_elseiflist{$2, $4}; $$->loc = @$;}
       | sl_elseif_list ELSE IF e DO se    { 
                                             auto pp = dynamic_cast<ast_node_elseiflist*>($1);
                                             pp->append_elseif($4, $6);
                                             $$ = $1; $$->loc = @$;
                                           }  
                                           
vardef_list: vardef                         {
                                                auto p = new ast_node_vardef_list;
                                                p->append($1); $$ = p; $$->loc = @$;
                                            }
           | vardef_list ',' vardef         {
                                                auto p = dynamic_cast<ast_node_vardef_list*>($$);
                                                p->append($3); $$->loc = @$;
                                            }                                    
 
ptrdef_list: '&'                            {
                                                auto p = new ast_node_ptrdef_list;
                                                p->append_const(false); $$ = p; $$->loc = @$;
                                            }
            | '&' ptrdef_list               {
                                                auto p = dynamic_cast<ast_node_ptrdef_list*>($2);
                                                p->append_const(false);
                                                $$ = p; $$->loc = @$;
                                            }

vardef: typedotchain typedotnamechain       {
                                                $$ = new ast_node_def{$1, $2, nullptr}; $$->loc = @$;
                                            }
        | ptrdef_list typedotchain typedotnamechain         
                                            {
                                                auto node = new ast_node_def{$2, $3, nullptr}; 
                                                auto node_pdl = dynamic_cast<ast_node_ptrdef_list*>($1);
                                                node->ptrdef_node = node_pdl;
                                                $$ = node; $$->loc = @$;
                                            }

typedotchain: TYPENAME                      {
                                                auto p = new ast_node_typedotchain{};
                                                p->append_type(*$1); 
                                                $$ = p; $$->loc = @$;
                                                delete $1;
                                            }
            | typedotchain '.' TYPENAME     {
                                                auto p = dynamic_cast<ast_node_typedotchain*>($$);
                                                p->append_type(*$3); $$->loc = @$;
                                                delete $3; 
                                            }

typedotnamechain: NAME                      {
                                                auto p = new ast_node_typedotnamechain{*$1, 0};
                                                $$ = p; $$->loc = @$;
                                                delete $1;
                                            }
            | typedotchain '.' NAME         {
                                                auto p = new ast_node_typedotnamechain{*$3, $1};
                                                delete $3;
                                                $$ = p; $$->loc = @$;
                                            }

using:  IMPORT usingchain                    { 
                                                $$ = new ast_node_using{$2};
                                                $$->loc = @$; $$->loc = @$;
                                            }
        | USING IMPORT usingchain           { 
                                                $$ = new ast_node_using{$3, true};
                                                $$->loc = @$; $$->loc = @$;
                                            }

usingchain: typedotchain                    { $$ = new ast_node_usingchain{$1}; $$->loc = @$;}
            | usingchain ',' typedotchain   {
                                                auto p = dynamic_cast<ast_node_usingchain*>($1);
                                                p->append_typedotchain($3);
                                                $$ = p;
                                                $$->loc = @$;
                                            }       

code_block: DO exp_list END     { $$ = new ast_node_doblock{$2}; $$->loc = @$;}

arg_list: e                 {
                                auto p = new ast_node_arglist;
                                p->append_arg($1); $$ = p; $$->loc = @$;
                            }
        | arg_list ',' e    {
                                auto p = static_cast<ast_node_arglist*>($$);
                                p->append_arg($3); $$->loc = @$;
                            }
    
 /* Top expression */
e: exp  
   | e '=' e                {$$ = new ast_node_assign{$1, $3}; $$->loc = @$;}
   ;

 /* Not compare or assign expressions */
exp: exp '+' exp            {$$ = new ast_node_add{$1, $3}; $$->loc = @$;}
    | exp '-' exp           {$$ = new ast_node_sub{$1, $3}; $$->loc = @$;}
    | exp '*' exp           {$$ = new ast_node_mul{$1, $3}; $$->loc = @$;}
    | exp '/' exp           {$$ = new ast_node_rdiv{$1, $3}; $$->loc = @$;}
    | exp '%' exp           {$$ = new ast_node_rem{$1, $3}; $$->loc = @$;}
    | exp INTDIV exp        {$$ = new ast_node_intdiv{$1, $3}; $$->loc = @$;}

    | exp '.' NAME          {$$ = new ast_node_dotop{$1, *$3}; delete $3; $$->loc = @$;}

    /* Pointer manipulation */
    | '@' exp               {$$ = new ast_node_deref{$2}; $$->loc = @$;}
    | ptrdef_list exp       { /* ptrdef_list instead of '&' to resolve shift/reduce conflict */
                                auto p = dynamic_cast<ast_node_ptrdef_list*>($1);
                                if (p->v_const.size() != 1)
                                    throw std::runtime_error("To many '&'s for an expression");
                                delete p;
                                $$ = new ast_node_address{$2}; $$->loc = @$;
                            }
    
    | exp AND exp           {$$ = new ast_node_and{$1, $3}; $$->loc = @$;}
    | exp OR exp            {$$ = new ast_node_or{$1, $3}; $$->loc = @$;}
    | exp XOR exp           {$$ = new ast_node_xor{$1, $3}; $$->loc = @$;}
    | exp NAND exp          {$$ = new ast_node_nand{$1, $3}; $$->loc = @$;}
    | exp NOR exp           {$$ = new ast_node_nor{$1, $3}; $$->loc = @$;}
    | exp XNOR exp          {$$ = new ast_node_xnor{$1, $3}; $$->loc = @$;}
    | NOT exp               {$$ = new ast_node_not{$2}; $$->loc = @$;}

    | exp CMP exp           {$$ = new ast_node_cmp{$1, $3}; $$->loc = @$;}
    | '-' exp %prec UMINUS  {$$ = new ast_node_uminus{$2}; $$->loc = @$;}
    | '(' exp ')'           {$$ = $2; $$->loc = @$;}
    | '|' exp '|'           {$$ = new ast_node_abs{$2}; $$->loc = @$;} 
    | '{' arg_list '}'      {$$ = new ast_node_listlit{$2}; $$->loc = @$;}
    | '{' '}'               {$$ = new ast_node_listlit{0}; $$->loc = @$;}
    | exp '^' exp           {$$ = new ast_node_pow{$1, $3}; $$->loc = @$;}
    | typedotnamechain      {$$ = new ast_node_var{$1}; $$->loc = @$;}
    | NUMBER                {$$ = $1; $$->loc = @$;}
    | typedotnamechain '(' arg_list ')' 
                            {$$ = new ast_node_funccall{$1, $3}; $$->loc = @$;}
    | typedotnamechain '(' ')'          
                            {$$ = new ast_node_funccall{$1, 0}; $$->loc = @$;}
    | ESC_STRING			{$$ = new ast_node_string_literal{*$1}; delete $1; $$->loc = @$;}
    | cmp_exp
    ;

struct_def : STRUCT EOL field_list EOL END { $$ = $3; $$->loc = @$; }

    /* TODO: Make a ast_node_list to accumulate ast_nodes to a generic list and 
    * then constreuct stuff from it etc. */
field_list: vardef          {  
                                auto fl = new ast_node_struct_def{};
                                fl->append_field($1);
                                $$ = fl; $$->loc = @$;
                            }
            | field_list EOL vardef
                            {
                                auto fl = dynamic_cast<ast_node_struct_def*>($1);
                                fl->append_field($3);
                                $$ = fl; $$->loc = @$;
                            }

  
 /* Compare expressions. They can be chained 3 > 2 > 1 is 3 > 2 AND 2 > 1 etc
  * with only one evaluation per operand. */
cmp_exp:  exp '>' exp       {
                                auto gre = new ast_node_gre{$1, $3};
                                $$ = new ast_node_andchain{gre}; $$->loc = @$;
                            }
        | exp '<' exp       {
                                auto cmp = new ast_node_les{$1, $3};
                                $$ = new ast_node_andchain{cmp}; $$->loc = @$;
                            }
        | exp EQU exp       {
                                auto cmp = new ast_node_equ{$1, $3};
                                $$ = new ast_node_andchain{cmp}; $$->loc = @$;
                            }
        | exp LEQ exp       {
                                auto cmp = new ast_node_leq{$1, $3};
                                $$ = new ast_node_andchain{cmp}; $$->loc = @$;
                            }
        | exp GEQ exp       {
                                auto cmp = new ast_node_geq{$1, $3};
                                $$ = new ast_node_andchain{cmp}; $$->loc = @$;
                            }
        | exp NEQ exp       {
                                auto cmp = new ast_node_neq{$1, $3};
                                $$ = new ast_node_andchain{cmp}; $$->loc = @$;
                            }
        | cmp_exp '>' exp   {
                                auto chain = dynamic_cast<ast_node_andchain*>($1);
                                auto gre = new ast_node_gre{nullptr, $3};
                                chain->append_next(gre);
                                $$ = chain; $$->loc = @$;
                            }
        | cmp_exp '<' exp   {
                                auto chain = dynamic_cast<ast_node_andchain*>($1);
                                auto les = new ast_node_les{nullptr, $3};
                                chain->append_next(les);
                                $$ = chain; $$->loc = @$;
                            }
        | cmp_exp EQU exp   {
                                auto chain = dynamic_cast<ast_node_andchain*>($1);
                                auto equ = new ast_node_equ{nullptr, $3};
                                chain->append_next(equ);
                                $$ = chain; $$->loc = @$;
                            }
        | cmp_exp LEQ exp   {
                                auto chain = dynamic_cast<ast_node_andchain*>($1);
                                auto leq = new ast_node_leq{nullptr, $3};
                                chain->append_next(leq);
                                $$ = chain; $$->loc = @$;
                            }
        | cmp_exp GEQ exp   {
                                auto chain = dynamic_cast<ast_node_andchain*>($1);
                                auto geq = new ast_node_geq{nullptr, $3};
                                chain->append_next(geq);
                                $$ = chain; $$->loc = @$;
                            }
        | cmp_exp NEQ exp   {
                                auto chain = dynamic_cast<ast_node_andchain*>($1);
                                auto neq = new ast_node_neq{nullptr, $3};
                                chain->append_next(neq);
                                $$ = chain; $$->loc = @$;
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
