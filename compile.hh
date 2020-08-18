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

    default_types(gcc_jit_context *context)
    {
        int_type = gcc_jit_context_get_type (context, GCC_JIT_TYPE_INT);
        double_type = gcc_jit_context_get_type (context, GCC_JIT_TYPE_DOUBLE);
        void_type = gcc_jit_context_get_type (context, GCC_JIT_TYPE_VOID);
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

    void* get_function(std::string name);
    void* get_var(std::string name);
    /* Add a ast node to the root block */
    void add_ast_node(ast_node *node);
    void init_as_root_context();
    void compile();
    void execute();
private:
    gcc_jit_result *result = nullptr;
    gcc_jit_context *context = nullptr;
    /* The root block is were root level expressions are evaluated. */
    gcc_jit_block *root_block = nullptr;
    gcc_jit_function *root_func = nullptr;

    void walk_tree(ast_node *node,
            gcc_jit_block ** current_block,
            gcc_jit_function **current_function,
            gcc_jit_rvalue **current_rvalue);
    void setup_default_root_environment();

    default_types *types;

    std::map<std::string, gcc_jit_function*> map_fnname_to_gccfnobj;
};

