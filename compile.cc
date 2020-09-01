#include <iostream>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <sstream>
#include <iomanip>

#include "compile.hh"
#include "common.hh"

#define INT_TYPE this->types->int_type
#define DOUBLE_TYPE this->types->double_type
#define VOID_TYPE this->types->void_type
#define BOOL_TYPE this->types->bool_type


std::string new_unique_name(std::string prefix = "")
{
    static long i;
    std::ostringstream ss;
    ss << prefix << "_" << std::setw(7) << std::setfill('0') <<  i++;

    return ss.str();
}

std::vector<gcc_jit_type*> v_return_type; /* Stack to keep track of return type of function defs. */
std::vector<bool> v_block_terminated;     /* To keep track off if the block was terminated depper in the tree. */


void jit::push_lval(std::string name, gcc_jit_lvalue* lval)
{
    auto &scope = get_current_scope();
    if (scope.find(name) != scope.end())
        throw std::runtime_error("Scope already contains the lval: " + name);
    scope[name] = lval;
}

gcc_jit_lvalue* jit::find_lval_in_scopes(std::string name)
{
    /* Search backwards since most nested scope has priority. */
    for (auto rit = v_of_map_of_varname_to_lval.rbegin();
            rit != v_of_map_of_varname_to_lval.rend(); rit++) {
        auto ans = rit->find(name);
        if (ans != rit->end())
            return ans->second;
    }
    return nullptr;
}

gcc_jit_rvalue* jit::cast_to(gcc_jit_rvalue *a_rv,
                        gcc_jit_type *target_type)
{
    gcc_jit_type *at = gcc_jit_rvalue_get_type(a_rv);
    if (at == target_type)
        return a_rv;
    return gcc_jit_context_new_cast(context, 0, a_rv, target_type);
}

gcc_jit_type* jit::promote_types(gcc_jit_rvalue *a_rv,
                        gcc_jit_rvalue *b_rv,
                        gcc_jit_rvalue **a_casted_rv,
                        gcc_jit_rvalue **b_casted_rv)
{
    gcc_jit_type *at = gcc_jit_rvalue_get_type(a_rv);
    gcc_jit_type *bt = gcc_jit_rvalue_get_type(b_rv);

    /* TODO: Kanske borde göra en LuT för casterna? */
    if (at == bt) { /* Same types no cast needed. */
        *a_casted_rv = a_rv;
        *b_casted_rv = b_rv;
        return bt;
    } else if (at == DOUBLE_TYPE || bt == DOUBLE_TYPE) { /* Double has highest prio. */
        if (at != DOUBLE_TYPE) {
            *a_casted_rv = gcc_jit_context_new_cast(context, 0, a_rv, DOUBLE_TYPE);
            *b_casted_rv = b_rv;
            return bt;
        } else {
            *a_casted_rv = a_rv;
            *b_casted_rv = gcc_jit_context_new_cast(context, 0, b_rv, DOUBLE_TYPE);
            return at;
        }
    } else if (at == INT_TYPE || bt == INT_TYPE) { /* In practice, cast bool to int */
        if (at != INT_TYPE) {
            *a_casted_rv = gcc_jit_context_new_cast(context, 0, a_rv, INT_TYPE);
            *b_casted_rv = b_rv;
            return bt;
        } else {
            *a_casted_rv = a_rv;
            *b_casted_rv = gcc_jit_context_new_cast(context, 0, b_rv, INT_TYPE);
            return at;
        }
    } else 
        throw std::runtime_error("Cast not supported");
}

void jit::dump(std::string path)
{
    gcc_jit_context_dump_to_file(context, path.c_str(), 0);
    gcc_jit_context_dump_reproducer_to_file(context, (path + ".c").c_str());
}

gcc_jit_type* jit::emc_type_to_jit_type(emc_type t)
{
    if (t.is_double())
        return DOUBLE_TYPE;
    else if (t.is_int())
        return INT_TYPE;
    else if (t.is_void())
        return VOID_TYPE;
    else
        DEBUG_ASSERT(false, "Not implemented");
    return 0;
}


void jit::compile()
{
    /* Close the root_block in root_func */
    gcc_jit_block_end_with_void_return(root_block, 0);
          
    result = gcc_jit_context_compile(context);
    if (!result)
    {
      std::cerr <<  "NULL result from JIT compilation" << std::endl;
    }
    /*
    gcc_jit_context_compile_to_file (context,
				 GCC_JIT_OUTPUT_KIND_ASSEMBLER,
				 "./ass.out");
    gcc_jit_context_compile_to_file (context,
				 GCC_JIT_OUTPUT_KIND_EXECUTABLE,
				 "./a.out");
    gcc_jit_context_compile_to_file (context,
				 GCC_JIT_OUTPUT_KIND_OBJECT_FILE,
				 "./a.o");   */        
}

void jit::execute()
{
    /* Begin calling the root_fn that initializes stuff like globals etc */
    typedef void (*root_fn_p)(void);
    root_fn_p root_func = (root_fn_p)gcc_jit_result_get_code(result, "root_fn");
    if (!root_func)
    {
      throw std::runtime_error("NULL root function after JIT compilation");
    } else
        root_func();

    for (auto e : v_node_fn_names) {
        root_fn_p root_func = (root_fn_p)gcc_jit_result_get_code(result, e.c_str());
        if (!root_func)
        {
            throw std::runtime_error("NULL function " + e + " after JIT compilation");
        }
        root_func();
    }
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
    gcc_jit_context_set_bool_option(context, GCC_JIT_BOOL_OPTION_DUMP_INITIAL_GIMPLE, 0);
    gcc_jit_context_set_bool_option(context,
                                            GCC_JIT_BOOL_OPTION_DUMP_GENERATED_CODE,
                                            0);
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
    /* Definition at file scope are special in that they require no function for themself
     * to wrap them but use the root init function in the case of global vars and just
     * define themself at filescope for functions. 
     */
    if (node->type == ast_type::DEF || node->type == ast_type::FUNCTION_DECLARATION ||
        node->type == ast_type::FUNCTION_DEFINITION) {
        gcc_jit_rvalue *rval = nullptr;
        walk_tree(node, 0, 0, &rval, 0);
    } else {
        gcc_jit_rvalue *rval = nullptr;
        std::string node_fn_name = new_unique_name("node_fn");

        gcc_jit_function *node_func = gcc_jit_context_new_function (
                                            context, NULL,
                                            GCC_JIT_FUNCTION_EXPORTED,
                                            VOID_TYPE,
                                            node_fn_name.c_str(),
                                            0, 0, 0);

        int block_depth = v_block_terminated.size();
        v_block_terminated.push_back(false);                                    
        gcc_jit_block *node_block = gcc_jit_function_new_block(node_func, new_unique_name("node_block").c_str());
        walk_tree(node, &node_block, &node_func, &rval, 0);

        if (rval) { /* We just print top level rvals to stdout */
            gcc_jit_type *t = gcc_jit_rvalue_get_type(rval);
            /* TODO: Lägg i en fin global istället? */
            gcc_jit_rvalue *rv_call = nullptr;
            if (t == INT_TYPE)
                rv_call = gcc_jit_context_new_call(context, 0, map_fnname_to_gccfnobj["printnl_int"], 1, &rval);
            else if (t == DOUBLE_TYPE)
                rv_call = gcc_jit_context_new_call(context, 0, map_fnname_to_gccfnobj["printnl_double"], 1, &rval);
            else if (t == BOOL_TYPE) {
                gcc_jit_rvalue *casted_rv = cast_to(rval, INT_TYPE);
                rv_call = gcc_jit_context_new_call(context, 0, map_fnname_to_gccfnobj["printnl_int"], 1, &casted_rv);
            }
            if (rv_call)
                gcc_jit_block_add_eval(node_block, 0, rv_call);
            else
                gcc_jit_block_add_eval(node_block, 0, rval);
        }

        /* End the root block with a "return;" */
        if (v_block_terminated.back() == false)
            gcc_jit_block_end_with_void_return(node_block, 0);
        v_block_terminated.pop_back();
        DEBUG_ASSERT(block_depth == v_block_terminated.size(), "Messed up block terminations");
        /* TODO: Push only function declarations. Expressions should be stored as there rvalues and be globals. */
        v_node_fn_names.push_back(node_fn_name);
    }
}

