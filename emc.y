
%code top {
#include <cstdio>
#include <cstdlib>
#include <iostream>

#include "emc.hh"

int yylex();
void yyerror(const char *s);
}

%union {
    ast_node *node;
    std::string *s;
}

%token <s> NUMBER
%token <s> NAME
%token <s> TYPENAME

%token EOL IF DO END ELSE WHILE ENDOFFILE FUNC

%right '='
%left CMP LEQ GEQ EQU NEQ '>' '<'
%left '+' '-'
%left '*' '/'
%right '^'

%nonassoc DO
%nonassoc END        
%nonassoc '|' UMINUS

%start program

%type <node> exp cmp_exp e se exp_list code_block arg_list param_list

%define parse.trace
    
%%

program:               
     | program EOL
     | program exp_list EOL
                        { 
                            auto val = $2->eval();
                            //delete $2;
                            if (val->type == value_type::DOUBLE) {
                               auto vald = static_cast<expr_value_double*>(val);
                               std::cout << vald->d << std::endl;
                            } else
                                throw std::runtime_error("Not implemented qweqe");
                            //delete val;
                            //return 1;
                        }
     | program ENDOFFILE { return 1;}
     ;

    /* List of expressions */
exp_list: se           {$$ = new ast_node_explist{$1};}
     | exp_list EOL se {
                            auto p_exl = static_cast<ast_node_explist*>($1);
                            p_exl->append_node($3); std::cout << "appended node\n";
                            $$ = $1; 
                        }
     ;                       

 /* Statement expression */
se: e
    | IF e DO EOL exp_list EOL END { $$ = new ast_node_if{$2, $5};}
    | IF e DO exp_list END { $$ = new ast_node_if{$2, $4};}

    | IF e DO     exp_list     ELSE     exp_list     END
                        { $$ = new ast_node_if{$2, $4, $6};}
    | IF e DO EOL exp_list EOL ELSE EOL exp_list EOL END 
                        { $$ = new ast_node_if{$2, $5, $9};}                        
    | code_block
    | FUNC NAME '(' param_list ')' code_block
                {
                    auto p = new ast_node_funcdec{$4, $6, *$2, ""};
                    delete $2;
                    $$ = p;
                }
    ;

code_block: DO exp_list END             { $$ = new ast_node_doblock{$2};}
          | DO EOL exp_list EOL END     { $$ = new ast_node_doblock{$3};}

param_list: NAME                     {auto p = new ast_node_parlist;
                                      $$ = p;
                                      p->append_parameter(*$1);
                                      delete $1;}
        | param_list ',' NAME        {auto p = static_cast<ast_node_parlist*>($$);
                                      p->append_parameter(*$3); delete $3;}

arg_list: e                         {auto p = new ast_node_arglist;
                                     p->append_arg($1); $$ = p;}
        | arg_list ',' e            {auto p = static_cast<ast_node_arglist*>($$);
                                     p->append_arg($3);}
    
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
     | NUMBER                {$$ = new ast_node_double_literal(*$1); delete $1;}
     | NAME '(' arg_list ')' {$$ = new ast_node_funccall{"", *$1, $3}; delete $1;}
     ;
  
 /* Compare expressions. */
cmp_exp:  exp '>' exp        {
                                auto t = new ast_node_temporary($3);
                                auto gre = new ast_node_gre{$1, t};
                                $$ = new ast_node_andchain{gre};
                             }
         | exp '<' exp       {
                                auto t = new ast_node_temporary($3);
                                auto cmp = new ast_node_les{$1, t};
                                $$ = new ast_node_andchain{cmp};
                             }
         | exp EQU exp       {
                                auto t = new ast_node_temporary($3);
                                auto cmp = new ast_node_equ{$1, t};
                                $$ = new ast_node_andchain{cmp};
                             }
         | exp LEQ exp       {
                                auto t = new ast_node_temporary($3);
                                auto cmp = new ast_node_leq{$1, t};
                                $$ = new ast_node_andchain{cmp};
                             }
         | exp GEQ exp       {
                                auto t = new ast_node_temporary($3);
                                auto cmp = new ast_node_geq{$1, t};
                                $$ = new ast_node_andchain{cmp};
                             }
         | exp NEQ exp       {
                                auto t = new ast_node_temporary($3);
                                auto cmp = new ast_node_neq{$1, t};
                                $$ = new ast_node_andchain{cmp};
                             }
         | cmp_exp '>' exp   {
                                auto chain = dynamic_cast<ast_node_andchain*>($1);
                                auto t = new ast_node_temporary($3);
                                auto gre = new ast_node_gre{chain->tail, t};
                                chain->append_next(new ast_node_andchain{gre});
                                $$ = chain;
                             }
         | cmp_exp '<' exp   {
                                auto chain = dynamic_cast<ast_node_andchain*>($1);
                                auto t = new ast_node_temporary($3);
                                auto cmp = new ast_node_les{chain->tail, t};
                                chain->append_next(new ast_node_andchain{cmp});
                                $$ = chain;
                             }
         | cmp_exp EQU exp   {
                                auto chain = dynamic_cast<ast_node_andchain*>($1);
                                auto t = new ast_node_temporary($3);
                                auto cmp = new ast_node_equ{chain->tail, t};
                                chain->append_next(new ast_node_andchain{cmp});
                                $$ = chain;
                             }
         | cmp_exp LEQ exp   {
                                auto chain = dynamic_cast<ast_node_andchain*>($1);
                                auto t = new ast_node_temporary($3);
                                auto cmp = new ast_node_leq{chain->tail, t};
                                chain->append_next(new ast_node_andchain{cmp});
                                $$ = chain;
                             }
         | cmp_exp GEQ exp   {
                                auto chain = dynamic_cast<ast_node_andchain*>($1);
                                auto t = new ast_node_temporary($3);
                                auto cmp = new ast_node_geq{chain->tail, t};
                                chain->append_next(new ast_node_andchain{cmp});
                                $$ = chain;
                             }
         ;

%%

scope_stack scopes;

int main()
{
    yydebug = 0;
    scopes.push_new_scope();
    return yyparse();
}

void yyerror(const char * s)
{
    fprintf(stderr, "%s\n", s);
}
