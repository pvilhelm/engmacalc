
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

%token EOL IF DO END ELSE

%right '='
%left CMP LEQ GEQ EQU NEQ '>' '<'
%left '+' '-'
%left '*' '/'
%right '^'
%nonassoc '|' UMINUS


%type <node> exp cmp_exp e se

%%

    /* List of expressions */
exp_list:
     | exp_list EOL 
     | exp_list e EOL {
                            auto val = $2->eval();
                            delete $2;
                            if (val->type == value_type::DOUBLE) {
                               auto vald = static_cast<expr_value_double*>(val);
                               std::cout << "> " << vald->d << std::endl;
                            } else
                               std::cout << "> Inte D" << std::endl;
                            delete val;
                        }


 /* Top expression */
e: exp
   | cmp_exp
   | e '=' e                 {$$ = new ast_node_assign{$1, $3};}

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
    scopes.push_new_scope();
    return yyparse();
}

void yyerror(const char * s)
{
    fprintf(stderr, "%s\n", s);
}