void jit::walk_tree_add(ast_node *node, gcc_jit_block **current_block, gcc_jit_function **current_function, gcc_jit_rvalue **current_rvalue)
{
    DEBUG_ASSERT_NOTNULL(current_rvalue);
    DEBUG_ASSERT_NOTNULL(node);
    auto t_node = dynamic_cast<ast_node_add*>(node);
    DEBUG_ASSERT_NOTNULL(t_node);
    /* Resolve types. */

    /* Get r-value a */
    emc_type rt = t_node->value_type;
    gcc_jit_type *result_type_emc = emc_type_to_jit_type(rt); /* TODO: Remove? */

    gcc_jit_rvalue *a_rv = nullptr;
    walk_tree(t_node->first, current_block, current_function, &a_rv, 0);
    DEBUG_ASSERT_NOTNULL(a_rv);

    gcc_jit_rvalue *b_rv = nullptr;
    walk_tree(t_node->sec, current_block, current_function, &b_rv);
    DEBUG_ASSERT_NOTNULL(b_rv);

    gcc_jit_rvalue *a_casted_rv = nullptr;
    gcc_jit_rvalue *b_casted_rv = nullptr;
    gcc_jit_type *result_type = promote_types(a_rv, b_rv, &a_casted_rv, &b_casted_rv);
    DEBUG_ASSERT_NOTNULL(result_type);
    DEBUG_ASSERT(result_type_emc == result_type, "Not anticipated type");

    gcc_jit_rvalue *rv_result = gcc_jit_context_new_binary_op(
                                    context, nullptr, GCC_JIT_BINARY_OP_PLUS,
                                    result_type, a_casted_rv, b_casted_rv);
    DEBUG_ASSERT_NOTNULL(rv_result);
    *current_rvalue = rv_result;
}

void jit::walk_tree_sub(ast_node *node, gcc_jit_block **current_block, gcc_jit_function **current_function, gcc_jit_rvalue **current_rvalue)
{
    DEBUG_ASSERT_NOTNULL(current_rvalue);
    DEBUG_ASSERT_NOTNULL(node);
    auto t_node = dynamic_cast<ast_node_sub*>(node);
    DEBUG_ASSERT_NOTNULL(t_node);
    /* Resolve types. */

    /* Get r-value a */
    emc_type rt = t_node->value_type;
    gcc_jit_type *result_type_emc = emc_type_to_jit_type(rt);

    gcc_jit_rvalue *a_rv = nullptr;
    walk_tree(t_node->first, current_block, current_function, &a_rv);
    DEBUG_ASSERT_NOTNULL(a_rv);
 
    gcc_jit_rvalue *b_rv = nullptr;
    walk_tree(t_node->sec, current_block, current_function, &b_rv);
    DEBUG_ASSERT_NOTNULL(b_rv);
 
    gcc_jit_rvalue *a_casted_rv = nullptr;
    gcc_jit_rvalue *b_casted_rv = nullptr;
    gcc_jit_type *result_type = promote_types(a_rv, b_rv, &a_casted_rv, &b_casted_rv);
    DEBUG_ASSERT_NOTNULL(result_type);
    DEBUG_ASSERT(result_type_emc == result_type, "Not anticipated type");

    gcc_jit_rvalue *rv_result = gcc_jit_context_new_binary_op(
                                    context, nullptr, GCC_JIT_BINARY_OP_MINUS,
                                    result_type, a_casted_rv, b_casted_rv);
    DEBUG_ASSERT_NOTNULL(rv_result);
    *current_rvalue = rv_result;
}

void jit::walk_tree_mul(ast_node *node, 
                        gcc_jit_block **current_block, 
                        gcc_jit_function **current_function, 
                        gcc_jit_rvalue **current_rvalue)
{
    DEBUG_ASSERT_NOTNULL(current_rvalue);
    DEBUG_ASSERT_NOTNULL(node);
    auto t_node = dynamic_cast<ast_node_mul*>(node);
    DEBUG_ASSERT_NOTNULL(t_node);
    /* Resolve types. */

    /* Get r-value a */
    emc_type rt = t_node->value_type;
    gcc_jit_type *result_type_emc = emc_type_to_jit_type(rt);

    gcc_jit_rvalue *a_rv = nullptr;
    walk_tree(t_node->first, current_block, current_function, &a_rv);
    DEBUG_ASSERT_NOTNULL(a_rv);
    gcc_jit_rvalue *b_rv = nullptr;
    walk_tree(t_node->sec, current_block, current_function, &b_rv);
    DEBUG_ASSERT_NOTNULL(b_rv);

    gcc_jit_rvalue *a_casted_rv = nullptr;
    gcc_jit_rvalue *b_casted_rv = nullptr;
    gcc_jit_type *result_type = promote_types(a_rv, b_rv, &a_casted_rv, &b_casted_rv);
    DEBUG_ASSERT_NOTNULL(result_type);
    DEBUG_ASSERT(result_type_emc == result_type, "Not anticipated type");

    gcc_jit_rvalue *rv_result = gcc_jit_context_new_binary_op(
                                    context, nullptr, GCC_JIT_BINARY_OP_MULT,
                                    result_type, a_casted_rv, b_casted_rv);
    DEBUG_ASSERT_NOTNULL(rv_result);
    *current_rvalue = rv_result;
}

void jit::walk_tree_rdiv(ast_node *node, 
                        gcc_jit_block **current_block, 
                        gcc_jit_function **current_function, 
                        gcc_jit_rvalue **current_rvalue)
{
    DEBUG_ASSERT_NOTNULL(current_rvalue);
    DEBUG_ASSERT_NOTNULL(node);
    auto t_node = dynamic_cast<ast_node_rdiv*>(node);
    DEBUG_ASSERT_NOTNULL(t_node);
    /* Resolve types. */

    /* Get r-value a */
    emc_type rt = t_node->value_type;
    gcc_jit_type *result_type_emc = emc_type_to_jit_type(rt);

    gcc_jit_rvalue *a_rv = nullptr;
    walk_tree(t_node->first, current_block, current_function, &a_rv);
    DEBUG_ASSERT_NOTNULL(a_rv);
    gcc_jit_rvalue *b_rv = nullptr;
    walk_tree(t_node->sec, current_block, current_function, &b_rv);
    DEBUG_ASSERT_NOTNULL(b_rv);

    gcc_jit_rvalue *a_casted_rv = nullptr;
    gcc_jit_rvalue *b_casted_rv = nullptr;
    gcc_jit_type *result_type = promote_types(a_rv, b_rv, &a_casted_rv, &b_casted_rv);
    DEBUG_ASSERT_NOTNULL(result_type);
    DEBUG_ASSERT(result_type_emc == result_type, "Not anticipated type");

    gcc_jit_rvalue *rv_result = gcc_jit_context_new_binary_op(
                                    context, nullptr, GCC_JIT_BINARY_OP_DIVIDE,
                                    result_type, a_casted_rv, b_casted_rv);
    DEBUG_ASSERT_NOTNULL(rv_result);                                
    *current_rvalue = rv_result;
}

void jit::walk_tree_and(ast_node *node, 
                        gcc_jit_block **current_block, 
                        gcc_jit_function **current_function, 
                        gcc_jit_rvalue **current_rvalue)
{
    DEBUG_ASSERT_NOTNULL(current_rvalue);
    DEBUG_ASSERT_NOTNULL(node);
    auto t_node = dynamic_cast<ast_node_and*>(node);
    DEBUG_ASSERT_NOTNULL(t_node);
    /* Resolve types. */

    /* Get r-value a */
    emc_type rt = t_node->value_type;

    gcc_jit_rvalue *a_rv = nullptr;
    walk_tree(t_node->first, current_block, current_function, &a_rv);
    DEBUG_ASSERT_NOTNULL(a_rv);
    gcc_jit_rvalue *b_rv = nullptr;
    walk_tree(t_node->sec, current_block, current_function, &b_rv);
    DEBUG_ASSERT_NOTNULL(b_rv);

    gcc_jit_rvalue *a_casted_rv = cast_to(a_rv, INT_TYPE);
    gcc_jit_rvalue *b_casted_rv = cast_to(b_rv, INT_TYPE);
    
    /* Create locals to store the rvals (to only eval them once) */
    gcc_jit_lvalue *temp_a_lv = gcc_jit_function_new_local(*current_function, 0,
                                        INT_TYPE, new_unique_name("and_a_tmp").c_str());
    gcc_jit_lvalue *temp_b_lv = gcc_jit_function_new_local(*current_function, 0,
                                        INT_TYPE, new_unique_name("and_b_tmp").c_str());
    gcc_jit_block_add_assignment(*current_block, 0, temp_a_lv, a_casted_rv);
    gcc_jit_block_add_assignment(*current_block, 0, temp_b_lv, b_casted_rv);
    gcc_jit_rvalue *temp_a_rv = gcc_jit_lvalue_as_rvalue(temp_a_lv);
    gcc_jit_rvalue *temp_b_rv = gcc_jit_lvalue_as_rvalue(temp_b_lv);

    gcc_jit_rvalue *rv_result = gcc_jit_context_new_binary_op(
                                    context, nullptr, GCC_JIT_BINARY_OP_LOGICAL_AND,
                                    INT_TYPE, temp_a_rv, temp_b_rv);
    DEBUG_ASSERT_NOTNULL(rv_result);                                
    *current_rvalue = rv_result;
}

