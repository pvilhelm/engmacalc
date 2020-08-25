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

std::vector<gcc_jit_type*> v_return_type; /* Stack to keep track of return type of function defs. */
std::vector<bool> v_block_terminated;     /* To keep track off if the block was terminated depper in the tree. */

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
    if (node->type == ast_type::DEF || node->type == ast_type::FUNCTION_DECLARATION) {
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
            if (t == INT_TYPE) {
                rv_call = gcc_jit_context_new_call(context, 0, map_fnname_to_gccfnobj["printnl_int"], 1, &rval);
            } else if (t == DOUBLE_TYPE)
                rv_call = gcc_jit_context_new_call(context, 0, map_fnname_to_gccfnobj["printnl_double"], 1, &rval);
            /* Else ignore. */
            if (rv_call)
                gcc_jit_block_add_eval(node_block, 0, rv_call);
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



void jit::walk_tree(ast_node *node, 
        gcc_jit_block **current_block,
        gcc_jit_function **current_function, 
        gcc_jit_rvalue **current_rvalue, 
        gcc_jit_lvalue **current_lvalue)
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
        }
    else if (type == ast_type::ANDCHAIN) {
        auto t_node = dynamic_cast<ast_node_andchain*>(node);
        /* TODO: Fix chaining somewhere here */
        /* Resolve types. */

        /* If there are only one child (two comparations) there is no need for locals
            * to hold values. */
        if (t_node->v_children.size() == 1) {
            gcc_jit_rvalue *rv = nullptr;
            walk_tree(t_node->v_children[0], 0, 0, &rv);
            *current_rvalue = rv;
            return;
        } else
            throw std::runtime_error("Not implemented ANDCHAIN");

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
    } else if (type == ast_type::LEQ) {
        auto t_node = dynamic_cast<ast_node_leq*>(node);
        /* TODO: Fix chaining somewhere here */
        /* Resolve types. */

        gcc_jit_rvalue *a_rv = nullptr;
        walk_tree(t_node->first.get(), 0, 0, &a_rv);
        DEBUG_ASSERT(a_rv != nullptr, "LEQ a is null");
        gcc_jit_rvalue *b_rv = nullptr;
        walk_tree(t_node->sec.get(), 0, 0, &b_rv);
        DEBUG_ASSERT(b_rv != nullptr, "LEQ b is null");

        gcc_jit_rvalue *rv_result = gcc_jit_context_new_comparison(
                                        context, nullptr, GCC_JIT_COMPARISON_LE,
                                        a_rv, b_rv);
        *current_rvalue = rv_result;
        return;
    } else if (type == ast_type::EQU) {
        auto t_node = dynamic_cast<ast_node_equ*>(node);
        /* TODO: Fix chaining somewhere here */
        /* Resolve types. */

        gcc_jit_rvalue *a_rv = nullptr;
        walk_tree(t_node->first.get(), 0, 0, &a_rv);
        DEBUG_ASSERT(a_rv != nullptr, "EQU a is null");
        gcc_jit_rvalue *b_rv = nullptr;
        walk_tree(t_node->sec.get(), 0, 0, &b_rv);
        DEBUG_ASSERT(b_rv != nullptr, "EQU b is null");

        gcc_jit_rvalue *rv_result = gcc_jit_context_new_comparison(
                                        context, nullptr, GCC_JIT_COMPARISON_EQ,
                                        a_rv, b_rv);
        *current_rvalue = rv_result;
        return;
    } else if (type == ast_type::LES) {
        auto t_node = dynamic_cast<ast_node_les*>(node);
        /* TODO: Fix chaining somewhere here */
        /* Resolve types. */

        gcc_jit_rvalue *a_rv = nullptr;
        walk_tree(t_node->first.get(), 0, 0, &a_rv);
        DEBUG_ASSERT(a_rv != nullptr, "GEQ a is null");
        gcc_jit_rvalue *b_rv = nullptr;
        walk_tree(t_node->sec.get(), 0, 0, &b_rv);
        DEBUG_ASSERT(b_rv != nullptr, "GEQ b is null");

        gcc_jit_rvalue *rv_result = gcc_jit_context_new_comparison(
                                        context, nullptr, GCC_JIT_COMPARISON_LT,
                                        a_rv, b_rv);
        *current_rvalue = rv_result;
        return;
    } else if (type == ast_type::GRE) {
        auto t_node = dynamic_cast<ast_node_gre*>(node);
        /* TODO: Fix chaining somewhere here */
        /* Resolve types. */

        gcc_jit_rvalue *a_rv = nullptr;
        walk_tree(t_node->first.get(), 0, 0, &a_rv);
        DEBUG_ASSERT(a_rv != nullptr, "GRE a is null");
        gcc_jit_rvalue *b_rv = nullptr;
        walk_tree(t_node->sec.get(), 0, 0, &b_rv);
        DEBUG_ASSERT(b_rv != nullptr, "GRE b is null");

        gcc_jit_rvalue *rv_result = gcc_jit_context_new_comparison(
                                        context, nullptr, GCC_JIT_COMPARISON_GT,
                                        a_rv, b_rv);
        *current_rvalue = rv_result;
        return;
    } else if (type == ast_type::NEQ) {
        auto t_node = dynamic_cast<ast_node_neq*>(node);
        /* TODO: Fix chaining somewhere here */
        /* Resolve types. */

        gcc_jit_rvalue *a_rv = nullptr;
        walk_tree(t_node->first.get(), 0, 0, &a_rv);
        DEBUG_ASSERT(a_rv != nullptr, "NEQ a is null");
        gcc_jit_rvalue *b_rv = nullptr;
        walk_tree(t_node->sec.get(), 0, 0, &b_rv);
        DEBUG_ASSERT(b_rv != nullptr, "NEQ b is null");

        gcc_jit_rvalue *rv_result = gcc_jit_context_new_comparison(
                                        context, nullptr, GCC_JIT_COMPARISON_NE,
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
    } else if (type == ast_type::FUNCTION_CALL) {
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
        auto *par_list = dynamic_cast<ast_node_parlist*>(fnobj->para_list);
        DEBUG_ASSERT(par_list != nullptr, "par_list is null");
        auto arg_t = dynamic_cast<ast_node_arglist*>(fcall_node->arg_list);
        DEBUG_ASSERT(arg_t != nullptr, "arg_t is null");

        if (par_list->v_func_paras.size() != arg_t->v_ast_args.size())
            throw std::runtime_error("Function " + fcall_node->name + " incorrect number of arguments.");

        std::vector<gcc_jit_rvalue*> v_arg_rv;
        for (int i = 0; i < arg_t->v_ast_args.size(); i++) {
            ast_node *arg_node = arg_t->v_ast_args[i];

            gcc_jit_rvalue *arg_rv = nullptr;

            walk_tree(arg_node, 0, 0, &arg_rv);
            DEBUG_ASSERT(arg_node, "Could not produce rval from argument");
            v_arg_rv.push_back(arg_rv);
        }

        gcc_jit_rvalue *fncall_rval = gcc_jit_context_new_call(context, 0, func, v_arg_rv.size(), v_arg_rv.data());

        *current_rvalue = fncall_rval;
        return;

    } else if (type == ast_type::FUNCTION_DECLARATION) {
        DEBUG_ASSERT(node != nullptr, "node was null");
        auto ast_funcdec = dynamic_cast<ast_node_funcdec*>(node);
        DEBUG_ASSERT(ast_funcdec != nullptr, "ast_funcdec was null");
        auto ast_parlist = dynamic_cast<ast_node_parlist*>(ast_funcdec->parlist);

        gcc_jit_type *return_type = emc_type_to_jit_type(ast_funcdec->vardef_list->resolve());
        v_return_type.push_back(return_type); /* Filescope vector used at return statements ... */
        /* TODO: Add suport for other types than double ... */
        push_scope();
        std::vector<gcc_jit_param*> v_params;
        for (func_para &e : ast_parlist->v_func_paras) {
            gcc_jit_param *para = gcc_jit_context_new_param(context, 0, DOUBLE_TYPE, e.name.c_str());
            v_params.push_back(para);
            /* Add the parameters to the scope as lvalues. */
            push_lval(e.name, gcc_jit_param_as_lvalue(para));
        }

        gcc_jit_function *fn = gcc_jit_context_new_function(context, 0, GCC_JIT_FUNCTION_INTERNAL,
                                        return_type, ast_funcdec->name.c_str(),
                                        v_params.size(), v_params.data(), 0);
        int block_depth = v_block_terminated.size();
        v_block_terminated.push_back(false);
        gcc_jit_block *fn_block = gcc_jit_function_new_block(fn, new_unique_name("fn_start").c_str());

        /* Add a new scope and push it with the parameter names. */
        extern scope_stack resolve_scope; /* TODO: Borde inte behöva detta i compile.cc ... refactor */
        resolve_scope.push_new_scope();
        auto &top_scope = resolve_scope.get_top_scope();
        for (func_para &par : ast_parlist->v_func_paras) {
            object_double *od = new object_double{par.name, 0};
            top_scope.push_object(od);
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

        return;
    } else if (type == ast_type::RETURN) {
        DEBUG_ASSERT(node != nullptr, "node was null");
        auto ret_node = dynamic_cast<ast_node_return*>(node);
        DEBUG_ASSERT(ret_node != nullptr, "ret_node was null");

        gcc_jit_rvalue *rval = nullptr;
        walk_tree(ret_node->first, 0, 0, &rval);
        DEBUG_ASSERT(rval != nullptr, "Invalid rval for return");

        gcc_jit_rvalue *casted_rval = nullptr;

        gcc_jit_type *rv_type = gcc_jit_rvalue_get_type(rval);
        emc_type emc_type = ret_node->resolve();

        DEBUG_ASSERT(v_return_type.size() >= 1, "Return type stack 0 sized");
        gcc_jit_type *return_type = v_return_type.back();

        if (return_type == rv_type)
            casted_rval = rval;
        else {
            casted_rval = gcc_jit_context_new_cast(
                                context, 0, rval, return_type);
        }
        v_block_terminated.back() = true;
        gcc_jit_block_end_with_return(*current_block, 0, casted_rval);
        /* TODO: Ensure there are no more returns in the block ... */

        return;
    } else if (type == ast_type::TEMP) {
        DEBUG_ASSERT_NOTNULL(current_rvalue);
        DEBUG_ASSERT_NOTNULL(node);
        auto tmp_node = dynamic_cast<ast_node_temporary*>(node);
        DEBUG_ASSERT_NOTNULL(tmp_node);

        if (current_lvalue == nullptr) {
            /* The caller is not anticipating a lval so no need for temps. */
            gcc_jit_rvalue *rval = nullptr;
            walk_tree(tmp_node->first, 0, 0, &rval);
            DEBUG_ASSERT_NOTNULL(rval);
            *current_rvalue = rval;
        } else {
            DEBUG_ASSERT_NOTNULL(current_block);
            DEBUG_ASSERT_NOTNULL(current_lvalue);
            /* Create an local to store the rval in so chaining is possible */
            gcc_jit_lvalue *lval = gcc_jit_function_new_local(*current_function, 0, BOOL_TYPE, 
                            new_unique_name("cmp_tmp").c_str());
            /* The rval to store in the temporary. */
            gcc_jit_rvalue *rval = nullptr;
            walk_tree(tmp_node->first, 0, 0, &rval);
            DEBUG_ASSERT_NOTNULL(rval);
            /* Assign the rval to the lval and ... */
            gcc_jit_block_add_assignment(*current_block, 0, lval, rval);
            /* ... return the lval as an rval */
            *current_rvalue = gcc_jit_lvalue_as_rvalue(lval);
            *current_lvalue = lval;
        }
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
        auto gcc_lval = find_lval_in_scopes(lval->name);
        if (gcc_lval != nullptr) {
            /* Add the assignment */
            gcc_jit_block_add_assignment(*current_block, 0, gcc_lval, rval);
        } else
            std::runtime_error("Could not resolve lval of object: " + lval->name);

        return;
    } else if (type == ast_type::VAR) {
        DEBUG_ASSERT(node != nullptr, "node was null");
        auto var_node = dynamic_cast<ast_node_var*>(node);
        DEBUG_ASSERT(var_node != nullptr, "ass_node was null");

        gcc_jit_lvalue *lval = find_lval_in_scopes(var_node->name);
        if (!lval)
            throw std::runtime_error("Could not resolve lval in the current scope: " + var_node->name);

        *current_rvalue = gcc_jit_lvalue_as_rvalue(lval);

        return;
    } else if(type == ast_type::DEF) {
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
            walk_tree(ast_def->value_node, 0, 0, &rv_assignment);
            /* Cast to the global's type */
            gcc_jit_rvalue *cast_rv = gcc_jit_context_new_cast(context, 0, rv_assignment, var_type);
            /* Assign the value in the root_block (i.e. not really like a filescope var in c */
            gcc_jit_block_add_assignment(root_block, 0, lval, cast_rv);
        } else { /* Local */
            gcc_jit_type *var_type = emc_type_to_jit_type(ast_def->value_type);
            gcc_jit_lvalue *lval = gcc_jit_function_new_local(*current_function, 0, var_type, ast_def->var_name.c_str());
            push_lval(ast_def->var_name, lval);

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
        walk_tree(if_ast->cond_e, 0, 0, &cond_rv);
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
        return;
    } else
        throw std::runtime_error("walk_tree not implemented: " + std::to_string((int)node->type));
}
