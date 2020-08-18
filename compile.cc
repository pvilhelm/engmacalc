#include <iostream>
#include <cstdlib>
#include <stdexcept>

#include "compile.hh"
#include "common.hh"

#define INT_TYPE this->types->int_type
#define DOUBLE_TYPE this->types->double_type
#define VOID_TYPE this->types->void_type



void jit::compile()
{
    result = gcc_jit_context_compile(context);
    if (!result)
    {
      std::cerr <<  "NULL result from JIT compilation" << std::endl;
    }
}

void jit::execute()
{
    typedef void (*root_fn_p)(void);
    root_fn_p root_func = (root_fn_p)gcc_jit_result_get_code(result, "root_fn");
    if (!root_func)
    {
      std::cerr <<  "NULL root function after JIT compilation" << std::endl;
    }
    root_func();
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
    { /* Add: void printnl_double(double d) */
        gcc_jit_param *param_d =
            gcc_jit_context_new_param (context, NULL, DOUBLE_TYPE, "d");
        auto p_fnobj =
            gcc_jit_context_new_function (context, NULL,
                          GCC_JIT_FUNCTION_IMPORTED,
                          VOID_TYPE,
                          "printnl_double",
                          1, &param_d,
                          0);
          map_fnname_to_gccfnobj["printnl_double"] = p_fnobj;
    }
    { /* Add: void printnl_int(int i) */
        gcc_jit_param *param_i =
            gcc_jit_context_new_param (context, NULL, INT_TYPE, "i");
        auto p_fnobj =
            gcc_jit_context_new_function (context, NULL,
                          GCC_JIT_FUNCTION_IMPORTED,
                          VOID_TYPE,
                          "printnl_int",
                          1, &param_i,
                          0);
          map_fnname_to_gccfnobj["printnl_int"] = p_fnobj;
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
    gcc_jit_rvalue *rval = nullptr;

    walk_tree(node, &root_block, &root_func, &rval);

    if (rval) { /* We just print top level rvals to stdout */
        gcc_jit_type *t = gcc_jit_rvalue_get_type(rval);
        /* TODO: Lägg i en fin global istället? */
        gcc_jit_rvalue *rv_call = nullptr;
        if (t == INT_TYPE) {
            rv_call = gcc_jit_context_new_call(context, 0, map_fnname_to_gccfnobj["printnl_int"], 1, &rval);
        } else if (t == DOUBLE_TYPE)
            rv_call = gcc_jit_context_new_call(context, 0, map_fnname_to_gccfnobj["printnl_double"], 1, &rval);
        /* Else ignore. */
        if (rv_call)
            gcc_jit_block_add_eval(root_block, 0, rv_call);
    }

    /* End the root block with a "return;" */
    gcc_jit_block_end_with_void_return(root_block, 0);
}

gcc_jit_type *emc_type_to_gccjit_type(const emc_type &type, gcc_jit_context *context)
{
    if (type.types.size() == 1) {
        if (type.types[0] == emc_types::INT)
            return gcc_jit_context_get_type (context, GCC_JIT_TYPE_INT);
        else if (type.types[0] == emc_types::DOUBLE)
            return gcc_jit_context_get_type (context, GCC_JIT_TYPE_DOUBLE);
        else
            throw std::runtime_error("Not implemented emc_type_to_gccjit_type 2");
    } else
        throw std::runtime_error("Not implemented emc_type_to_gccjit_type");
}

void jit::walk_tree(ast_node *node, gcc_jit_block **current_block,
        gcc_jit_function **current_function, gcc_jit_rvalue **current_rvalue)
{
    ast_type type = node->type;
    std::cout << "Waling tree:" << (*current_rvalue == nullptr) << " type: " << (int)type << std::endl;
    if (type == ast_type::ADD) {
        auto t_node = dynamic_cast<ast_node_add*>(node);
        /* Resolve types. */

        /* Get r-value a */
        emc_type rt = t_node->resolve();
        gcc_jit_type *result_type = emc_type_to_gccjit_type(rt, context);

        gcc_jit_rvalue *a_rv = nullptr;
        walk_tree(t_node->first, 0, 0, &a_rv);
        if (a_rv == nullptr)
            throw std::runtime_error("add a is null");
        gcc_jit_rvalue *b_rv = nullptr;
        walk_tree(t_node->sec, 0, 0, &b_rv);
        if (b_rv == nullptr)
            throw std::runtime_error("add b is null");

        gcc_jit_rvalue *rv_result = gcc_jit_context_new_binary_op(
                                        context, nullptr, GCC_JIT_BINARY_OP_PLUS,
                                        result_type, a_rv, b_rv);
        *current_rvalue = rv_result;
        return;
    } else if (type == ast_type::DOUBLE_SUB) {
        auto t_node = dynamic_cast<ast_node_sub*>(node);
        /* Resolve types. */

        /* Get r-value a */
        emc_type rt = t_node->resolve();
        gcc_jit_type *result_type = emc_type_to_gccjit_type(rt, context);

        gcc_jit_rvalue *a_rv = nullptr;
        walk_tree(t_node->first, 0, 0, &a_rv);
        if (a_rv == nullptr)
            throw std::runtime_error("sub a is null");
        gcc_jit_rvalue *b_rv = nullptr;
        walk_tree(t_node->sec, 0, 0, &b_rv);
        if (b_rv == nullptr)
            throw std::runtime_error("sub b is null");

        gcc_jit_rvalue *rv_result = gcc_jit_context_new_binary_op(
                                        context, nullptr, GCC_JIT_BINARY_OP_MINUS,
                                        result_type, a_rv, b_rv);
        *current_rvalue = rv_result;
        return;
    } else if (type == ast_type::DOUBLE_MUL) {
        auto t_node = dynamic_cast<ast_node_mul*>(node);
        /* Resolve types. */

        /* Get r-value a */
        emc_type rt = t_node->resolve();
        gcc_jit_type *result_type = emc_type_to_gccjit_type(rt, context);

        gcc_jit_rvalue *a_rv = nullptr;
        walk_tree(t_node->first, 0, 0, &a_rv);
        if (a_rv == nullptr)
            throw std::runtime_error("mul a is null");
        gcc_jit_rvalue *b_rv = nullptr;
        walk_tree(t_node->sec, 0, 0, &b_rv);
        if (b_rv == nullptr)
            throw std::runtime_error("mul b is null");

        gcc_jit_rvalue *rv_result = gcc_jit_context_new_binary_op(
                                        context, nullptr, GCC_JIT_BINARY_OP_MULT,
                                        result_type, a_rv, b_rv);
        *current_rvalue = rv_result;
        return;
    } else if (type == ast_type::DOUBLE_RDIV) {
        auto t_node = dynamic_cast<ast_node_rdiv*>(node);
        /* Resolve types. */

        /* Get r-value a */
        emc_type rt = t_node->resolve();
        gcc_jit_type *result_type = emc_type_to_gccjit_type(rt, context);

        gcc_jit_rvalue *a_rv = nullptr;
        walk_tree(t_node->first, 0, 0, &a_rv);
        if (a_rv == nullptr)
            throw std::runtime_error("RDIV a is null");
        gcc_jit_rvalue *b_rv = nullptr;
        walk_tree(t_node->sec, 0, 0, &b_rv);
        if (b_rv == nullptr)
            throw std::runtime_error("RDIV b is null");

        gcc_jit_rvalue *rv_result = gcc_jit_context_new_binary_op(
                                        context, nullptr, GCC_JIT_BINARY_OP_DIVIDE,
                                        result_type, a_rv, b_rv);
        *current_rvalue = rv_result;
        return;
#if 0
        }
 else if (type == ast_type::ANDCHAIN) {
        auto t_node = dynamic_cast<ast_node_andchain*>(node);
        /* TODO: Fix chaining somewhere here */
        /* Resolve types. */

        gcc_jit_rvalue *a_rv = nullptr;
        walk_tree(t_node->first, 0, 0, &a_rv);
        if (a_rv == nullptr)
            throw std::runtime_error("GEQ a is null");
        gcc_jit_rvalue *b_rv = nullptr;
        walk_tree(t_node->sec, 0, 0, &b_rv);
        if (b_rv == nullptr)
            throw std::runtime_error("GEQ b is null");

        gcc_jit_rvalue *rv_result = gcc_jit_context_new_comparison(
                                        context, nullptr, GCC_JIT_COMPARISON_GE,
                                        a_rv, b_rv);
        *current_rvalue = rv_result;
        return;
#endif
    } else if (type == ast_type::GEQ) {
        auto t_node = dynamic_cast<ast_node_geq*>(node);
        /* TODO: Fix chaining somewhere here */
        /* Resolve types. */

        gcc_jit_rvalue *a_rv = nullptr;
        walk_tree(t_node->first, 0, 0, &a_rv);
        if (a_rv == nullptr)
            throw std::runtime_error("GEQ a is null");
        gcc_jit_rvalue *b_rv = nullptr;
        walk_tree(t_node->sec, 0, 0, &b_rv);
        if (b_rv == nullptr)
            throw std::runtime_error("GEQ b is null");

        gcc_jit_rvalue *rv_result = gcc_jit_context_new_comparison(
                                        context, nullptr, GCC_JIT_COMPARISON_GE,
                                        a_rv, b_rv);
        *current_rvalue = rv_result;
        return;
    } else if (type == ast_type::DOUBLE_UMINUS) {
        auto t_node = dynamic_cast<ast_node_uminus*>(node);
        /* Resolve types. */

        /* Get r-value a */
        emc_type rt = t_node->resolve();
        gcc_jit_type *result_type = emc_type_to_gccjit_type(rt, context);

        gcc_jit_rvalue *a_rv = nullptr;
        walk_tree(t_node->first, 0, 0, &a_rv);
        if (a_rv == nullptr)
            throw std::runtime_error("sub a is null");

        gcc_jit_rvalue *rv_result = gcc_jit_context_new_unary_op(context, nullptr,
                GCC_JIT_UNARY_OP_MINUS, result_type, a_rv);
        *current_rvalue = rv_result;
        return;
    } else if (type == ast_type::DOUBLE_LITERAL) {
        /* A double literal is an rvalue. */
        auto dlit_node = dynamic_cast<ast_node_double_literal*>(node);
        *current_rvalue = gcc_jit_context_new_rvalue_from_double(context, DOUBLE_TYPE, dlit_node->d);
        if (*current_rvalue == nullptr)
            throw std::runtime_error("DOUBLE_LITERAL current_rvalue is null");
        return;
    } else
        throw std::runtime_error("walk_tree not implemented: " + std::to_string((int)node->type));
}

#if 0
emc_type resolve()
{
    return value_type = standard_type_promotion(first->resolve(), sec->resolve());
}

emc_type resolve()
{
    return value_type = emc_type{emc_types::STRING};
}
emc_type resolve()
{
    return value_type = emc_type{emc_types::INT};
}
emc_type resolve()
{
    return value_type = emc_type{emc_types::NONE};
}

emc_type resolve()
{
    return value_type = first->resolve();
}
emc_type resolve()
{
    /* If we don't find any real type in the list the type is NONE.
     * I.e. can't be used for expression assignments etc.
     */
    emc_type t = emc_type{emc_types::NONE};
    for (auto e : v_nodes) {
        auto et = e->resolve();
        if (t.types[0] == emc_types::NONE && et.types[0] != emc_types::NONE)
            t = et;
    }
    return t;
}

#endif
