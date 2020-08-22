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

void jit::dump(std::string path)
{
    gcc_jit_context_dump_to_file(context, path.c_str(), 0);
}

gcc_jit_type* jit::emc_type_to_jit_type(emc_type t)
{
    if (t.is_double())
        return DOUBLE_TYPE;
    else if (t.is_int())
        return INT_TYPE;
    else
        DEBUG_ASSERT(false, "Not implemented");
    return 0;
}
/* TODO: remove this duplicate function (se above) */
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

std::string new_unique_name(std::string prefix = "")
{
    static long i;
    std::ostringstream ss;
    ss << prefix << "_" << std::setw(7) << std::setfill('0') <<  i++;

    return ss.str();
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
}

void jit::execute()
{
    /* Begin calling the root_fn that initializes stuff like globals etc */
    typedef void (*root_fn_p)(void);
    root_fn_p root_func = (root_fn_p)gcc_jit_result_get_code(result, "root_fn");
    if (!root_func)
    {
      std::cerr <<  "NULL root function after JIT compilation" << std::endl;
    }
    root_func();

    for (auto e : v_node_fn_names) {
        root_fn_p root_func = (root_fn_p)gcc_jit_result_get_code(result, e.c_str());
        if (!root_func)
        {
          std::cerr <<  "NULL " << e << " after JIT compilation" << std::endl;
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
    std::string node_fn_name = new_unique_name("node_fn");

    gcc_jit_function *node_func = gcc_jit_context_new_function (
                                        context, NULL,
                                        GCC_JIT_FUNCTION_EXPORTED,
                                        VOID_TYPE,
                                        node_fn_name.c_str(),
                                        0, 0, 0);
    gcc_jit_block *node_block = gcc_jit_function_new_block(node_func, new_unique_name("node_block").c_str());
    walk_tree(node, &node_block, &node_func, &rval);

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
            gcc_jit_block_add_eval(node_block, 0, rv_call);
    }

    /* End the root block with a "return;" */
    gcc_jit_block_end_with_void_return(node_block, 0);

    /* TODO: Push only function declarations. Expressions should be stored as there rvalues and be globals. */
    v_node_fn_names.push_back(node_fn_name);
}



void jit::walk_tree(ast_node *node, gcc_jit_block **current_block,
        gcc_jit_function **current_function, gcc_jit_rvalue **current_rvalue)
{
    ast_type type = node->type;
    //std::cout << "Waling tree:" << (*current_rvalue == nullptr) << " type: " << (int)type << std::endl;
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
        walk_tree(t_node->first.get(), 0, 0, &a_rv);
        DEBUG_ASSERT(a_rv != nullptr, "GEQ a is null");
        gcc_jit_rvalue *b_rv = nullptr;
        walk_tree(t_node->sec.get(), 0, 0, &b_rv);
        DEBUG_ASSERT(b_rv != nullptr, "GEQ b is null");

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
        DEBUG_ASSERT(a_rv != nullptr, "sub a is null");

        gcc_jit_rvalue *rv_result = gcc_jit_context_new_unary_op(context, nullptr,
                GCC_JIT_UNARY_OP_MINUS, result_type, a_rv);
        *current_rvalue = rv_result;
        return;
    } else if (type == ast_type::DOUBLE_LITERAL) {
        /* A double literal is an rvalue. */
        auto dlit_node = dynamic_cast<ast_node_double_literal*>(node);
        *current_rvalue = gcc_jit_context_new_rvalue_from_double(context, DOUBLE_TYPE, dlit_node->d);
        DEBUG_ASSERT(*current_rvalue != nullptr, "DOUBLE_LITERAL current_rvalue is null");
        return;
    } else if (type == ast_type::FUNCTION_DECLARATION) {
        DEBUG_ASSERT(node != nullptr, "node was null");
        auto ast_funcdec = dynamic_cast<ast_node_funcdec*>(node);
        DEBUG_ASSERT(ast_funcdec != nullptr, "ast_funcdec was null");
        auto ast_parlist = dynamic_cast<ast_node_parlist*>(ast_funcdec->parlist);

        gcc_jit_type *return_type =  DOUBLE_TYPE;

        /* TODO: Add suport for other types than double ... */
        std::vector<gcc_jit_param*> v_params;
        for (func_para e : ast_parlist->v_func_paras) {
            gcc_jit_param *para = gcc_jit_context_new_param(context, 0, DOUBLE_TYPE, e.name.c_str());
            v_params.push_back(para);
        }

        gcc_jit_function *fn = gcc_jit_context_new_function(context, 0, GCC_JIT_FUNCTION_INTERNAL,
                                        return_type, ast_funcdec->name.c_str(),
                                        v_params.size(), v_params.data(), 0);
        gcc_jit_block *fn_block = gcc_jit_function_new_block(fn, new_unique_name("fn_start").c_str());
        gcc_jit_rvalue *rval = nullptr;
        call_depth++;
        walk_tree(ast_funcdec->code_block, &fn_block, &fn, &rval);
        call_depth--;
        /* TODO: Support for proper return statements ... */
        gcc_jit_block_end_with_return(fn_block, 0, rval);

        return;
    } else if (type == ast_type::ASSIGN) {
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
        auto it_local = map_varname_to_locals.find(lval->name);
        if (it_local != map_varname_to_locals.end()) {
            /* It is a local */
            /* Add the assignment */
            gcc_jit_block_add_assignment(*current_block, 0, it_local->second, rval);
        } else { /* Maybe it is a global ... */
            auto it_glb = map_globals.find(lval->name);
            if (it_glb != map_globals.end()) {
                gcc_jit_block_add_assignment(*current_block, 0, it_glb->second, rval);
            } else
                DEBUG_ASSERT(false, "Variable " << lval->name << " doesn't exist");
        }

        return;
    } else if(type == ast_type::DEF) {
        auto *ast_def = dynamic_cast<ast_node_def*>(node);
        /* Global or local? */
        if (call_depth == 0) { /* Global */
            gcc_jit_type *var_type = emc_type_to_jit_type(ast_def->value_type);
            gcc_jit_lvalue *lval = gcc_jit_context_new_global(
                    context, 0, GCC_JIT_GLOBAL_EXPORTED,
                    var_type,
                    ast_def->var_name.c_str());
            map_globals[ast_def->var_name] = lval;

            gcc_jit_rvalue *rv_assignment = nullptr;
            walk_tree(ast_def->value_node, 0, 0, &rv_assignment);
            /* Cast to the global's type */
            gcc_jit_rvalue *cast_rv = gcc_jit_context_new_cast(context, 0, rv_assignment, var_type);
            /* Assign the value in the root_block (i.e. not really like a filescope var in c */
            gcc_jit_block_add_assignment(root_block, 0, lval, cast_rv);
        } else { /* Local */
            gcc_jit_type *var_type = emc_type_to_jit_type(ast_def->value_type);
            gcc_jit_lvalue *lval = gcc_jit_function_new_local(*current_function, 0, var_type, ast_def->var_name.c_str());
            map_varname_to_locals[ast_def->var_name] = lval;

            gcc_jit_rvalue *rv_assignment = nullptr;
            walk_tree(ast_def->value_node, 0, 0, &rv_assignment);
            /* Cast to the local's type */
            gcc_jit_rvalue *cast_rv = gcc_jit_context_new_cast(context, 0, rv_assignment, var_type);
            /* Assign the value */
            gcc_jit_block_add_assignment(*current_block, 0, lval, cast_rv);
        }

        return;
    } else if (type == ast_type::EXPLIST) {
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

        return;
    } else if (type == ast_type::DOBLOCK) {
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

        return;
    } else if (type == ast_type::IF) {
        auto if_ast = dynamic_cast<ast_node_if*>(node);

        /* Create the if-block */
        gcc_jit_block_add_comment(*current_block, 0, "IF");
        gcc_jit_block *if_block = gcc_jit_function_new_block(*current_function, new_unique_name("if_block").c_str());
        /* Walk the tree and append any blocks to the if block or any statements to the block in question */
        gcc_jit_rvalue *if_rv = nullptr;
        gcc_jit_block *last_if_block = if_block;
        walk_tree(if_ast->if_el, &last_if_block, current_function, &if_rv);

        /* Create the else block */
        gcc_jit_block *else_block = gcc_jit_function_new_block(*current_function, new_unique_name("else_block").c_str());
        /* Walk the tree and append any blocks to the if block or any statements to the block in question */
        gcc_jit_rvalue *else_rv = nullptr;
        gcc_jit_block *last_else_block = else_block;

        if (if_ast->else_el) {
            walk_tree(if_ast->else_el, &last_else_block, current_function, &else_rv);
        }

        /* With the if and else block created we can end the current block with a conditional jump to either of these. */
        gcc_jit_rvalue *cond_rv = nullptr;
        walk_tree(if_ast->cond_e, 0, 0, &cond_rv);
        DEBUG_ASSERT(cond_rv != nullptr, "If condition rvalue is null");

        gcc_jit_rvalue *bool_cond_rv = gcc_jit_context_new_cast(
                context, 0,
                gcc_jit_context_new_cast(context, 0, cond_rv, INT_TYPE),
                BOOL_TYPE);

        gcc_jit_block_end_with_conditional(*current_block, 0, bool_cond_rv, if_block, else_block);

        /* The last block in the if and else part need to jump to the after_block */
        gcc_jit_block *after_block = gcc_jit_function_new_block(*current_function, new_unique_name("after_block").c_str());
        gcc_jit_block_end_with_jump(if_block, 0, after_block);
        gcc_jit_block_end_with_jump(last_else_block, 0, after_block);
        /* Repoint the head block. */
        *current_block = after_block;

        return;
    } else
        throw std::runtime_error("walk_tree not implemented: " + std::to_string((int)node->type));
}
