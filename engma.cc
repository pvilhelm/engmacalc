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
    {"exe",     'X', 0, 0, "Execute as a JIT compilation."},
    {0,         'o', "FILE", 0, "Specifies output file name"},
    {0,         'I', "FOLDER", 0, "Specifies an include directory"},
    {0,         'c', 0, 0, "Compile the program to object files, don't link"},
    {"shared",  ARG_SHARED, 0, 0, "Link into a shared library"},
    {0,         'l',"LIB",0,"Link to library"},
    {0,         'O', "LEVEL", OPTION_ARG_OPTIONAL, "Optimization level"},
    {0,         'g',"LEVEL", OPTION_ARG_OPTIONAL, "Debugging flag"},
    {0,         'L', "FOLDER", 0, "Specifies a folder to look for shared objects"},
    {0}
};

static int parse_opt (int key, char *arg, struct argp_state *state)
{
    switch (key) {
    case 'g':
        opts.debug_flag = "-g" + std::string{arg};
        break;
    case 'L':
        opts.l_folders.push_back("-L" + std::string{arg});
        break;
    case 'l':
        opts.l_folders.push_back("-l" + std::string{arg});
        break;
    case 'O':
        opts.optimization_level = "-O" + std::string{arg};
        break;
    case 'X':
        opts.run_type = engma_run_type::EXECUTE;
        break;
    case 'I':
        opts.include_dirs.push_back(arg);
        break;
    case 'c':
        opts.run_type = engma_run_type::OUTPUT_TO_OBJ_FILE;
        break;
    case ARG_SHARED:
        opts.run_type = engma_run_type::OUTPUT_TO_SO;
        break;
    case 'o':
        opts.outputfile_name = std::string{arg};
        break;
    case ARGP_KEY_ARG:
        if (ends_with(arg, ".em"))
            opts.files.push_back(arg);
        else
            opts.nonengma_files.push_back(arg);
        break;
    }

    return 0;
}

void verify_opts()
{

}