void jit::walk_tree_not(ast_node *node, 
                        gcc_jit_block **current_block, 
                        gcc_jit_function **current_function, 
                        gcc_jit_rvalue **current_rvalue)
{
    DEBUG_ASSERT_NOTNULL(current_rvalue);
    DEBUG_ASSERT_NOTNULL(node);
    auto t_node = dynamic_cast<ast_node_not*>(node);
    DEBUG_ASSERT_NOTNULL(t_node);
    /* Resolve types. */

    /* Get r-value a */
    emc_type rt = t_node->value_type;

    gcc_jit_rvalue *a_rv = nullptr;
    walk_tree(t_node->first, current_block, current_function, &a_rv);
    DEBUG_ASSERT_NOTNULL(a_rv);

    gcc_jit_rvalue *a_casted_rv = cast_to(a_rv, INT_TYPE);
    
    /* Create locals to store the rvals (to only eval them once) */
    gcc_jit_lvalue *temp_a_lv = gcc_jit_function_new_local(*current_function, 0,
                                        INT_TYPE, new_unique_name("not_a_tmp").c_str());
    gcc_jit_block_add_assignment(*current_block, 0, temp_a_lv, a_casted_rv);
    gcc_jit_rvalue *temp_a_rv = gcc_jit_lvalue_as_rvalue(temp_a_lv);

    gcc_jit_rvalue *rv_result = gcc_jit_context_new_unary_op(
                                    context, nullptr, GCC_JIT_UNARY_OP_LOGICAL_NEGATE,
                                    INT_TYPE, temp_a_rv);
    DEBUG_ASSERT_NOTNULL(rv_result);                                
    *current_rvalue = rv_result;
}

void jit::walk_tree_or(ast_node *node, 
                        gcc_jit_block **current_block, 
                        gcc_jit_function **current_function, 
                        gcc_jit_rvalue **current_rvalue)
{
    DEBUG_ASSERT_NOTNULL(current_rvalue);
    DEBUG_ASSERT_NOTNULL(node);
    auto t_node = dynamic_cast<ast_node_or*>(node);
    DEBUG_ASSERT_NOTNULL(t_node);
    /* Resolve types. */

    /* Get r-value a */
    emc_type rt = t_node->value_type;

    gcc_jit_rvalue *a_rv = nullptr;
    walk_tree(t_node->first, current_block, current_function, &a_rv);
    DEBUG_ASSERT_NOTNULL(a_rv);
    gcc_jit_rvalue *b_rv = nullptr;
    walk_tree(t_node->sec, current_block, current_function, &b_rv);
    DEBUG_ASSERT_NOTNULL(b_rv);

    gcc_jit_rvalue *a_casted_rv = cast_to(a_rv, INT_TYPE);
    gcc_jit_rvalue *b_casted_rv = cast_to(b_rv, INT_TYPE);

    /* Create locals to store the rvals (to only eval them once) */
    gcc_jit_lvalue *temp_a_lv = gcc_jit_function_new_local(*current_function, 0,
                                        INT_TYPE, new_unique_name("or_a_tmp").c_str());
    gcc_jit_lvalue *temp_b_lv = gcc_jit_function_new_local(*current_function, 0,
                                        INT_TYPE, new_unique_name("or_b_tmp").c_str());
    gcc_jit_block_add_assignment(*current_block, 0, temp_a_lv, a_casted_rv);
    gcc_jit_block_add_assignment(*current_block, 0, temp_b_lv, b_casted_rv);
    gcc_jit_rvalue *temp_a_rv = gcc_jit_lvalue_as_rvalue(temp_a_lv);
    gcc_jit_rvalue *temp_b_rv = gcc_jit_lvalue_as_rvalue(temp_b_lv);

    gcc_jit_rvalue *rv_result = gcc_jit_context_new_binary_op(
                                    context, nullptr, GCC_JIT_BINARY_OP_LOGICAL_OR,
                                    INT_TYPE, temp_a_rv, temp_b_rv);
    DEBUG_ASSERT_NOTNULL(rv_result);                                
    *current_rvalue = rv_result;
}

void jit::walk_tree_xor( ast_node *node,
                         gcc_jit_block **current_block, 
                         gcc_jit_function **current_function,
                         gcc_jit_rvalue **current_rvalue)
{
    DEBUG_ASSERT_NOTNULL(current_rvalue);
    DEBUG_ASSERT_NOTNULL(current_block);
    DEBUG_ASSERT_NOTNULL(current_function);
    DEBUG_ASSERT_NOTNULL(*current_block);
    DEBUG_ASSERT_NOTNULL(*current_function);

    DEBUG_ASSERT_NOTNULL(node);
    auto t_node = dynamic_cast<ast_node_xor*>(node);
    DEBUG_ASSERT_NOTNULL(t_node);
    /* Resolve types. */

    /* Get r-value a */
    emc_type rt = t_node->value_type;

    gcc_jit_rvalue *a_rv = nullptr;
    walk_tree(t_node->first, current_block, current_function, &a_rv);
    DEBUG_ASSERT_NOTNULL(a_rv);
    gcc_jit_rvalue *b_rv = nullptr;
    walk_tree(t_node->sec, current_block, current_function, &b_rv);
    DEBUG_ASSERT_NOTNULL(b_rv);

    gcc_jit_rvalue *a_casted_rv = cast_to(a_rv, INT_TYPE);
    gcc_jit_rvalue *b_casted_rv = cast_to(b_rv, INT_TYPE);

    /* Create locals to store the rvals (to only eval them once) */
    gcc_jit_lvalue *temp_a_lv = gcc_jit_function_new_local(*current_function, 0,
                                        INT_TYPE, new_unique_name("xor_a_tmp").c_str());
    gcc_jit_lvalue *temp_b_lv = gcc_jit_function_new_local(*current_function, 0,
                                        INT_TYPE, new_unique_name("xor_b_tmp").c_str());
    gcc_jit_block_add_assignment(*current_block, 0, temp_a_lv, a_casted_rv);
    gcc_jit_block_add_assignment(*current_block, 0, temp_b_lv, b_casted_rv);

    /* a XOR b = (a OR b) AND NOT (a AND b) */
    gcc_jit_rvalue *temp_a_rv = gcc_jit_lvalue_as_rvalue(temp_a_lv);
    gcc_jit_rvalue *temp_b_rv = gcc_jit_lvalue_as_rvalue(temp_b_lv);

    
    gcc_jit_rvalue *aorb_rv = gcc_jit_context_new_binary_op(context, nullptr, GCC_JIT_BINARY_OP_LOGICAL_OR, 
                                        INT_TYPE, temp_a_rv, temp_b_rv);
    gcc_jit_rvalue *aandb_rv = gcc_jit_context_new_binary_op(context, nullptr, GCC_JIT_BINARY_OP_LOGICAL_AND, 
                                        INT_TYPE, temp_a_rv, temp_b_rv);
    gcc_jit_rvalue *not_aandb_rv = gcc_jit_context_new_unary_op(context, 0, GCC_JIT_UNARY_OP_LOGICAL_NEGATE, 
                                        INT_TYPE, aandb_rv);
    gcc_jit_rvalue *rv_result = gcc_jit_context_new_binary_op(
                                    context, nullptr, GCC_JIT_BINARY_OP_LOGICAL_AND,
                                    INT_TYPE, aorb_rv, not_aandb_rv);
    DEBUG_ASSERT_NOTNULL(rv_result);                                
    *current_rvalue = rv_result;
}

void jit::walk_tree_nor( ast_node *node,
                         gcc_jit_block **current_block, 
                         gcc_jit_function **current_function,
                         gcc_jit_rvalue **current_rvalue)
{
    DEBUG_ASSERT_NOTNULL(current_rvalue);
    DEBUG_ASSERT_NOTNULL(current_block);
    DEBUG_ASSERT_NOTNULL(current_function);
    DEBUG_ASSERT_NOTNULL(*current_block);
    DEBUG_ASSERT_NOTNULL(*current_function);

    DEBUG_ASSERT_NOTNULL(node);
    auto t_node = dynamic_cast<ast_node_nor*>(node);
    DEBUG_ASSERT_NOTNULL(t_node);
    /* Resolve types. */

    /* Get r-value a */
    emc_type rt = t_node->value_type;

    gcc_jit_rvalue *a_rv = nullptr;
    walk_tree(t_node->first, current_block, current_function, &a_rv);
    DEBUG_ASSERT_NOTNULL(a_rv);
    gcc_jit_rvalue *b_rv = nullptr;
    walk_tree(t_node->sec, current_block, current_function, &b_rv);
    DEBUG_ASSERT_NOTNULL(b_rv);

    gcc_jit_rvalue *a_casted_rv = cast_to(a_rv, INT_TYPE);
    gcc_jit_rvalue *b_casted_rv = cast_to(b_rv, INT_TYPE);

    /* Create locals to store the rvals (to only eval them once) */
    gcc_jit_lvalue *temp_a_lv = gcc_jit_function_new_local(*current_function, 0,
                                        INT_TYPE, new_unique_name("nor_a_tmp").c_str());
    gcc_jit_lvalue *temp_b_lv = gcc_jit_function_new_local(*current_function, 0,
                                        INT_TYPE, new_unique_name("nor_b_tmp").c_str());
    gcc_jit_block_add_assignment(*current_block, 0, temp_a_lv, a_casted_rv);
    gcc_jit_block_add_assignment(*current_block, 0, temp_b_lv, b_casted_rv);

    /* a XOR b = (a OR b) AND NOT (a AND b) */
    gcc_jit_rvalue *temp_a_rv = gcc_jit_lvalue_as_rvalue(temp_a_lv);
    gcc_jit_rvalue *temp_b_rv = gcc_jit_lvalue_as_rvalue(temp_b_lv);

    
    gcc_jit_rvalue *aorb_rv = gcc_jit_context_new_binary_op(context, nullptr, GCC_JIT_BINARY_OP_LOGICAL_OR, 
                                        INT_TYPE, temp_a_rv, temp_b_rv);
    gcc_jit_rvalue *rv_result = gcc_jit_context_new_unary_op(context, 0, GCC_JIT_UNARY_OP_LOGICAL_NEGATE, 
                                        INT_TYPE, aorb_rv);
    DEBUG_ASSERT_NOTNULL(rv_result);                                
    *current_rvalue = rv_result;
}

