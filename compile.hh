#pragma once

#include <string>
#include <libgccjit.h>
#include <vector>
#include <map>

#include "emc.hh"

struct default_types {
    gcc_jit_type *void_type;
    gcc_jit_type *int_type;
    gcc_jit_type *double_type;
    gcc_jit_type *bool_type;

    default_types(gcc_jit_context *context)
    {
        int_type = gcc_jit_context_get_type (context, GCC_JIT_TYPE_INT);
        double_type = gcc_jit_context_get_type (context, GCC_JIT_TYPE_DOUBLE);
        void_type = gcc_jit_context_get_type (context, GCC_JIT_TYPE_VOID);
        bool_type = gcc_jit_context_get_type (context, GCC_JIT_TYPE_BOOL);
    }
};

class jit {
public:
    ~jit()
    {
        gcc_jit_result_release(result);
        gcc_jit_context_release(context);
        delete types;
    }

    jit()
    {
        /* Push the global scope of lvals */
        push_scope();
    }

    void* get_function(std::string name);
    void* get_var(std::string name);
    /* Add a ast node to the root block */
    void add_ast_node(ast_node *node);
    void init_as_root_context();
    void compile();
    void execute();
    void dump(std::string path);
    gcc_jit_type *emc_type_to_jit_type(emc_type t);
private:
    gcc_jit_result *result = nullptr;
    gcc_jit_context *context = nullptr;
    /* The root block is were root level expressions are evaluated. */

    gcc_jit_block *root_block = nullptr;
    gcc_jit_function *root_func = nullptr; /* Call to setup globals etc. */

    void setup_default_root_environment();

    default_types *types;

    std::map<std::string, gcc_jit_function*> map_fnname_to_gccfnobj;
    int call_depth = 0;

    /* Cast a and b according to promotion rules. The casted values can be
     * pointing to the original rvalues if no cast was done. 
     * 
     * The return value is of the type promoted to or the original type. */
    gcc_jit_type* promote_types( gcc_jit_rvalue *a_rv,
                        gcc_jit_rvalue *b_rv,
                        gcc_jit_rvalue **a_casted_rv,
                        gcc_jit_rvalue **b_casted_rv);

    /* Returns a rvalue that is a_rv with an cast to target_type applied to it.
     * The return value can point to the same rvalue as a_rv if no cast is needed. */
    gcc_jit_rvalue* cast_to(gcc_jit_rvalue *a_rv,
                        gcc_jit_type *target_type);

    void push_scope()
    {
        v_of_map_of_varname_to_lval.push_back(
                (std::map<std::string, gcc_jit_lvalue*>){});
    }

    void pop_scope()
    {
        v_of_map_of_varname_to_lval.pop_back();
    }

    std::map<std::string, gcc_jit_lvalue*> &get_current_scope()
    {
        return v_of_map_of_varname_to_lval.back();
    }

    gcc_jit_lvalue* find_lval_in_scopes(std::string name);
    void push_lval(std::string name, gcc_jit_lvalue* lval);
    int scope_n_nested() { return v_of_map_of_varname_to_lval.size(); }

    std::vector<std::map<std::string, gcc_jit_lvalue*>> v_of_map_of_varname_to_lval;

    std::vector<std::string> v_node_fn_names;

    /* Helper functions ... */
    /*
    walk_tree(ast_node *node, 
        gcc_jit_block **current_block,
        gcc_jit_function **current_function, 
        gcc_jit_rvalue **current_rvalue, 
        gcc_jit_lvalue **current_lvalue)
    */
    /* walk_tree(node, current_block, current_function, current_rvalue); */
    void walk_tree(ast_node *node, 
        gcc_jit_block **current_block,
        gcc_jit_function **current_function, 
        gcc_jit_rvalue **current_rvalue, 
        gcc_jit_lvalue **current_lvalue = 0);
    /* TODO: Lägg parametrarna i ett objekt istället ... */ 
    void walk_tree_and(ast_node *node, gcc_jit_block **current_block, gcc_jit_function **current_function, gcc_jit_rvalue **current_rvalue);
    void walk_tree_or(ast_node *node, gcc_jit_block **current_block, gcc_jit_function **current_function, gcc_jit_rvalue **current_rvalue);
    void walk_tree_nand(ast_node *node, gcc_jit_block **current_block, gcc_jit_function **current_function, gcc_jit_rvalue **current_rvalue);
    void walk_tree_nor(ast_node *node, gcc_jit_block **current_block, gcc_jit_function **current_function, gcc_jit_rvalue **current_rvalue);
    void walk_tree_xor(ast_node *node, gcc_jit_block **current_block, gcc_jit_function **current_function, gcc_jit_rvalue **current_rvalue);
    void walk_tree_xnor(ast_node *node, gcc_jit_block **current_block, gcc_jit_function **current_function, gcc_jit_rvalue **current_rvalue);
 
