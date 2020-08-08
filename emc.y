
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

%token EOL

%right '='
%left CMP LEQ GEQ EQU NEQ
%left '+' '-'
%left '*' '/'
%nonassoc '|' UMINUS

%type <node> exp cmp_exp

%%

    /* List of expressions */
exp_list:
     | exp_list EOL 
     | exp_list exp EOL {
                            auto val = $2->eval();
                            delete $2;
                            if (val->type == value_type::DOUBLE) {
                               auto vald = static_cast<expr_value_double*>(val);
                               std::cout << "> " << vald->d << std::endl;
                            } else
                               std::cout << "> Inte D" << std::endl;
                            delete val;
                        }


exp: exp '+' exp             {$$ = new ast_node_add{$1, $3};}
     | exp '-' exp           {$$ = new ast_node_sub{$1, $3};}
     | exp '*' exp           {$$ = new ast_node_mul{$1, $3};}
     | exp '/' exp           {$$ = new ast_node_rdiv{$1, $3};}
     | exp CMP exp           {$$ = new ast_node_cmp{$1, $3};}
     | exp '<' exp           {$$ = new ast_node_les{$1, $3};}
  /*   | exp '>' exp           {$$ = new ast_node_gre{$1, $3};} */
     | exp EQU exp           {$$ = new ast_node_equ{$1, $3};}
     | exp LEQ exp           {$$ = new ast_node_leq{$1, $3};}
     | exp GEQ exp           {$$ = new ast_node_geq{$1, $3};}
     | exp NEQ exp           {$$ = new ast_node_neq{$1, $3};}
     | '-' exp %prec UMINUS  {$$ = new ast_node_uminus{$2};}
     | '(' exp ')'           {$$ = $2;}
     | NAME                  {$$ = new ast_node_var{*$1, ""}; delete $1;}
     | exp '=' exp           {$$ = new ast_node_assign{$1, $3};}
     | NUMBER                {$$ = new ast_node_double_literal(*$1); delete $1;}
     ;

cmp_exp: exp '>' exp         {$$ = new ast_node_gre{$1, $3};}
         | cmp_exp '>' exp   {
                                auto pc = static_cast<ast_node_chaincmp*>($1);
                                $$ = new ast_node_gre{pc->sec, $3};
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