void jit::walk_tree_nand( ast_node *node,
                         gcc_jit_block **current_block, 
                         gcc_jit_function **current_function,
                         gcc_jit_rvalue **current_rvalue)
{
    DEBUG_ASSERT_NOTNULL(current_rvalue);
    DEBUG_ASSERT_NOTNULL(current_block);
    DEBUG_ASSERT_NOTNULL(current_function);
    DEBUG_ASSERT_NOTNULL(*current_block);
    DEBUG_ASSERT_NOTNULL(*current_function);

    DEBUG_ASSERT_NOTNULL(node);
    auto t_node = dynamic_cast<ast_node_nand*>(node);
    DEBUG_ASSERT_NOTNULL(t_node);
    /* Resolve types. */

    /* Get r-value a */
    emc_type rt = t_node->value_type;

    gcc_jit_rvalue *a_rv = nullptr;
    walk_tree(t_node->first, current_block, current_function, &a_rv);
    DEBUG_ASSERT_NOTNULL(a_rv);
    gcc_jit_rvalue *b_rv = nullptr;
    walk_tree(t_node->sec, current_block, current_function, &b_rv);
    DEBUG_ASSERT_NOTNULL(b_rv);

    gcc_jit_rvalue *a_casted_rv = cast_to(a_rv, INT_TYPE);
    gcc_jit_rvalue *b_casted_rv = cast_to(b_rv, INT_TYPE);
    /* TODO: Should be bool? */

    /* Create locals to store the rvals (to only eval them once) */
    gcc_jit_lvalue *temp_a_lv = gcc_jit_function_new_local(*current_function, 0,
                                        INT_TYPE, new_unique_name("nand_a_tmp").c_str());
    gcc_jit_lvalue *temp_b_lv = gcc_jit_function_new_local(*current_function, 0,
                                        INT_TYPE, new_unique_name("nand_b_tmp").c_str());
    gcc_jit_block_add_assignment(*current_block, 0, temp_a_lv, a_casted_rv);
    gcc_jit_block_add_assignment(*current_block, 0, temp_b_lv, b_casted_rv);
 
    gcc_jit_rvalue *temp_a_rv = gcc_jit_lvalue_as_rvalue(temp_a_lv);
    gcc_jit_rvalue *temp_b_rv = gcc_jit_lvalue_as_rvalue(temp_b_lv);

    
    gcc_jit_rvalue *aandb_rv = gcc_jit_context_new_binary_op(context, nullptr, GCC_JIT_BINARY_OP_LOGICAL_AND, 
                                        INT_TYPE, temp_a_rv, temp_b_rv);
    gcc_jit_rvalue *rv_result = gcc_jit_context_new_unary_op(context, 0, GCC_JIT_UNARY_OP_LOGICAL_NEGATE, 
                                        INT_TYPE, aandb_rv);
    DEBUG_ASSERT_NOTNULL(rv_result);                                
    *current_rvalue = rv_result;
}

void jit::walk_tree_xnor( ast_node *node,
                         gcc_jit_block **current_block, 
                         gcc_jit_function **current_function,
                         gcc_jit_rvalue **current_rvalue)
{
    DEBUG_ASSERT_NOTNULL(current_rvalue);
    DEBUG_ASSERT_NOTNULL(current_block);
    DEBUG_ASSERT_NOTNULL(current_function);
    DEBUG_ASSERT_NOTNULL(*current_block);
    DEBUG_ASSERT_NOTNULL(*current_function);

    DEBUG_ASSERT_NOTNULL(node);
    auto t_node = dynamic_cast<ast_node_xnor*>(node);
    DEBUG_ASSERT_NOTNULL(t_node);
    /* Resolve types. */

    /* Get r-value a */
    emc_type rt = t_node->value_type;

    gcc_jit_rvalue *a_rv = nullptr;
    walk_tree(t_node->first, current_block, current_function, &a_rv);
    DEBUG_ASSERT_NOTNULL(a_rv);
    gcc_jit_rvalue *b_rv = nullptr;
    walk_tree(t_node->sec, current_block, current_function, &b_rv);
    DEBUG_ASSERT_NOTNULL(b_rv);

    gcc_jit_rvalue *a_casted_rv = cast_to(a_rv, INT_TYPE);
    gcc_jit_rvalue *b_casted_rv = cast_to(b_rv, INT_TYPE);

    /* Create locals to store the rvals (to only eval them once) */
    gcc_jit_lvalue *temp_a_lv = gcc_jit_function_new_local(*current_function, 0,
                                        INT_TYPE, new_unique_name("xor_a_tmp").c_str());
    gcc_jit_lvalue *temp_b_lv = gcc_jit_function_new_local(*current_function, 0,
                                        INT_TYPE, new_unique_name("xor_b_tmp").c_str());
    gcc_jit_block_add_assignment(*current_block, 0, temp_a_lv, a_casted_rv);
    gcc_jit_block_add_assignment(*current_block, 0, temp_b_lv, b_casted_rv);

    /* a XNOR b = (a AND b) OR NOT (a OR b) */
    gcc_jit_rvalue *temp_a_rv = gcc_jit_lvalue_as_rvalue(temp_a_lv);
    gcc_jit_rvalue *temp_b_rv = gcc_jit_lvalue_as_rvalue(temp_b_lv);

    
    gcc_jit_rvalue *aorb_rv = gcc_jit_context_new_binary_op(context, nullptr, GCC_JIT_BINARY_OP_LOGICAL_OR, 
                                        INT_TYPE, temp_a_rv, temp_b_rv);
    gcc_jit_rvalue *aandb_rv = gcc_jit_context_new_binary_op(context, nullptr, GCC_JIT_BINARY_OP_LOGICAL_AND, 
                                        INT_TYPE, temp_a_rv, temp_b_rv);
    gcc_jit_rvalue *not_aorb_rv = gcc_jit_context_new_unary_op(context, 0, GCC_JIT_UNARY_OP_LOGICAL_NEGATE, 
                                        INT_TYPE, aorb_rv);
    gcc_jit_rvalue *rv_result = gcc_jit_context_new_binary_op(
                                    context, nullptr, GCC_JIT_BINARY_OP_LOGICAL_OR,
                                    INT_TYPE, aandb_rv, not_aorb_rv);
    DEBUG_ASSERT_NOTNULL(rv_result);                                
    *current_rvalue = rv_result;
}

/* TODO: Make not global */
std::vector<gcc_jit_lvalue*> andchain_lvals;

