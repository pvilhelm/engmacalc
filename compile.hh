#pragma once

#include <string>
#include <libgccjit.h>
#include <vector>
#include <map>

#include "emc.hh"


struct default_types {
    gcc_jit_type *void_type = 0;
    gcc_jit_type *void_ptr_type = 0;
    gcc_jit_type *bool_type = 0;
    gcc_jit_type *char_type = 0;
    gcc_jit_type *char_ptr_type = 0;
    gcc_jit_type *const_char_ptr_type = 0;
    gcc_jit_type *schar_type = 0;
    gcc_jit_type *uchar_type = 0;
    gcc_jit_type *short_type = 0;
    gcc_jit_type *ushort_type = 0;
    gcc_jit_type *int_type = 0;
    gcc_jit_type *uint_type = 0;
    gcc_jit_type *long_type = 0;
    gcc_jit_type *ulong_type = 0;
    gcc_jit_type *float_type = 0;
    gcc_jit_type *double_type = 0;
    gcc_jit_type *string_type = 0;
    

    default_types(gcc_jit_context *context)
    {
        /* TODO: Make this platform independent */
        int_type = gcc_jit_context_get_type (context, GCC_JIT_TYPE_INT);
        uint_type = gcc_jit_context_get_type (context, GCC_JIT_TYPE_UNSIGNED_INT);
        long_type = gcc_jit_context_get_type (context, GCC_JIT_TYPE_LONG);
        ulong_type = gcc_jit_context_get_type (context, GCC_JIT_TYPE_UNSIGNED_LONG);
        double_type = gcc_jit_context_get_type (context, GCC_JIT_TYPE_DOUBLE);
        void_type = gcc_jit_context_get_type (context, GCC_JIT_TYPE_VOID);
        bool_type = gcc_jit_context_get_type (context, GCC_JIT_TYPE_BOOL);
        uchar_type = gcc_jit_context_get_type (context, GCC_JIT_TYPE_UNSIGNED_CHAR);
        schar_type = gcc_jit_context_get_type (context, GCC_JIT_TYPE_SIGNED_CHAR);
        short_type = gcc_jit_context_get_type (context, GCC_JIT_TYPE_SHORT);
        ushort_type = gcc_jit_context_get_type (context, GCC_JIT_TYPE_UNSIGNED_SHORT);
        float_type = gcc_jit_context_get_type (context, GCC_JIT_TYPE_FLOAT);

        const_char_ptr_type = gcc_jit_context_get_type(context, GCC_JIT_TYPE_CONST_CHAR_PTR);
        /*
        GCC_JIT_TYPE_VOID,
        GCC_JIT_TYPE_VOID_PTR,
        GCC_JIT_TYPE_BOOL,
        GCC_JIT_TYPE_CHAR,
        GCC_JIT_TYPE_SIGNED_CHAR,
        GCC_JIT_TYPE_UNSIGNED_CHAR,
        GCC_JIT_TYPE_SHORT,
        GCC_JIT_TYPE_UNSIGNED_SHORT,
        GCC_JIT_TYPE_INT,
        GCC_JIT_TYPE_UNSIGNED_INT,
        GCC_JIT_TYPE_LONG,
        GCC_JIT_TYPE_UNSIGNED_LONG,
        GCC_JIT_TYPE_LONG_LONG, 
        GCC_JIT_TYPE_UNSIGNED_LONG_LONG,
        GCC_JIT_TYPE_FLOAT,
        GCC_JIT_TYPE_DOUBLE,
        GCC_JIT_TYPE_LONG_DOUBLE,
        GCC_JIT_TYPE_CONST_CHAR_PTR,
        GCC_JIT_TYPE_SIZE_T,
        GCC_JIT_TYPE_FILE_PTR,
        GCC_JIT_TYPE_COMPLEX_FLOAT,
        GCC_JIT_TYPE_COMPLEX_DOUBLE,
        GCC_JIT_TYPE_COMPLEX_LONG_DOUBLE */
    }
};

