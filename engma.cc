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
extern bool parsed_eol;

/* Scope during resolving of the ast node tree. */
scope_stack resolve_scope;

int main(int argc, char **argv)
{
    init_builtin_functions();
    init_standard_variables(); /* pi, e ... */
    init_linked_cfunctions();  /* Initialice function objects for statically linked cfunctions. */
    init_builtin_types();

    yyscan_t scanner;
    yydebug = 0;

    yylex_init(&scanner);
    std::vector<ast_node*> v_nodes;

    redo:
    int err = yyparse(scanner);

    if (err) {
        std::cerr << "error" << std::endl;
        if (!isatty(0))
            return 1;
    }

    if (!isatty(0)) {
        if (ast_root)
            ast_root->resolve(), v_nodes.push_back(ast_root);
        if (!parsed_eol)
            goto redo;
        jit jit;
        jit.init_as_root_context();

        for (auto e : v_nodes)
            jit.add_ast_node(e);
        
        jit.dump("./dump.txt");
        jit.compile();
        jit.execute();

    } else if (isatty(0) && ast_root && err == 0) {
        emc_type type = ast_root->resolve();
        {

            jit jit;
            jit.init_as_root_context();

            for (auto e : v_nodes)
                jit.add_ast_node(e);
            jit.add_ast_node(ast_root);
           
            jit.dump("./dump.txt");
            jit.compile();
            jit.execute();

            /* Save variable declarations and definitions. */
            /*TODO: add persistens env. var. values somehow between CLI
             * and the JIT.
             */
            if (ast_root->type == ast_type::FUNCTION_DECLARATION ||
                ast_root->type == ast_type::DEF) {
                v_nodes.push_back(ast_root);
            } else
                delete ast_root;
            
        }
        if (std::cin && !parsed_eol)
            goto redo;
    } else if (std::cin && !parsed_eol)
            goto redo;

    yylex_destroy(scanner);

    /* Clear some globals so we can see that all nodes are freed for
     * debugging purposes. */
    for (auto e : v_nodes)
        delete e;
    v_nodes.clear();
    resolve_scope.clear();

    DEBUG_ASSERT(ast_node_count == 0, "ast nodes seems to be leaking: " << ast_node_count);
    DEBUG_ASSERT(value_expr_count == 0, "value_expr seems to be leaking: " << value_expr_count);
    return 0;
}