void jit::walk_tree_andchain(ast_node *node, 
                        gcc_jit_block **current_block, 
                        gcc_jit_function **current_function, 
                        gcc_jit_rvalue **current_rvalue)
{
    DEBUG_ASSERT_NOTNULL(current_rvalue);
    DEBUG_ASSERT_NOTNULL(node);   
    auto t_node = dynamic_cast<ast_node_andchain*>(node);
    DEBUG_ASSERT_NOTNULL(t_node); 
    DEBUG_ASSERT(t_node->v_children.size(), "No children in andchain");  
    /* TODO: Fix chaining somewhere here */
    /* Resolve types. */

    /* If there are only one child (two comparations) there is no need for locals
        * to hold values. */
    if (t_node->v_children.size() == 1) {
        gcc_jit_rvalue *rv = nullptr;
        walk_tree(t_node->v_children[0], current_block, current_function, &rv);
        *current_rvalue = rv;
        return;
    } else {
        /* We need to chain the comparations like this: 
         * a < b && b < c
         * 
         * TODO: Is this implementation nice looking enought?
         */
        
        { /* First rval to compare, make a local for it */
            gcc_jit_rvalue *a_rv = nullptr; 
            walk_tree(t_node->v_children[0]->first.get(), current_block, current_function, &a_rv);
            DEBUG_ASSERT_NOTNULL(a_rv);
            gcc_jit_lvalue *a_lv = gcc_jit_function_new_local(*current_function, 0, 
                                        gcc_jit_rvalue_get_type(a_rv), 
                                        new_unique_name("cmp_chain_tmp").c_str());
            gcc_jit_block_add_assignment(*current_block, 0, a_lv, a_rv);
            andchain_lvals.push_back(a_lv);
        }

        /* Make locals for the right operand for the rest of the compares. */
        for (auto e : t_node->v_children) {
            gcc_jit_rvalue *b_rv = nullptr; 
            walk_tree(e->sec.get(), current_block, current_function, &b_rv);
            DEBUG_ASSERT_NOTNULL(b_rv);
            gcc_jit_lvalue *b_lv = gcc_jit_function_new_local(*current_function, 0, 
                                        gcc_jit_rvalue_get_type(b_rv), 
                                        new_unique_name("cmp_chain_tmp").c_str());
            gcc_jit_block_add_assignment(*current_block, 0, b_lv, b_rv);
            andchain_lvals.push_back(b_lv);
        }
        

        /* Walk the tree to get rvalues for each comparasion. Note ugly global vector
         * that overrides two operand behavour in the comparation walk_tree functions ...
         * Also note backwards iterator .
         */
        std::vector<gcc_jit_rvalue*> v_rvals;
        for (auto e = t_node->v_children.rbegin(); e != t_node->v_children.rend(); e++) {
            gcc_jit_rvalue *rv = nullptr;
            walk_tree(*e, current_block, current_function, &rv);
            DEBUG_ASSERT_NOTNULL(rv);
            v_rvals.push_back(rv);
        }
        andchain_lvals.pop_back(); /* Pops the left most rval. */

        /* Iterate over v_rvals backwards (v_rvals is in it self "backwards")
         * and create &&:s.
         */
        gcc_jit_rvalue *rv_and_head = gcc_jit_context_new_binary_op(context, 0,
                                        GCC_JIT_BINARY_OP_LOGICAL_AND, BOOL_TYPE, *v_rvals.rbegin(), *(v_rvals.rbegin() + 1));
        for (auto e = v_rvals.rbegin() + 2; e != v_rvals.rend(); e++) {
            gcc_jit_rvalue *rv = *e;
            rv_and_head = gcc_jit_context_new_binary_op(context, 0,
                                        GCC_JIT_BINARY_OP_LOGICAL_AND, BOOL_TYPE, rv_and_head, rv);
        }
        *current_rvalue = rv_and_head;
        DEBUG_ASSERT(andchain_lvals.size() == 0, "Not all lvals popped");
    }
}

#define CMP_FNC(emc_ast_type, gcc_jit_type) \
auto t_node = dynamic_cast<emc_ast_type*>(node);\
/* TODO: Fix chaining somewhere here */ \
/* Resolve types. */ \
\
gcc_jit_rvalue *b_rv = nullptr; \
if (andchain_lvals.size() == 0) \
    walk_tree(t_node->sec.get(), current_block, current_function, &b_rv); \
else { \
    b_rv = gcc_jit_lvalue_as_rvalue(andchain_lvals.back()); \
    andchain_lvals.pop_back(); \
} \
DEBUG_ASSERT_NOTNULL(b_rv); \
\
gcc_jit_rvalue *a_rv = nullptr; \
if (andchain_lvals.size() == 0) \
    walk_tree(t_node->first.get(), current_block, current_function, &a_rv); \
else { \
    a_rv = gcc_jit_lvalue_as_rvalue(andchain_lvals.back()); \
    /* Not poped since used in other chained comparation */ \
} \
DEBUG_ASSERT_NOTNULL(a_rv); \
\
gcc_jit_rvalue *a_casted_rv = nullptr; \
gcc_jit_rvalue *b_casted_rv = nullptr; \
promote_types(a_rv, b_rv, &a_casted_rv, &b_casted_rv); \
\
gcc_jit_rvalue *rv_result = gcc_jit_context_new_comparison(\
                                context, nullptr, gcc_jit_type,\
                                a_casted_rv, b_casted_rv);\
DEBUG_ASSERT_NOTNULL(rv_result); \
*current_rvalue = rv_result;\

void jit::walk_tree_geq(ast_node *node, 
                        gcc_jit_block **current_block, 
                        gcc_jit_function **current_function, 
                        gcc_jit_rvalue **current_rvalue)
{
    DEBUG_ASSERT_NOTNULL(current_rvalue);
    DEBUG_ASSERT_NOTNULL(node); 
    CMP_FNC(ast_node_geq, GCC_JIT_COMPARISON_GE)
}

void jit::walk_tree_leq(ast_node *node, 
                        gcc_jit_block **current_block, 
                        gcc_jit_function **current_function, 
                        gcc_jit_rvalue **current_rvalue)
{
    DEBUG_ASSERT_NOTNULL(current_rvalue);
    DEBUG_ASSERT_NOTNULL(node); 
    CMP_FNC(ast_node_leq, GCC_JIT_COMPARISON_LE)
}

void jit::walk_tree_equ(ast_node *node, 
                        gcc_jit_block **current_block, 
                        gcc_jit_function **current_function, 
                        gcc_jit_rvalue **current_rvalue)
{
    DEBUG_ASSERT_NOTNULL(current_rvalue);
    DEBUG_ASSERT_NOTNULL(node); 
    CMP_FNC(ast_node_equ, GCC_JIT_COMPARISON_EQ)
}

void jit::walk_tree_les(ast_node *node, 
                        gcc_jit_block **current_block, 
                        gcc_jit_function **current_function, 
                        gcc_jit_rvalue **current_rvalue)
{
    DEBUG_ASSERT_NOTNULL(current_rvalue);
    DEBUG_ASSERT_NOTNULL(node); 
    CMP_FNC(ast_node_les, GCC_JIT_COMPARISON_LT)
}

void jit::walk_tree_gre(ast_node *node, 
                        gcc_jit_block **current_block, 
                        gcc_jit_function **current_function, 
                        gcc_jit_rvalue **current_rvalue)
{
    DEBUG_ASSERT_NOTNULL(current_rvalue);
    DEBUG_ASSERT_NOTNULL(node); 
    CMP_FNC(ast_node_gre, GCC_JIT_COMPARISON_GT)
}

void jit::walk_tree_neq(ast_node *node, 
                        gcc_jit_block **current_block, 
                        gcc_jit_function **current_function, 
                        gcc_jit_rvalue **current_rvalue)
{
    DEBUG_ASSERT_NOTNULL(current_rvalue);
    DEBUG_ASSERT_NOTNULL(node); 
    CMP_FNC(ast_node_neq, GCC_JIT_COMPARISON_NE)
}

void jit::walk_tree_uminus(ast_node *node, 
                        gcc_jit_block **current_block, 
                        gcc_jit_function **current_function, 
                        gcc_jit_rvalue **current_rvalue)
{
    auto t_node = dynamic_cast<ast_node_uminus*>(node);
    /* Resolve types. */

    /* Get r-value a */
    emc_type rt = t_node->value_type;
    gcc_jit_type *result_type = emc_type_to_jit_type(rt);

    gcc_jit_rvalue *a_rv = nullptr;
    walk_tree(t_node->first, current_block, current_function, &a_rv);
    DEBUG_ASSERT(a_rv != nullptr, "sub a is null");

    gcc_jit_rvalue *rv_result = gcc_jit_context_new_unary_op(context, nullptr,
            GCC_JIT_UNARY_OP_MINUS, result_type, a_rv);
    *current_rvalue = rv_result;
}

void jit::walk_tree_dlit(ast_node *node, 
                        gcc_jit_block **current_block, 
                        gcc_jit_function **current_function, 
                        gcc_jit_rvalue **current_rvalue)
{
    /* A double literal is an rvalue. */
    auto dlit_node = dynamic_cast<ast_node_double_literal*>(node);
    *current_rvalue = gcc_jit_context_new_rvalue_from_double(context, DOUBLE_TYPE, dlit_node->d);
    DEBUG_ASSERT(*current_rvalue != nullptr, "DOUBLE_LITERAL current_rvalue is null");        
}

void jit::walk_tree_ilit(ast_node *node, 
                        gcc_jit_block **current_block, 
                        gcc_jit_function **current_function, 
                        gcc_jit_rvalue **current_rvalue)
{
    /* A double literal is an rvalue. */
    auto ilit_node = dynamic_cast<ast_node_int_literal*>(node);
    *current_rvalue = gcc_jit_context_new_rvalue_from_int(context, INT_TYPE, ilit_node->i);
    DEBUG_ASSERT(*current_rvalue != nullptr, "INT_LITERAL current_rvalue is null");        
}

