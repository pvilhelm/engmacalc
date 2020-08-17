#include <iostream>
#include <cstdlib>
#include <stdexcept>

#include "compile.hh"
#include "common.hh"

#define INT_TYPE this->types->int_type
#define DOUBLE_TYPE this->types->double_type
#define VOID_TYPE this->types->void_type

void printnl_int(int i)
{
    std::cout << i << std::endl;
}

void jit::setup_default_root_environment()
{
    { /* Add: int getchar() */
        auto p_fnobj = gcc_jit_context_new_function (context, NULL,
                          GCC_JIT_FUNCTION_IMPORTED,
                          INT_TYPE,
                          "getchar",
                          0, NULL,
                          0);
        map_fnname_to_gccfnobj["getchar"] = p_fnobj;
    }
    { /* Add: void putchar(int c) */
        gcc_jit_param *param_c =
            gcc_jit_context_new_param (context, NULL, INT_TYPE, "c");
        auto p_fnobj =
            gcc_jit_context_new_function (context, NULL,
                          GCC_JIT_FUNCTION_IMPORTED,
                          VOID_TYPE,
                          "putchar",
                          1, &param_c,
                          0);
          map_fnname_to_gccfnobj["putchar"] = p_fnobj;
    }
}

void jit::init_as_root_context()
{
    /* Init a context */
    context = gcc_jit_context_acquire ();
    if (!context)
    {
      std::cerr << STREAM_FILE_FNC_LNO << "Could not acquire jit context";
      throw std::runtime_error("Could not acquire jit context");
    }

    /* Optimazation level. O3 == 3 O0 == 0 */
    gcc_jit_context_set_int_option(context, GCC_JIT_INT_OPTION_OPTIMIZATION_LEVEL, 0);

    /* Dump gimple to stderr  */
    gcc_jit_context_set_bool_option(context, GCC_JIT_BOOL_OPTION_DUMP_INITIAL_GIMPLE, 1);

    /* Initialize the default types for this context (e.g. int etc) and put them in this->types */
    types = new default_types(context);

    setup_default_root_environment();

    root_func =
        gcc_jit_context_new_function (context, NULL,
                      GCC_JIT_FUNCTION_EXPORTED,
                      VOID_TYPE,
                      "root_fn",
                      0, 0, 0);
    root_block = gcc_jit_function_new_block(root_func, "root_block");
}

void jit::add_ast_node(ast_node *node)
{
    walk_tree(node, root_block, root_func);
}

void jit::walk_tree(ast_node *node, gcc_jit_block * current_block,
        gcc_jit_function *current_function)
{
    ast_type type = node->type;

    if (type == ast_type::ADD) {
        /* Resolve types. */
        gcc_jit_type *result_type = INT_TYPE;
        /* Get r-value a */


        gcc_jit_context_new_binary_op(context, nullptr, GCC_JIT_BINARY_OP_PLUS,
                result_type, a, b);
    }
}
