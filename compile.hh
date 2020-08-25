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

gcc_jit_type * emc_type_to_gccjit_type(const emc_type &type);

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

    void walk_tree(ast_node *node, 
        gcc_jit_block **current_block,
        gcc_jit_function **current_function, 
        gcc_jit_rvalue **current_rvalue, 
        gcc_jit_lvalue **current_lvalue = nullptr);
    void setup_default_root_environment();

    default_types *types;

    std::map<std::string, gcc_jit_function*> map_fnname_to_gccfnobj;
    int call_depth = 0;

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

    gcc_jit_lvalue* find_lval_in_scopes(std::string name)
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

    void push_lval(std::string name, gcc_jit_lvalue* lval)
    {
        auto &scope = get_current_scope();
        if (scope.find(name) != scope.end())
            throw std::runtime_error("Scope already contains the lval: " + name);
        scope[name] = lval;
    }
    int scope_n_nested() { return v_of_map_of_varname_to_lval.size(); }

    std::vector<std::map<std::string, gcc_jit_lvalue*>> v_of_map_of_varname_to_lval;

    std::vector<std::string> v_node_fn_names;
};