void jit::walk_tree_fcall(ast_node *node, 
                        gcc_jit_block **current_block, 
                        gcc_jit_function **current_function, 
                        gcc_jit_rvalue **current_rvalue)
{
    /* A double literal is an rvalue. */
    DEBUG_ASSERT(node != nullptr, "node was null");
    auto fcall_node = dynamic_cast<ast_node_funccall*>(node);
    DEBUG_ASSERT(fcall_node != nullptr, "FUNCTION_CALL current_rvalue is null");

    auto it = map_fnname_to_gccfnobj.find(fcall_node->name);
    if (it == map_fnname_to_gccfnobj.end())
        throw std::runtime_error("Function " + fcall_node->name + " not defined.");
    gcc_jit_function *func = it->second;

    /* Find the ast node of the corrensponding function declaration. */
    extern scope_stack resolve_scope;
    obj* fnobj_ = resolve_scope.find_object(fcall_node->name, "");
    object_func *fnobj = dynamic_cast<object_func*>(fnobj_);
    if (!fnobj)
        throw std::runtime_error("Function " + fcall_node->name + " not defined/found.");
    auto *par_list = dynamic_cast<ast_node_vardef_list*>(fnobj->para_list);
    DEBUG_ASSERT(par_list != nullptr, "par_list is null");
    auto arg_t = dynamic_cast<ast_node_arglist*>(fcall_node->arg_list);
    DEBUG_ASSERT(arg_t != nullptr, "arg_t is null");

    if (par_list->v_defs.size() != arg_t->v_ast_args.size())
        throw std::runtime_error("Function " + fcall_node->name + " incorrect number of arguments.");

    std::vector<gcc_jit_rvalue*> v_arg_rv;
    for (int i = 0; i < arg_t->v_ast_args.size(); i++) {
        ast_node *arg_node = arg_t->v_ast_args[i];

        gcc_jit_rvalue *arg_rv = nullptr;

        walk_tree(arg_node, current_block, current_function, &arg_rv);
        DEBUG_ASSERT(arg_node, "Could not produce rval from argument");
        /* The actual type of the parameter in question. */
        gcc_jit_type *arg_type = gcc_jit_rvalue_get_type(
                                    gcc_jit_param_as_rvalue(
                                        gcc_jit_function_get_param(func, i)));
        gcc_jit_rvalue *casted_rv = cast_to(arg_rv, arg_type);                                  
        DEBUG_ASSERT_NOTNULL(casted_rv);
        v_arg_rv.push_back(casted_rv);
    }

    gcc_jit_rvalue *fncall_rval = gcc_jit_context_new_call(context, 0, func, v_arg_rv.size(), v_arg_rv.data());

    *current_rvalue = fncall_rval;
}

void jit::walk_tree_fdec(ast_node *node, 
                        gcc_jit_block **current_block, 
                        gcc_jit_function **current_function, 
                        gcc_jit_rvalue **current_rvalue)
{
    DEBUG_ASSERT(node != nullptr, "node was null");
    auto ast_funcdec = dynamic_cast<ast_node_funcdef*>(node);
    DEBUG_ASSERT(ast_funcdec != nullptr, "ast_funcdec was null");
    auto ast_parlist = dynamic_cast<ast_node_vardef_list*>(ast_funcdec->parlist);
    DEBUG_ASSERT_NOTNULL(ast_parlist);

    gcc_jit_type *return_type = emc_type_to_jit_type(ast_funcdec->return_list->value_type);
    v_return_type.push_back(return_type); /* Filescope vector used at return statements ... */
    
    push_scope();
    std::vector<gcc_jit_param*> v_params;
    for (auto e : ast_parlist->v_defs) {
        auto vardef = dynamic_cast<ast_node_def*>(e);
        gcc_jit_type *type = emc_type_to_jit_type(vardef->value_type);
        gcc_jit_param *para = gcc_jit_context_new_param(context, 
                    0, type, vardef->var_name.c_str());
        v_params.push_back(para);
        /* Add the parameters to the scope as lvalues. */
        push_lval(vardef->var_name, gcc_jit_param_as_lvalue(para));
    }

    gcc_jit_function *fn = nullptr;
    if (v_params.size())
        fn = gcc_jit_context_new_function(context, 0, GCC_JIT_FUNCTION_INTERNAL,
                                    return_type, ast_funcdec->name.c_str(),
                                    v_params.size(), v_params.data(), 0);
    else 
        fn = gcc_jit_context_new_function(context, 0, GCC_JIT_FUNCTION_INTERNAL,
                                    return_type, ast_funcdec->name.c_str(),
                                    0, 0, 0);
    int block_depth = v_block_terminated.size();
    v_block_terminated.push_back(false);
    gcc_jit_block *fn_block = gcc_jit_function_new_block(fn, new_unique_name("fn_start").c_str());

    /* Add a new scope and push it with the parameter names. */
    extern scope_stack resolve_scope; /* TODO: Borde inte behöva detta i compile.cc ... refactor */
    resolve_scope.push_new_scope();
    auto &top_scope = resolve_scope.get_top_scope();
    for (auto e : ast_parlist->v_defs) {
        auto vardef = dynamic_cast<ast_node_def*>(e);
        obj *obj = nullptr;
        if (vardef->type_name == "Double") /* TODO: Borde göras i funktion eg obj_from_typename */
            obj = new object_double{vardef->var_name, 0};
        else if (vardef->type_name == "Int")
            obj = new object_int{vardef->var_name, 0};
        top_scope.push_object(obj);
    }

    gcc_jit_block *last_block = fn_block;
    gcc_jit_rvalue *rval = nullptr; /* TODO: Inge snyggt skicka denna från en func dec */
    walk_tree(ast_funcdec->code_block, &last_block, &fn, &rval);
    pop_scope();

    /* Pop the function's scope. */
    resolve_scope.pop_scope();

    map_fnname_to_gccfnobj[ast_funcdec->name.c_str()] = fn;

    v_return_type.pop_back(); /* Pop return type stack */

    /* Was the last block terminated? */
    if (v_block_terminated.back())
        v_block_terminated.pop_back();
    else
        throw std::runtime_error("Function's last block not terminated.");

    DEBUG_ASSERT(block_depth == v_block_terminated.size(), "Messup in terminations");

}

void jit::walk_tree_fdef(ast_node *node, 
                        gcc_jit_block **current_block, 
                        gcc_jit_function **current_function, 
                        gcc_jit_rvalue **current_rvalue)
{
    DEBUG_ASSERT(node != nullptr, "node was null");
    auto ast_funcdef = dynamic_cast<ast_node_funcdec*>(node);
    DEBUG_ASSERT(ast_funcdef != nullptr, "ast_funcdef was null");
    auto ast_parlist = dynamic_cast<ast_node_vardef_list*>(ast_funcdef->parlist);
    DEBUG_ASSERT_NOTNULL(ast_parlist);

    gcc_jit_type *return_type = emc_type_to_jit_type(ast_funcdef->return_list->value_type);

    std::vector<gcc_jit_param*> v_params;
    for (auto e : ast_parlist->v_defs) {
        auto vardef = dynamic_cast<ast_node_def*>(e);
        gcc_jit_type *type = emc_type_to_jit_type(vardef->value_type);
        gcc_jit_param *para = gcc_jit_context_new_param(context, 
                    0, type, vardef->var_name.c_str());
        v_params.push_back(para);
    }

    gcc_jit_function *fn = nullptr;
    if (v_params.size())
        fn = gcc_jit_context_new_function(context, 0, GCC_JIT_FUNCTION_IMPORTED,
                                    return_type, ast_funcdef->name.c_str(),
                                    v_params.size(), v_params.data(), 0);
    else 
        fn = gcc_jit_context_new_function(context, 0, GCC_JIT_FUNCTION_IMPORTED,
                                    return_type, ast_funcdef->name.c_str(),
                                    0, 0, 0);
 
    map_fnname_to_gccfnobj[ast_funcdef->name] = fn;
}

void jit::walk_tree_ret(ast_node *node, 
                        gcc_jit_block **current_block, 
                        gcc_jit_function **current_function, 
                        gcc_jit_rvalue **current_rvalue)
{
    DEBUG_ASSERT(node != nullptr, "node was null");
    auto ret_node = dynamic_cast<ast_node_return*>(node);
    DEBUG_ASSERT(ret_node != nullptr, "ret_node was null");

    

    /* Is there anything to return? "RETURN x" */
    if (ret_node->first) {
        gcc_jit_rvalue *rval = nullptr;
        walk_tree(ret_node->first, current_block, current_function, &rval);
        DEBUG_ASSERT(rval != nullptr, "Invalid rval for return");

        gcc_jit_rvalue *casted_rval = nullptr;

        gcc_jit_type *rv_type = gcc_jit_rvalue_get_type(rval);
        emc_type emc_type = ret_node->value_type;

        DEBUG_ASSERT(v_return_type.size() >= 1, "Return type stack 0 sized");
        gcc_jit_type *return_type = v_return_type.back();

        if (return_type == rv_type)
            casted_rval = rval;
        else {
            casted_rval = gcc_jit_context_new_cast(
                                context, 0, rval, return_type);
        }
        
        gcc_jit_block_end_with_return(*current_block, 0, casted_rval);
        /* TODO: Ensure there are no more returns in the block ... */
    } else { /* void return */
        gcc_jit_block_end_with_void_return(*current_block, 0);
    }

    v_block_terminated.back() = true;
}