class jit {
public:
    ~jit()
    {
        if (result)
            gcc_jit_result_release(result);
        if (context)
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
    void init_as_dummy_context();
    void postprocess();
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

    default_types *types = nullptr;
    std::map<std::string, gcc_jit_type*> map_typename_to_gcctypeobj;

    std::map<std::string, gcc_jit_function*> map_fnname_to_gccfnobj;
    int call_depth = 0;

    


    /* Cast a and b according to promotion rules. The casted values can be
     * pointing to the original rvalues if no cast was done. 
     * 
     * The return value is of the type promoted to or the original type. */
    gcc_jit_type* promote_rvals( gcc_jit_rvalue *a_rv,
                        gcc_jit_rvalue *b_rv,
                        gcc_jit_rvalue **a_casted_rv,
                        gcc_jit_rvalue **b_casted_rv);

    /* Cast a and b in such a way that they can be compared. The casted 
     * values can be pointing to the original rvalues if no cast was done. 
     * 
     * E.g. (int, long) -> (long, long) returning long
     * 
     * Note that ulong and long can't be compared
     * 
     * The return value is of the type promoted to or the original type. */
    gcc_jit_type* promote_rvals_for_compare( gcc_jit_rvalue *a_rv,
                        gcc_jit_rvalue *b_rv,
                        gcc_jit_rvalue **a_casted_rv,
                        gcc_jit_rvalue **b_casted_rv);

    /* Promote a rval to a specific type. The casted value can be
     * pointing to the original rvalue if no cast was done. 
     * 
     * Throws if the cast is not implicit.
     */
    gcc_jit_type* promote_rval( gcc_jit_type *at,
                        gcc_jit_rvalue *b_rv,
                        gcc_jit_rvalue **b_casted_rv);

    /* Returns a rvalue that is a_rv with an cast to target_type applied to it.
     * The return value can point to the same rvalue as a_rv if no cast is needed. */
    gcc_jit_rvalue* cast_to(gcc_jit_rvalue *a_rv,
                        gcc_jit_type *target_type);

    void push_scope()
    {
        compilation_units.get_current_objstack().push_new_scope();
        v_of_map_of_varname_to_lval.push_back(
                (std::map<std::string, gcc_jit_lvalue*>){});
    }

    void pop_scope()
    {
        compilation_units.get_current_objstack().pop_scope();
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

    gcc_jit_rvalue* obj_to_gcc_literal(obj* obj);
    
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
    void walk_tree_not(ast_node *node, gcc_jit_block **current_block, gcc_jit_function **current_function, gcc_jit_rvalue **current_rvalue);

    void walk_tree_add(ast_node *node, gcc_jit_block **current_block, gcc_jit_function **current_function, gcc_jit_rvalue **current_rvalue);
    void walk_tree_sub(ast_node *node, gcc_jit_block **current_block, gcc_jit_function **current_function, gcc_jit_rvalue **current_rvalue);
    void walk_tree_mul(ast_node *node, gcc_jit_block **current_block, gcc_jit_function **current_function, gcc_jit_rvalue **current_rvalue);
    void walk_tree_rem(ast_node *node, gcc_jit_block **current_block, gcc_jit_function **current_function, gcc_jit_rvalue **current_rvalue);
    void walk_tree_pow(ast_node *node, gcc_jit_block **current_block, gcc_jit_function **current_function, gcc_jit_rvalue **current_rvalue);
    void walk_tree_rdiv(ast_node *node, gcc_jit_block **current_block, gcc_jit_function **current_function, gcc_jit_rvalue **current_rvalue);
    void walk_tree_intdiv(ast_node *node, gcc_jit_block **current_block, gcc_jit_function **current_function, gcc_jit_rvalue **current_rvalue);
    void walk_tree_andchain(ast_node *node, gcc_jit_block **current_block, gcc_jit_function **current_function, gcc_jit_rvalue **current_rvalue);
    void walk_tree_geq(ast_node *node, gcc_jit_block **current_block, gcc_jit_function **current_function, gcc_jit_rvalue **current_rvalue);
    void walk_tree_leq(ast_node *node, gcc_jit_block **current_block, gcc_jit_function **current_function, gcc_jit_rvalue **current_rvalue);
    void walk_tree_equ(ast_node *node, gcc_jit_block **current_block, gcc_jit_function **current_function, gcc_jit_rvalue **current_rvalue);
    void walk_tree_les(ast_node *node, gcc_jit_block **current_block, gcc_jit_function **current_function, gcc_jit_rvalue **current_rvalue);
    void walk_tree_gre(ast_node *node, gcc_jit_block **current_block, gcc_jit_function **current_function, gcc_jit_rvalue **current_rvalue);
    void walk_tree_neq(ast_node *node, gcc_jit_block **current_block, gcc_jit_function **current_function, gcc_jit_rvalue **current_rvalue);
    void walk_tree_uminus(ast_node *node, gcc_jit_block **current_block, gcc_jit_function **current_function, gcc_jit_rvalue **current_rvalue);
    void walk_tree_abs(ast_node *node, gcc_jit_block **current_block, gcc_jit_function **current_function, gcc_jit_rvalue **current_rvalue);
    void walk_tree_dotop(ast_node *node, gcc_jit_block **current_block, gcc_jit_function **current_function, gcc_jit_rvalue **current_rvalue, gcc_jit_lvalue **current_lvalue);
    void walk_tree_deref(ast_node *node, gcc_jit_block **current_block, gcc_jit_function **current_function, gcc_jit_rvalue **current_rvalue, gcc_jit_lvalue **current_lvalue);
    void walk_tree_address(ast_node *node, gcc_jit_block **current_block, gcc_jit_function **current_function, gcc_jit_rvalue **current_rvalue);
    void walk_tree_dlit(ast_node *node, gcc_jit_block **current_block, gcc_jit_function **current_function, gcc_jit_rvalue **current_rvalue);
    void walk_tree_ilit(ast_node *node, gcc_jit_block **current_block, gcc_jit_function **current_function, gcc_jit_rvalue **current_rvalue);
    void walk_tree_slit(ast_node *node, gcc_jit_block **current_block, gcc_jit_function **current_function, gcc_jit_rvalue **current_rvalue);

    void walk_tree_using(ast_node *node, gcc_jit_block **current_block, gcc_jit_function **current_function, gcc_jit_rvalue **current_rvalue);
    void walk_tree_struct(ast_node *node, gcc_jit_block **current_block, gcc_jit_function **current_function, gcc_jit_rvalue **current_rvalue);
    void walk_tree_type(ast_node *node, gcc_jit_block **current_block, gcc_jit_function **current_function, gcc_jit_rvalue **current_rvalue);
    void walk_tree_fcall(ast_node *node, gcc_jit_block **current_block, gcc_jit_function **current_function, gcc_jit_rvalue **current_rvalue);
    void walk_tree_fdecl(ast_node *node, gcc_jit_block **current_block, gcc_jit_function **current_function, gcc_jit_rvalue **current_rvalue);
    void walk_tree_fdefi(ast_node *node, gcc_jit_block **current_block, gcc_jit_function **current_function, gcc_jit_rvalue **current_rvalue);
    void walk_tree_ret(ast_node *node, gcc_jit_block **current_block, gcc_jit_function **current_function, gcc_jit_rvalue **current_rvalue);
    void walk_tree_assign(ast_node *node, gcc_jit_block **current_block, gcc_jit_function **current_function, gcc_jit_rvalue **current_rvalue);
    void walk_tree_var(ast_node *node, gcc_jit_block **current_block, gcc_jit_function **current_function, gcc_jit_rvalue **current_rvalue, gcc_jit_lvalue **current_lvalue);
    void walk_tree_def(ast_node *node, gcc_jit_block **current_block, gcc_jit_function **current_function, gcc_jit_rvalue **current_rvalue);
    void walk_tree_explist(ast_node *node, gcc_jit_block **current_block, gcc_jit_function **current_function, gcc_jit_rvalue **current_rvalue);
    void walk_tree_doblock(ast_node *node, gcc_jit_block **current_block, gcc_jit_function **current_function, gcc_jit_rvalue **current_rvalue);
    void walk_tree_if(ast_node *node, gcc_jit_block **current_block, gcc_jit_function **current_function, gcc_jit_rvalue **current_rvalue);
    void walk_tree_while(ast_node *node, gcc_jit_block **current_block, gcc_jit_function **current_function, gcc_jit_rvalue **current_rvalue);

    void walk_tree_def_zero_structs_helper(bool is_file_scope, 
                          gcc_jit_block **current_block,
                          const std::vector<gcc_jit_field *> &gccjit_fields,
                          std::vector<emc_type> &children_types,
                          gcc_jit_lvalue *lval);
};

