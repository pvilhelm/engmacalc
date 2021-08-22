#include <unistd.h>
#include <iostream>
#include <stdexcept>
#include <argp.h>

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
ast_compilation_units compilation_units;
typescope_stack builtin_typestack;
objscope_stack builtin_objstack;

struct engma_options opts;

#define ARG_SHARED 1000
struct argp_option options[] = 
{
    {0,'o', "FILE", 0, "Specifies output file name"},
    {0,'I', "FOLDER", 0, "Specifies a include directory"},
    {0,'c', 0, 0, "Only compile the program, don't link"},
    {"shared", ARG_SHARED, 0, 0, "Link into a shared library"},
    {0}
};

static int parse_opt (int key, char *arg, struct argp_state *state)
{
    switch (key) {
    case 'I':
        opts.include_dirs.push_back(arg);
        break;
    case 'c':
        opts.output_to_obj_file = true;
        opts.execute = false;
        break;
    case ARG_SHARED:
        opts.output_to_so = true;
        opts.execute = false;
        break;
    case 'o':
        opts.outputfile_name = std::string{arg};
        break;
    case ARGP_KEY_ARG:
        opts.files.push_back(arg);
        break;
    }

    return 0;
}

int main(int argc, char **argv)
{
    
    argp argp = {options, parse_opt, "FILES...", 0};
    argp_parse(&argp, argc, argv, 0, 0, 0);

    init_builtin_functions();
    init_standard_variables(); /* pi, e ... */
    init_linked_cfunctions();  /* Initialice function objects for statically linked cfunctions. */
    init_builtin_types();

    yyscan_t scanner;
    yydebug = 0;

    yylex_init(&scanner);

    FILE *f = nullptr;

    if (opts.files.size()) {
        f = fopen(opts.files[0].c_str(), "r");
        if(!f) {
            throw std::runtime_error("Could not open file: " + std::string{argv[1]});
        }
        yyset_in(f, scanner);
    }

    redo:
    int err = yyparse(scanner);

    if (err) {
        std::cerr << "error" << std::endl;
        if (!isatty(0))
            return 1;
    }

    auto &cu = compilation_units.get_current_compilation_unit();

    if (!isatty(0) || f) {
        if (cu.ast_root) {
            cu.ast_root->resolve(), 
            cu.v_nodes.push_back(cu.ast_root);
            cu.ast_root = nullptr;
        }
        if (!cu.parsed_eol)
            goto redo;
        jit jit;
        jit.init_as_root_context();

        for (auto e : cu.v_nodes)
            jit.add_ast_node(e);
        
        jit.dump("./dump.txt");
        jit.compile();
        if (opts.execute)
            jit.execute();
    } else if (isatty(0) && cu.ast_root && err == 0) {
        emc_type type = cu.ast_root->resolve();
        {

            jit jit;
            jit.init_as_root_context();

            for (auto e : cu.v_nodes)
                jit.add_ast_node(e);
            jit.add_ast_node(ast_root);
           
            jit.dump("./dump.txt");
            jit.compile();
            jit.execute();

            /* Save variable declarations and definitions. */
            /*TODO: add persistens env. var. values somehow between CLI
             * and the JIT.
             */
            if (ast_root->type == ast_type::FUNCTION_DEF ||
                ast_root->type == ast_type::DEF) {
                cu.v_nodes.push_back(cu.ast_root);
                cu.ast_root = nullptr;
            } else {
                delete cu.ast_root;
                cu.ast_root = nullptr;
            }
        }
        if (std::cin && !cu.parsed_eol)
            goto redo;
    } else if (std::cin)
            goto redo;

    yylex_destroy(scanner);

    /* Clear some globals so we can see that all nodes are freed for
     * debugging purposes. */
    compilation_units.clear();
    builtin_typestack.clear();
    builtin_objstack.clear();

    DEBUG_ASSERT(ast_node_count == 0, "ast nodes seems to be leaking: " << ast_node_count);
    DEBUG_ASSERT(value_expr_count == 0, "value_expr seems to be leaking: " << value_expr_count);
    return 0;
}