void jit::walk_tree_assign(ast_node *node, 
                        gcc_jit_block **current_block, 
                        gcc_jit_function **current_function, 
                        gcc_jit_rvalue **current_rvalue)
{
    /* A double literal is an rvalue. */
    DEBUG_ASSERT(node != nullptr, "node was null");
    auto ass_node = dynamic_cast<ast_node_assign*>(node);
    DEBUG_ASSERT(ass_node != nullptr, "ass_node was null");

    /* TODO: There should be support for rval lval here for eg getter fncs, and array access etc */
    auto lval = dynamic_cast<ast_node_var*>(ass_node->first);
    DEBUG_ASSERT(lval != nullptr, "Other types of left hand sides in assigned not implemented");

    /* Resolve the rval of the assignment */
    gcc_jit_rvalue *rval = nullptr;
    gcc_jit_block *last_block = *current_block;
    walk_tree(ass_node->sec, &last_block, current_function, &rval);
    if (last_block != *current_block) { /* Atleast one block was added */
        *current_block = last_block; /* Point the head to the last block added */
    }
    /* Resolve the lval */
    auto gcc_lval = find_lval_in_scopes(lval->name);
    if (gcc_lval == nullptr)
        std::runtime_error("Could not resolve lval of object: " + lval->name);    

    gcc_jit_type *lv_as_rv_t = gcc_jit_rvalue_get_type(gcc_jit_lvalue_as_rvalue(gcc_lval));
    gcc_jit_type *rv_t = gcc_jit_rvalue_get_type(rval);

    gcc_jit_rvalue *casted_rval = nullptr;
    if (lv_as_rv_t == rv_t)
        casted_rval = rval;
    else {
        casted_rval = gcc_jit_context_new_cast(context, 0, rval, lv_as_rv_t);
    }
    /* Add the assignment */
    gcc_jit_block_add_assignment(*current_block, 0, gcc_lval, casted_rval);
    
}

void jit::walk_tree_var(ast_node *node, 
                        gcc_jit_block **current_block, 
                        gcc_jit_function **current_function, 
                        gcc_jit_rvalue **current_rvalue)
{
    DEBUG_ASSERT(node != nullptr, "node was null");
    auto var_node = dynamic_cast<ast_node_var*>(node);
    DEBUG_ASSERT(var_node != nullptr, "ass_node was null");

    gcc_jit_lvalue *lval = find_lval_in_scopes(var_node->name);
    if (!lval)
        throw std::runtime_error("Could not resolve lval in the current scope: " + var_node->name);

    *current_rvalue = gcc_jit_lvalue_as_rvalue(lval);
}

void jit::walk_tree_def(ast_node *node, 
                        gcc_jit_block **current_block, 
                        gcc_jit_function **current_function, 
                        gcc_jit_rvalue **current_rvalue)
{
    auto *ast_def = dynamic_cast<ast_node_def*>(node);
    /* Global or local? */
    if (scope_n_nested() == 1) { /* Global */
        gcc_jit_type *var_type = emc_type_to_jit_type(ast_def->value_type);
        gcc_jit_lvalue *lval = gcc_jit_context_new_global(
                context, 0, GCC_JIT_GLOBAL_EXPORTED,
                var_type,
                ast_def->var_name.c_str());
        push_lval(ast_def->var_name, lval);

        gcc_jit_rvalue *rv_assignment = nullptr;
        walk_tree(ast_def->value_node, current_block, current_function, &rv_assignment);
        /* Cast to the global's type */
        gcc_jit_rvalue *cast_rv = gcc_jit_context_new_cast(context, 0, rv_assignment, var_type);
        /* Assign the value in the root_block (i.e. not really like a filescope var in c */
        gcc_jit_block_add_assignment(root_block, 0, lval, cast_rv);
       
    } else { /* Local */
        gcc_jit_type *var_type = emc_type_to_jit_type(ast_def->value_type);
        gcc_jit_lvalue *lval = gcc_jit_function_new_local(*current_function, 0, var_type, ast_def->var_name.c_str());
        push_lval(ast_def->var_name, lval);

        gcc_jit_rvalue *rv_assignment = nullptr;
        walk_tree(ast_def->value_node, current_block, current_function, &rv_assignment);
        /* Cast to the local's type */
        gcc_jit_rvalue *cast_rv = gcc_jit_context_new_cast(context, 0, rv_assignment, var_type);
        /* Assign the value */
        gcc_jit_block_add_assignment(*current_block, 0, lval, cast_rv);

        /* TODO: Bör göras i emc.hh */
        extern scope_stack resolve_scope;
        obj *od = nullptr;
        if (ast_def->value_type.is_double())
            od = new object_double{ast_def->var_name, 0};
        else if (ast_def->value_type.is_int())
            od = new object_int{ast_def->var_name, 0};
        else
            throw std::runtime_error("Type not implemented walk_tree_def");
        resolve_scope.get_top_scope().push_object(od);
    }
}

void jit::walk_tree_explist( ast_node *node, 
                             gcc_jit_block **current_block, 
                             gcc_jit_function **current_function,
                             gcc_jit_rvalue **current_rvalue)
{
    auto expl_ast = dynamic_cast<ast_node_explist*>(node);

    gcc_jit_rvalue *rval = nullptr;
    for (auto e : expl_ast->v_nodes) {
        rval = nullptr;
        gcc_jit_block *last_block = *current_block;
        walk_tree(e, &last_block, current_function, &rval);

        if (last_block != *current_block) { /* Atleast one block was added */
            *current_block = last_block; /* Point the head to the last block added */
        } else if (rval) { /* There was only expressions so add it/them to the current block. */
            gcc_jit_block_add_eval(*current_block, 0, rval);
        }
    }
    /* The last rval of the expression list is its value. */
    *current_rvalue = rval;
}

void jit::walk_tree_doblock(    ast_node *node, 
                                gcc_jit_block **current_block, 
                                gcc_jit_function **current_function,
                                gcc_jit_rvalue **current_rvalue)
{
    auto do_ast = dynamic_cast<ast_node_doblock*>(node);

    gcc_jit_rvalue *rval = nullptr;
    gcc_jit_block *last_block = *current_block;
    walk_tree(do_ast->first, &last_block, current_function, &rval);

    if (last_block != *current_block) { /* Atleast one block was added */
        *current_block = last_block; /* Point the head to the last block added */
    } else if (0 && rval) { /* There was only expressions so add it/them to the current block. */
        gcc_jit_block_add_eval(*current_block, 0, rval);
    }
    /* The last rval of the expression list is its value. */
    *current_rvalue = rval;    
}

void jit::walk_tree_if(ast_node *node, 
                        gcc_jit_block **current_block, 
                        gcc_jit_function **current_function, 
                        gcc_jit_rvalue **current_rvalue)
{
    auto if_ast = dynamic_cast<ast_node_if*>(node);

    /* Create the if-block */
    gcc_jit_block_add_comment(*current_block, 0, "IF");
    gcc_jit_block *if_block = gcc_jit_function_new_block(*current_function, new_unique_name("if_block").c_str());
    /* Walk the tree and append any blocks to the if block or any statements to the block in question */
    gcc_jit_rvalue *if_rv = nullptr;
    gcc_jit_block *last_if_block = if_block;
    v_block_terminated.push_back(false);
    walk_tree(if_ast->if_el, &last_if_block, current_function, &if_rv);
    bool if_was_terminated = v_block_terminated.back();
    v_block_terminated.pop_back();
    /* Create the else block */
    gcc_jit_block *else_block = gcc_jit_function_new_block(*current_function, new_unique_name("else_block").c_str());
    /* Walk the tree and append any blocks to the if block or any statements to the block in question */
    gcc_jit_rvalue *else_rv = nullptr;
    gcc_jit_block *last_else_block = else_block;

    v_block_terminated.push_back(false);
    if (if_ast->else_el) {
        walk_tree(if_ast->else_el, &last_else_block, current_function, &else_rv);
    }
    bool else_was_terminated = v_block_terminated.back();
    v_block_terminated.pop_back();

    /* With the if and else block created we can end the current block with a conditional jump to either of these. */
    gcc_jit_rvalue *cond_rv = nullptr;
    walk_tree(if_ast->cond_e, current_block, current_function, &cond_rv);
    DEBUG_ASSERT(cond_rv != nullptr, "If condition rvalue is null");

    gcc_jit_rvalue *bool_cond_rv = gcc_jit_context_new_cast(
            context, 0,
            gcc_jit_context_new_cast(context, 0, cond_rv, INT_TYPE),
            BOOL_TYPE);

    gcc_jit_block_end_with_conditional(*current_block, 0, bool_cond_rv, if_block, else_block);

    /* The last block in the if and else part need to jump to the after_block,
        * unless both the if and else block are terminated. */
    if (!if_was_terminated || !else_was_terminated) {
        gcc_jit_block *after_block = gcc_jit_function_new_block(*current_function, new_unique_name("after_block").c_str());
        if (!if_was_terminated)
            gcc_jit_block_end_with_jump(last_if_block, 0, after_block);
        if (!else_was_terminated)
            gcc_jit_block_end_with_jump(last_else_block, 0, after_block);
        /* Repoint the head block. */
        *current_block = after_block;
    } else {
        /* Both the if and else was terminated so there can be no after-block. I.e.
            * both returned. */
        v_block_terminated.back() = true;
    }    
}

