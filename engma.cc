#include <unistd.h>
#include <iostream>
#include <stdexcept>

/* Bison and flex requires this include order. */
#include "emc.hh"
typedef void *yyscan_t;
#include "emc.tab.h"
#include "lexer.h"
/* End of stupid include order. */

#include "compile.hh"

/* external objects */
extern int yydebug;
extern ast_node *ast_root; /* Bison writes to this after each parse(). */

/* Scope during runtime, tree walking interpretion */
scope_stack scopes;
/* Scope during resolving of the ast node tree. */
scope_stack resolve_scope;

int main()
{
    scopes.push_new_scope(); /* Add a top scope */
    init_builtin_functions();
    init_standard_variables(); /* pi, e ... */
    init_linked_cfunctions(); /* Initialice function objects for statically linked cfunctions. */
    init_builtin_types();
    resolve_scope = scopes;


    yyscan_t scanner;
    yylex_init(&scanner);
    yydebug = 0;

    bool interpret = true;

    redo:
    int err = yyparse(scanner);
    if (err) {
        std::cerr << "error" << std::endl;
        yylex_destroy(scanner);
        return 1;
    }

    if (ast_root && err == 0) {
        emc_type type = ast_root->resolve();
        if (interpret) {
            auto val = ast_root->eval();
            delete ast_root;
            /* Print value if stdin is from a terminal. */
            if (isatty(0)) {
                if (val->type == value_type::DOUBLE) {
                    auto vald = static_cast<expr_value_double*>(val);
                    std::cout << std::endl << "> " << vald->d << std::endl;
                } else if (val->type == value_type::STRING) {
                    auto vals = static_cast<expr_value_string*>(val);
                    std::cout << std::endl << "> " << vals->s << std::endl;
                } else if (val->type == value_type::INT) {
                    auto vali = static_cast<expr_value_int*>(val);
                    std::cout << std::endl << "> " << vali->i << std::endl;
                } else
                    throw std::runtime_error("Not implemented qweqe");
            }
            delete val;
        } else {
            jit jit;
            jit.init_as_root_context();
            jit.add_ast_node(ast_root);
            jit.compile();
            jit.execute();
        }
        if (std::cin)
            goto redo;
    } else if (!ast_root && err == 0)
        if (std::cin)
            goto redo;
    return 0;
}