    void walk_tree_add(ast_node *node, gcc_jit_block **current_block, gcc_jit_function **current_function, gcc_jit_rvalue **current_rvalue);
    void walk_tree_sub(ast_node *node, gcc_jit_block **current_block, gcc_jit_function **current_function, gcc_jit_rvalue **current_rvalue);
    void walk_tree_mul(ast_node *node, gcc_jit_block **current_block, gcc_jit_function **current_function, gcc_jit_rvalue **current_rvalue);
    void walk_tree_rdiv(ast_node *node, gcc_jit_block **current_block, gcc_jit_function **current_function, gcc_jit_rvalue **current_rvalue);
    void walk_tree_andchain(ast_node *node, gcc_jit_block **current_block, gcc_jit_function **current_function, gcc_jit_rvalue **current_rvalue);
    void walk_tree_geq(ast_node *node, gcc_jit_block **current_block, gcc_jit_function **current_function, gcc_jit_rvalue **current_rvalue);
    void walk_tree_leq(ast_node *node, gcc_jit_block **current_block, gcc_jit_function **current_function, gcc_jit_rvalue **current_rvalue);
    void walk_tree_equ(ast_node *node, gcc_jit_block **current_block, gcc_jit_function **current_function, gcc_jit_rvalue **current_rvalue);
    void walk_tree_les(ast_node *node, gcc_jit_block **current_block, gcc_jit_function **current_function, gcc_jit_rvalue **current_rvalue);
    void walk_tree_gre(ast_node *node, gcc_jit_block **current_block, gcc_jit_function **current_function, gcc_jit_rvalue **current_rvalue);
    void walk_tree_neq(ast_node *node, gcc_jit_block **current_block, gcc_jit_function **current_function, gcc_jit_rvalue **current_rvalue);
    void walk_tree_uminus(ast_node *node, gcc_jit_block **current_block, gcc_jit_function **current_function, gcc_jit_rvalue **current_rvalue);

    void walk_tree_dlit(ast_node *node, gcc_jit_block **current_block, gcc_jit_function **current_function, gcc_jit_rvalue **current_rvalue);
    void walk_tree_ilit(ast_node *node, gcc_jit_block **current_block, gcc_jit_function **current_function, gcc_jit_rvalue **current_rvalue);

    void walk_tree_fcall(ast_node *node, gcc_jit_block **current_block, gcc_jit_function **current_function, gcc_jit_rvalue **current_rvalue);
    void walk_tree_fdef(ast_node *node, gcc_jit_block **current_block, gcc_jit_function **current_function, gcc_jit_rvalue **current_rvalue);
    void walk_tree_fdec(ast_node *node, gcc_jit_block **current_block, gcc_jit_function **current_function, gcc_jit_rvalue **current_rvalue);
    void walk_tree_ret(ast_node *node, gcc_jit_block **current_block, gcc_jit_function **current_function, gcc_jit_rvalue **current_rvalue);
    void walk_tree_assign(ast_node *node, gcc_jit_block **current_block, gcc_jit_function **current_function, gcc_jit_rvalue **current_rvalue);
    void walk_tree_var(ast_node *node, gcc_jit_block **current_block, gcc_jit_function **current_function, gcc_jit_rvalue **current_rvalue);
    void walk_tree_def(ast_node *node, gcc_jit_block **current_block, gcc_jit_function **current_function, gcc_jit_rvalue **current_rvalue);
    void walk_tree_explist(ast_node *node, gcc_jit_block **current_block, gcc_jit_function **current_function, gcc_jit_rvalue **current_rvalue);
    void walk_tree_doblock(ast_node *node, gcc_jit_block **current_block, gcc_jit_function **current_function, gcc_jit_rvalue **current_rvalue);
    void walk_tree_if(ast_node *node, gcc_jit_block **current_block, gcc_jit_function **current_function, gcc_jit_rvalue **current_rvalue);
    void walk_tree_while(ast_node *node, gcc_jit_block **current_block, gcc_jit_function **current_function, gcc_jit_rvalue **current_rvalue);
};