void jit::walk_tree_while(ast_node *node, 
                        gcc_jit_block **current_block, 
                        gcc_jit_function **current_function, 
                        gcc_jit_rvalue **current_rvalue)
{
    /*
     * For WHILE cond DO ... ELSE DO ... END
     * 
     *  first_cond_block:
     *      if (cond) goto while_block;
     *      else goto else_block;
     *  cond_block:
     *      if (cond) goto while_block;
     *      else goto after_block;
     *  while_block:
     *      ...
     *      goto cond_block; (unless returns for all paths)
     *  else_block:
     *      ...
     *      goto after_block; (unless returns for all paths)
     *  after_block: (unless both while and else returns for all paths in them)
     *      ...
     * 
     * For WHILE cond DO .... END
     *  
     *  cond_block:
     *      if (cond) goto while_block;
     *      else goto else_block;
     *  while_block:
     *      ...
     *      goto cond_block (unless returns)
     *  else_block:
     *      ...
     */


    auto while_ast = dynamic_cast<ast_node_while*>(node);
    DEBUG_ASSERT_NOTNULL(while_ast);
    /* Create the while-block */
    gcc_jit_block_add_comment(*current_block, 0, "WHILE");
    /* WHILE needs a condition block to be able to jump back to. */
    gcc_jit_block *first_cond_block = nullptr;
    if (while_ast->else_el) /* If there is an else we need a first condition block tested only once */
        first_cond_block = gcc_jit_function_new_block(*current_function, new_unique_name("first_while_cond_block").c_str());
    gcc_jit_block *cond_block = gcc_jit_function_new_block(*current_function, new_unique_name("while_cond_block").c_str());

    if (while_ast->else_el) {
        gcc_jit_block_end_with_jump(*current_block, 0, first_cond_block);
    } else 
        gcc_jit_block_end_with_jump(*current_block, 0, cond_block);

    gcc_jit_block *while_block = gcc_jit_function_new_block(*current_function, new_unique_name("while_block").c_str());
    /* Walk the tree and append any blocks to the if block or any statements to the block in question */
    gcc_jit_rvalue *while_rv = nullptr;
    gcc_jit_block *last_while_block = while_block;
    v_block_terminated.push_back(false);
    walk_tree(while_ast->if_el, &last_while_block, current_function, &while_rv);
    bool while_was_terminated = v_block_terminated.back();
    v_block_terminated.pop_back();   
    /* Create the else block */
    gcc_jit_block *else_block = gcc_jit_function_new_block(*current_function, new_unique_name("else_block").c_str());
    /* Walk the tree and append any blocks to the if block or any statements to the block in question */
    gcc_jit_rvalue *else_rv = nullptr;
    gcc_jit_block *last_else_block = else_block;

    v_block_terminated.push_back(false);
    if (while_ast->else_el) {
        walk_tree(while_ast->else_el, &last_else_block, current_function, &else_rv);
    }
    bool else_was_terminated = v_block_terminated.back();
    v_block_terminated.pop_back();

    /* With the if and else block created we can end the current block with a conditional jump to either of these. */
    gcc_jit_rvalue *cond_rv = nullptr;
    walk_tree(while_ast->cond_e, current_block, current_function, &cond_rv);
    DEBUG_ASSERT(cond_rv != nullptr, "If condition rvalue is null");

    gcc_jit_rvalue *bool_cond_rv = gcc_jit_context_new_cast(
            context, 0,
            gcc_jit_context_new_cast(context, 0, cond_rv, INT_TYPE),
            BOOL_TYPE);

    if (while_ast->else_el) {
        gcc_jit_block *after_block = nullptr;
        if (!while_was_terminated || !else_was_terminated) /* Unless the quite silly while block where all paths return */
            after_block = gcc_jit_function_new_block(*current_function, new_unique_name("after_block").c_str());
        gcc_jit_block_end_with_conditional(first_cond_block, 0, bool_cond_rv, while_block, else_block);

        if (after_block)
            gcc_jit_block_end_with_conditional(cond_block, 0, bool_cond_rv, while_block, after_block);
        else
            gcc_jit_block_end_with_jump(cond_block, 0, while_block); /* All paths in the while block return. */ 

        if (!else_was_terminated)
            gcc_jit_block_end_with_jump(else_block, 0, after_block);
        
        if (after_block)            
            *current_block = after_block;
        else
            v_block_terminated.back() = true;    
    } else {
        gcc_jit_block_end_with_conditional(cond_block, 0, bool_cond_rv, while_block, else_block);
        *current_block = else_block;
    }

    /* Unless all paths in the while block are terminted it need to end in an jump back to the cond block. */
    if (!while_was_terminated)
        gcc_jit_block_end_with_jump(while_block, 0, cond_block);    
}

void jit::walk_tree(ast_node *node, 
        gcc_jit_block **current_block,
        gcc_jit_function **current_function, 
        gcc_jit_rvalue **current_rvalue, 
        gcc_jit_lvalue **current_lvalue)
{
    ast_type type = node->type;
    if (type == ast_type::ADD) {
        walk_tree_add(node, current_block, current_function, current_rvalue);
    } else if (type == ast_type::SUB) {
        walk_tree_sub(node, current_block, current_function, current_rvalue);
    } else if (type == ast_type::MUL) {
        walk_tree_mul(node, current_block, current_function, current_rvalue);
    } else if (type == ast_type::RDIV) {
        walk_tree_rdiv(node, current_block, current_function, current_rvalue);
    } else if (type == ast_type::ANDCHAIN) {
        walk_tree_andchain(node, current_block, current_function, current_rvalue);
    } else if (type == ast_type::GEQ) {
        walk_tree_geq(node, current_block, current_function, current_rvalue);
    } else if (type == ast_type::LEQ) {
        walk_tree_leq(node, current_block, current_function, current_rvalue);
    } else if (type == ast_type::EQU) {
        walk_tree_equ(node, current_block, current_function, current_rvalue);
    } else if (type == ast_type::LES) {
        walk_tree_les(node, current_block, current_function, current_rvalue);
    } else if (type == ast_type::GRE) {
        walk_tree_gre(node, current_block, current_function, current_rvalue);
    } else if (type == ast_type::NEQ) {
        walk_tree_neq(node, current_block, current_function, current_rvalue);
    } else if (type == ast_type::UMINUS) {
        walk_tree_uminus(node, current_block, current_function, current_rvalue);
    } else if (type == ast_type::DOUBLE_LITERAL) {
        walk_tree_dlit(node, current_block, current_function, current_rvalue);
    } else if (type == ast_type::INT_LITERAL) {
        walk_tree_ilit(node, current_block, current_function, current_rvalue);
    } else if (type == ast_type::FUNCTION_CALL) {
        walk_tree_fcall(node, current_block, current_function, current_rvalue);
    } else if (type == ast_type::FUNCTION_DECLARATION) {
        walk_tree_fdec(node, current_block, current_function, current_rvalue);
    } else if (type == ast_type::RETURN) {
        walk_tree_ret(node, current_block, current_function, current_rvalue);
    } else if (type == ast_type::ASSIGN) {
        walk_tree_assign(node, current_block, current_function, current_rvalue);
    } else if (type == ast_type::VAR) {
        walk_tree_var(node, current_block, current_function, current_rvalue);
    } else if(type == ast_type::DEF) {
        walk_tree_def(node, current_block, current_function, current_rvalue);
    } else if (type == ast_type::EXPLIST) {
        walk_tree_explist(node, current_block, current_function, current_rvalue);
    } else if (type == ast_type::DOBLOCK) {
        walk_tree_doblock(node, current_block, current_function, current_rvalue);
    } else if (type == ast_type::IF) {
        walk_tree_if(node, current_block, current_function, current_rvalue);
    } else if (type == ast_type::WHILE) {
        walk_tree_while(node, current_block, current_function, current_rvalue);
    } else if (type == ast_type::FUNCTION_DEFINITION) {
        walk_tree_fdef(node, current_block, current_function, current_rvalue);
    } else if (type == ast_type::AND) {
        walk_tree_and(node, current_block, current_function, current_rvalue);
    } else if (type == ast_type::OR) {
        walk_tree_or(node, current_block, current_function, current_rvalue);
    } else if (type == ast_type::NAND) {
        walk_tree_nand(node, current_block, current_function, current_rvalue);
    } else if (type == ast_type::XOR) {
        walk_tree_xor(node, current_block, current_function, current_rvalue);
    } else if (type == ast_type::NOR) {
        walk_tree_nor(node, current_block, current_function, current_rvalue);
    } else if (type == ast_type::XNOR) {
        walk_tree_xnor(node, current_block, current_function, current_rvalue);
    } else if (type == ast_type::NOT) {
        walk_tree_not(node, current_block, current_function, current_rvalue);
    } else
        throw std::runtime_error("walk_tree not implemented: " + std::to_string((int)node->type));
}