/* TODO: Refactor this messy main function */
int main(int argc, char **argv)
{
    
    argp argp = {options, parse_opt, "FILES...", 0};
    argp_parse(&argp, argc, argv, 0, 0, 0);
    verify_opts();
    yydebug = 0;

    init_builtin_types();

    if (opts.run_type == engma_run_type::OUTPUT_TO_OBJ_FILE) {
        if (opts.nonengma_files.size()) {
            compile_c_obj_files();
        }
        if (opts.files.size()) {
            for (std::string file : opts.files) {
                yyscan_t scanner;
                yylex_init(&scanner);

                FILE *f = nullptr;
                f = fopen(file.c_str(), "r");
                if(!f) {
                    throw std::runtime_error("Could not open file: " + std::string{argv[1]});
                }
                yyset_in(f, scanner);

                    bool parsed_eol;

                do {
                    int err = yyparse(scanner);

                    if (err) {
                        std::cerr << "error" << std::endl;
                        if (!isatty(0))
                            return 1;
                    }

                    auto &cu = compilation_units.get_current_compilation_unit();

                    if (cu.ast_root) {
                        cu.ast_root->resolve(), 
                        cu.v_nodes.push_back(cu.ast_root);
                        cu.ast_root = nullptr;
                    }

                    parsed_eol = cu.parsed_eol;

                } while (!parsed_eol);

                jit jit;
                jit.init_as_root_context();

                auto &cu = compilation_units.get_current_compilation_unit();
                for (auto e : cu.v_nodes)
                    jit.add_ast_node(e);
                
                jit.postprocess();
                jit.dump("./dump.txt");
                jit.compile();
                if (opts.run_type == engma_run_type::EXECUTE)
                    jit.execute();

                yylex_destroy(scanner);

                /* Clear some globals so we can see that all nodes are freed for
                * debugging purposes. */
                compilation_units.clear();
                builtin_typestack.clear();
                builtin_objstack.clear();

                DEBUG_ASSERT(ast_node_count == 0, "ast nodes seems to be leaking: " << ast_node_count);
                DEBUG_ASSERT(value_expr_count == 0, "value_expr seems to be leaking: " << value_expr_count);
            }
        }
    } else if (opts.run_type == engma_run_type::OUTPUT_TO_SO || 
                opts.run_type == engma_run_type::OUTPUT_TO_EXE) 
    {
        if (opts.files.size()) {
            if (opts.files.size() != 1)
                THROW_NOT_IMPLEMENTED("");

            for (std::string file : opts.files) {
                yyscan_t scanner;
                yylex_init(&scanner);

                FILE *f = nullptr;
                f = fopen(file.c_str(), "r");
                if(!f) {
                    throw std::runtime_error("Could not open file: " + std::string{argv[1]});
                }
                yyset_in(f, scanner);

                    bool parsed_eol;

                do {
                    int err = yyparse(scanner);

                    if (err) {
                        std::cerr << "error" << std::endl;
                        if (!isatty(0))
                            return 1;
                    }

                    auto &cu = compilation_units.get_current_compilation_unit();

                    if (cu.ast_root) {
                        cu.ast_root->resolve(), 
                        cu.v_nodes.push_back(cu.ast_root);
                        cu.ast_root = nullptr;
                    }

                    parsed_eol = cu.parsed_eol;

                } while (!parsed_eol);

                jit jit;
                jit.init_as_root_context();

                auto &cu = compilation_units.get_current_compilation_unit();
                for (auto e : cu.v_nodes)
                    jit.add_ast_node(e);
                
                jit.postprocess();
                jit.dump("./dump.txt");
                jit.compile();
                if (opts.run_type == engma_run_type::EXECUTE)
                    jit.execute();

                yylex_destroy(scanner);

                /* Clear some globals so we can see that all nodes are freed for
                * debugging purposes. */
                compilation_units.clear();
                builtin_typestack.clear();
                builtin_objstack.clear();

                DEBUG_ASSERT(ast_node_count == 0, "ast nodes seems to be leaking: " << ast_node_count);
                DEBUG_ASSERT(value_expr_count == 0, "value_expr seems to be leaking: " << value_expr_count);
            }
        }
    } else if (opts.run_type == engma_run_type::EXECUTE) {
        if (opts.files.size()) {

            if (opts.files.size() != 1)
                THROW_NOT_IMPLEMENTED("");
                
            for (std::string file : opts.files) {
                yyscan_t scanner;
                yylex_init(&scanner);

                FILE *f = nullptr;
                f = fopen(file.c_str(), "r");
                if(!f) {
                    throw std::runtime_error("Could not open file: " + std::string{argv[1]});
                }
                yyset_in(f, scanner);

                    bool parsed_eol;

                do {
                    int err = yyparse(scanner);

                    if (err) {
                        std::cerr << "error" << std::endl;
                        if (!isatty(0))
                            return 1;
                    }

                    auto &cu = compilation_units.get_current_compilation_unit();

                    if (cu.ast_root) {
                        cu.ast_root->resolve(), 
                        cu.v_nodes.push_back(cu.ast_root);
                        cu.ast_root = nullptr;
                    }

                    parsed_eol = cu.parsed_eol;

                } while (!parsed_eol);

                jit jit;
                jit.init_as_root_context();

                auto &cu = compilation_units.get_current_compilation_unit();
                for (auto e : cu.v_nodes)
                    jit.add_ast_node(e);
                
                jit.postprocess();
                jit.dump("./dump.txt");
                jit.compile();
                if (opts.run_type == engma_run_type::EXECUTE)
                    jit.execute();

                yylex_destroy(scanner);

                /* Clear some globals so we can see that all nodes are freed for
                * debugging purposes. */
                compilation_units.clear();
                builtin_typestack.clear();
                builtin_objstack.clear();

                DEBUG_ASSERT(ast_node_count == 0, "ast nodes seems to be leaking: " << ast_node_count);
                DEBUG_ASSERT(value_expr_count == 0, "value_expr seems to be leaking: " << value_expr_count);
            }
        }
    }

    return 0;
}
