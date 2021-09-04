#pragma once

#include <stdexcept>
#include <string>
#include <vector>
#include <iostream>
#include <cmath>
#include <memory>
#include <cstdlib>
#include <cerrno>
#include <limits>
#include <map>
#include <cinttypes>
#include <sstream>
#include <filesystem>
#include <functional>

#include "emc_assert.hh"
#include "util_string.hh"

# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
typedef struct YYLTYPE YYLTYPE;
struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
};

#ifndef NDEBUG
extern int ast_node_count;
extern int value_expr_count;
#endif

enum class engma_run_type {
    OUTPUT_TO_OBJ_FILE,
    OUTPUT_TO_SO,
    EXECUTE,
    OUTPUT_TO_EXE,
    OUTPUT_ASSEMBLER
};

struct engma_options {
    std::string outputfile_name; 

    engma_run_type run_type = engma_run_type::OUTPUT_TO_EXE;

    std::vector<std::string> nonengma_files;

    std::vector<std::string> include_dirs;

    std::vector<std::string> l_folders;

    std::vector<std::string> L_folders;

    std::vector<std::string> files; /* All the Engma files to process */
    
    std::string optimization_level = "-O0";

    std::string debug_flag;
};

/* CLI options parsed into this struct in main() */
extern struct engma_options opts;

/* compilation_units keeps track of type and objects in scopes for each compilation unit. */
class ast_compilation_units;
class typescope_stack;
class objscope_stack;
extern ast_compilation_units compilation_units;
extern typescope_stack builtin_typestack;
extern objscope_stack builtin_objstack;

enum class ast_type {
    INVALID = 0,
    DOUBLE_LITERAL = 1,
    INT_LITERAL,
    STRING_LITERAL,
    ADD,
    SUB,
    MUL,
    RDIV,
    UMINUS,
    VAR,
    ASSIGN,
    GEQ,
    GRE,
    LEQ,
    LES,
    CMP,
    EQU,
    NEQ,
    ABS,
    TEMP,
    POW,
    EXPLIST,
    DOBLOCK,
    IF,
    PARAMETER_LIST, /* Parameters are in the function signature. */
    ARGUMENT_LIST,
    FUNCTION_DEF,
    FUNCTION_DECL,
    FUNCTION_CALL,
    ANDCHAIN, /* Dummy node for chained comparations. */
    WHILE,
    DEF,
    RETURN,
    VARDEF_LIST,
    AND,
    OR,
    XOR,
    NAND,
    NOR,
    XNOR,
    NOT,
    TYPE,
    STRUCT,
    DOTOPERATOR,
    LISTLITERAL,
    DEREF,
    ADDRESS,
    PTRDEF_LIST,
    TYPEDOTCHAIN,
    TYPEDOTNAMECHAIN,
    NAMESPACE,
    USINGCHAIN,
    USING,
    REM,
    INTDIV
};

enum class object_type {
    DOUBLE = 1,
    FUNC,
    STATIC_CFUNC,
    STRING,
    INT,
    UINT,
    BYTE,
    SBYTE,
    SHORT,
    USHORT,
    FLOAT,
    LONG,
    ULONG,
    BOOL,
    STRUCT
};

enum class emc_types {
    INVALID = 0,
    NONE, /* The node have no type. */
    POINTER,
    INT,
    UINT,
    BOOL,
    BYTE,
    SBYTE,
    DOUBLE,
    STRING,
    CONST,
    STRUCT,
    FUNCTION,
    FLOAT,
    SHORT,
    USHORT,
    LONG,
    ULONG,
    LISTLIT
};

struct emc_type {
    emc_type() {}
    /* First element is inner element. I.e.
     * c type "const int *" => POINTER, CONST, INT
     */
    emc_type(emc_types type)
    {
        this->type = type;
    }

    emc_type(emc_types type, bool is_const_expr)
        : is_const_expr(is_const_expr)
    {
        this->type = type;
    }
  
    bool is_valid() const 
    {
        return type != emc_types::INVALID;
    }

    /* TODO: Fugly. Should be direct lookup here and in jit::emc_type_to_jit_type() */
    bool is_double() const 
    {
        return type == emc_types::DOUBLE;
    }
    bool is_int() const 
    {
        return type == emc_types::INT;
    }
    bool is_short() const 
    {
        return type == emc_types::SHORT;
    }
    bool is_sbyte() const 
    {
        return type == emc_types::SBYTE;
    }
    bool is_byte() const 
    {
        return type == emc_types::BYTE;
    }
    bool is_uint() const 
    {
        return type == emc_types::UINT;
    }
    bool is_ushort() const 
    {
        return type == emc_types::USHORT;
    }
    bool is_long() const 
    {
        return type == emc_types::LONG;
    }
    bool is_ulong() const 
    {
        return type == emc_types::ULONG;
    }
    bool is_float() const 
    {
        return type == emc_types::FLOAT;
    }
    bool is_void() const 
    {
        return type == emc_types::NONE;
    }
    bool is_bool() const 
    {
        return type == emc_types::BOOL;
    }
    bool is_struct() const 
    {
        return type == emc_types::STRUCT;
    }
    bool is_primitive() const 
    {
        return is_double() || is_int() || is_short() || is_sbyte() || is_byte() ||
                is_uint() || is_ushort() || is_long() || is_ulong() || is_float() ||
                is_bool() || is_ushort();
    }
    bool is_listlit() const 
    {
        return type == emc_types::LISTLIT;
    }
    bool is_pointer() const
    {
        return n_pointer_indirections;
    }
    bool is_string() const
    {
        return type == emc_types::STRING;
    }

    /* For structs etc with children types. */
    std::vector<emc_type> children_types;
    /* Used for fields in structs etc. */
    std::string name;
    std::string mangled_name;

    emc_type find_type_of_child(std::string name) const 
    {
        for (auto child_type : children_types)
            if (child_type.name == name)
                return child_type;
        THROW_BUG("Type " + this->name + " has no child " + name);
    }
    
    /* TODO: Should be flags for const etc instead? Do a "root type" and then have
             vector only for structs etc. */
    int n_pointer_indirections = 0;
    bool is_const = false;
    bool is_const_expr = false;
    emc_types type;

    bool operator==(const emc_type &r) const
    {
        DEBUG_ASSERT(is_struct() || is_primitive() || is_string(), ""); //Other not implemented

        if (r.is_string() && is_primitive()) {
            if (
                type == emc_types::SBYTE &&
                n_pointer_indirections == 1 &&
                is_const == r.is_const && // TODO: Const support for strings
                children_types == r.children_types
            )
                return true;
            return false;
        }
        if (r.is_primitive() && is_string()) {
            if (
                r.type == emc_types::SBYTE &&
                r.n_pointer_indirections == 1 &&
                is_const == r.is_const &&
                children_types == r.children_types
            )
                return true;
            return false;
        }

        if (
                type != r.type ||
                n_pointer_indirections != r.n_pointer_indirections ||
                is_const != r.is_const ||
                is_struct() && (mangled_name != r.mangled_name) ||
                children_types != r.children_types
            )
            return false;
        return true;
    }

    bool operator!=(const emc_type &r) const
    {
        return !(*this == r);
    }
};

emc_type cast_to(const emc_type &a, const emc_types &target);
emc_type standard_type_promotion(const emc_type &a, const emc_type &b);
emc_type standard_type_promotion_or_invalid(const emc_type &a, const emc_type &b);
emc_type string_to_type(std::string);
std::string mangle_emc_var_name(std::string name, std::string nspace);
/* Forward declarations. */
class objscope;
class objscope_stack;
class ast_node;
class obj;
class object_func;

/* Functions */
void init_builtin_types();
std::string mangle_emc_fn_name(const object_func &fn_obj);
std::string demangle_emc_fn_name(std::string c_fn_name);
std::string mangle_emc_type_name(std::string full_path);
void verify_obj_fits_in_type(obj* obj, emc_type type);
obj* cast_obj_to_type_return_new_obj(obj* obj, emc_type type);

class obj {
public:
    virtual ~obj()
    {
#ifndef NDEBUG
        value_expr_count--;
#endif
    }
    obj()
    {
#ifndef NDEBUG
        value_expr_count++;
#endif
    }
    obj(const obj&) = delete;
    obj& operator=(const obj&) = delete;

    virtual emc_type resolve() {return emc_type{emc_types::INVALID};}
    virtual void debug_print()
    {
        std::cout << "Object, Namespace: " << nspace <<
                "Name: " << name << " Type: " << (int)type << std::endl;
    }
    std::string name; /* the name of the object (without namespace) */
    std::string mangled_name; /* The mangled name of the object dependent on namespace and name */
    std::string nspace; /* The namespace of the object */
    object_type type;
    int n_pointer_indirection = 0;
};

class objscope;
class objscope_stack;
class objscope_stack {
public:
    /* There is always one root scope in the scope stack. */
    objscope_stack() {push_new_scope();}
    ~objscope_stack() {clear();}

    objscope_stack(const objscope_stack&) = delete;
    objscope_stack& operator=(const objscope_stack&) = delete;

    void push_new_scope();

    void pop_scope()
    {
        DEBUG_ASSERT(vec_scope.size() >= 1, 
            "Trying to pop objscope_stack too far");
        vec_scope.pop_back();
    }

    objscope& get_top_scope()
    {
        DEBUG_ASSERT(vec_scope.size() >= 1, 
            "Trying to access scope in empty objscope_stack");
        return vec_scope.back();
    }

    bool is_in_global_scope()
    {
        return vec_scope.size() == 1;
    }

    objscope& get_global_scope()
    {
        DEBUG_ASSERT(vec_scope.size() >= 1, 
            "Trying to access scope in empty objscope_stack");
        return vec_scope.front();
    }

    obj* find_object(std::string name);
    std::vector<obj*> find_objects_by_not_mangled_name(std::string name, std::string nspace);
    std::vector<obj*> find_objects_by_not_mangled_name_helper(std::string name, std::string nspace);

    std::vector<objscope> vec_scope;

    void debug_print();

    void clear();

    std::vector<objscope_stack*> linked_objscope_stacks;
};

class objscope {
public:
    ~objscope()
    {
        clear();
    }
    objscope(){}
    objscope(objscope &&s) 
    {
        std::swap(vec_objs, s.vec_objs);
    }
    /* Since this objects keeps track of pointers there can
     * be no copy or assignment ctor.
     */ 
    objscope(const objscope&) = delete;
    objscope operator=(const objscope&) = delete;

    std::vector<obj*> vec_objs;

    void push_object(obj *obj)
    {
        std::string name;
        if (obj->mangled_name.size())
            name = obj->mangled_name;
        else
            name = obj->name;

        if (find_object(name))
            THROW_BUG("push_object: Pushing existing object:"
                    + obj->nspace + name);
        vec_objs.push_back(obj);
    }

    void pop_object()
    {
        vec_objs.pop_back();
    }

    obj* find_object(std::string name)
    {
        /* TODO: Map ist för vec? */
        for (auto p : vec_objs) {
            if (p->mangled_name.size()) {
                if (p->mangled_name == name)
                    return p;
            } else if (p->name == name)
                return p;
        }

        return nullptr;
    }

    std::vector<obj*> find_objects_by_not_mangled_name(std::string name, std::string nspace)
    { 
        std::vector<obj*> ans;
        /* TODO: Map ist för vec? */
        for (auto p : vec_objs) {
            if (p->name == name && p->nspace == nspace)
                ans.push_back(p);
        }
        return ans;
    }

    void clear()
    {
        for (auto p : vec_objs)
            delete p;
        vec_objs.clear();
    }
};

class typescope_stack {
public:
    /* There is always one root scope in the scope stack. */
    typescope_stack() {using_scopes.push_back({});}
    ~typescope_stack()
    {
        clear(); /* TODO: Uneccesarry? */
    }

    typescope_stack(const typescope_stack&) = delete;
    typescope_stack& operator=(const typescope_stack&) = delete;

    void push_new_scope(std::string scope_name)
    {
        vec_scope.push_back(scope_name);
        /* Create a cached string on the form Ns.Ns.Ns for the current scope */
        current_scope = join_scopes();
        /* using scopes */
        using_scopes.push_back({});
    }

    void pop_scope()
    {
        DEBUG_ASSERT(vec_scope.size() >= 1, 
            "Trying to pop scope_stack too far");
        vec_scope.pop_back();
        /* Create a cached string on the form Ns.Ns.Ns for the current scope */
        current_scope = join_scopes();
        
        using_scopes.pop_back();
    }

    void push_using(std::string ns)
    {
        DEBUG_ASSERT(using_scopes.size(),"");
        using_scopes.back().push_back(ns);
    }

    bool find_type_helper(std::string name, emc_type &ans)
    {
        std::string prefix;
        if (current_scope.size())
            prefix = current_scope + ".";

        std::string full_type_name = prefix + name;

        /* Search built-in types first */
        if (this != &builtin_typestack)
            if (builtin_typestack.has_type(name)) { /* TODO: Wastefull check */
                ans = builtin_typestack.find_type(name);
                return true;
            }

        /* Search in typescopes relative to the current scope */
        auto it = map_typename_to_type.find(full_type_name);
        if (it != map_typename_to_type.end()) {
            ans = it->second;
            return true;
        }

        /* Search from root scope */
        it = map_typename_to_type.find(name);
        if (it != map_typename_to_type.end()) {
            ans = it->second;
            return true;
        }
        /* Searched all the linked type stacks for the type */
        bool found = false;

        for (auto *e : linked_typescope_stacks) {
            if (e->has_type(name)) {
                if (found)
                    THROW_BUG("Collision on using type: " + name);
                ans = e->find_type(name);
                found = true;
            }
        }

        if (found)
            return true;
        return false;
    }

    emc_type find_type(std::string name);    

    bool has_type(std::string name) 
    {
        std::string prefix;
        if (current_scope.size())
            prefix = current_scope + ".";

        std::string full_type_name = prefix + name;

        /* Search in typescopes relative to the current scope first */
        auto it = map_typename_to_type.find(full_type_name);
        if (it != map_typename_to_type.end())
            return true;

        /* Search from root scope */
        it = map_typename_to_type.find(name);
        if (it != map_typename_to_type.end())
            return true;
        return false;
    }

    /* The namespace if any need to be in name.
       Eg. Foo.Bar.Structy or Long */
    void push_type(std::string name, emc_type type)
    {
        /*std::string prefix;
        if (current_scope.size())
            prefix = current_scope + ".";

        std::string full_name = prefix + name;*/

        auto it = map_typename_to_type.find(name);
        if (it != map_typename_to_type.end())
            THROW_BUG("Type " + name + " in map of types already");
        map_typename_to_type[name] = type;
    }

    /* Essentially a vector of strings for each nested namespace scope. */
    std::vector<std::string> vec_scope;
    std::string current_scope;
    std::map<std::string, emc_type> map_typename_to_type;
    std::vector<typescope_stack*> linked_typescope_stacks;
    /* vector of vector of using:s */
    std::vector<std::vector<std::string>> using_scopes;

    void clear()
    {
        vec_scope.clear();
        map_typename_to_type.clear();
        linked_typescope_stacks.clear();
    }

    /* Joins a vector of string to "A.B.C.D.", but an empty vector to "" */
    std::string join_scopes() const 
    {
        std::ostringstream os{};
        
        for (int i = 0; i < vec_scope.size(); i++) {
            std::string ns = vec_scope[i];
            os << ns;
            if (i != vec_scope.size() - 1)
                os << ".";
        }
        return os.str();
    }

    void clear_scopes()
    {
        vec_scope.clear();
        current_scope = join_scopes();
    }

    void set_scopes(std::string full_nspace) 
    {
        clear_scopes();

        vec_scope = split_string(full_nspace, ".");
        current_scope = join_scopes();
    }
};

class compilation_unit {
public:
    objscope_stack objstack;
    typescope_stack typestack;

    bool parsed_eol = false;
    ast_node *ast_root = nullptr;
    std::vector<ast_node*> v_nodes;
    /* Set to true by tree walk so that we only walk a unit once
       if included multiple times. */
    bool is_compiled = false;

    std::string file_name;

    ~compilation_unit()
    {
        clear();
    }

    void clear();    
};

class ast_compilation_units {
public:

    ~ast_compilation_units()
    {
        
    }

    void clear()
    {
        for (auto kv : map_compilation_units_by_paths)
            delete kv.second;
        map_compilation_units_by_paths.clear();
        map_compunits_by_nspaceobj.clear();
        stack.clear();
        push_compilation_unit(""); /* Root compilation unit */
    }

    ast_compilation_units()
    {
        push_compilation_unit(""); /* Root compilation unit */
    }

    /* This map maps a compilation units root namespace to the compilation unit.
     * All cu:s are saved in this map, so it is also used for deletion in the dtor. */
    std::map<std::string, compilation_unit*> map_compilation_units_by_paths;
    /* This map maps all individual namespace objects to the compilation unit they
     * belong too. */
    std::map<std::string, compilation_unit*> map_compunits_by_nspaceobj;

    /* The stack is just a helper for walking nested compilation units */
    std::vector<compilation_unit*> stack;
    compilation_unit *current_cu = nullptr;

    compilation_unit& get_current_compilation_unit()
    {
        DEBUG_ASSERT_NOTNULL(current_cu);
        return *current_cu;
    }

    objscope_stack& get_current_objstack()
    {
        return get_current_compilation_unit().objstack;
    }

    typescope_stack& get_current_typestack()
    {
        return get_current_compilation_unit().typestack;
    }

    compilation_unit* find_compilation_unit_containing(std::string nspace)
    {
        THROW_NOT_IMPLEMENTED("");
    }

    compilation_unit* find_compilation_unit(std::string nspace)
    {
        auto it  = map_compilation_units_by_paths.find(nspace);
        if (it == map_compilation_units_by_paths.end())
            return nullptr;
        return it->second;
    }
    
    void push_compilation_unit(std::string nspace)
    {
        auto it = map_compilation_units_by_paths.find(nspace);
        if (it != map_compilation_units_by_paths.end())
            THROW_BUG("Compilation unit " + nspace + " allready processed");
        
        auto cu = new compilation_unit{};
        map_compilation_units_by_paths[nspace] = cu;
        stack.push_back(cu);
        current_cu = stack.back();
    }

    void pop_compilation_unit()
    {
        if (stack.size() == 1)
            THROW_BUG("Trying to pop last compilation unit");
        stack.pop_back();
        DEBUG_ASSERT(stack.size(),"");
        current_cu = stack.back();
    }
};


#define OBJCLASS_DEF(classname, object_type, emc_ret_type, c_type)\
class classname: public obj {\
public:\
    ~classname()\
    {\
    }\
    ;\
\
    classname(std::string name, std::string nspace, c_type val, int n_pointer_indirection)\
    :\
            val(val)\
    {\
        this->n_pointer_indirection = n_pointer_indirection;\
        type = object_type;\
        this->name = name;\
        this->nspace = nspace;\
    }\
    ;\
    classname(std::string name, c_type val, int n_pointer_indirection) :\
            classname(name, "", val, n_pointer_indirection)\
    {\
    }\
    classname(c_type val) :\
            classname("", "", val, 0)\
    {\
    }\
    classname() :\
            classname("", "", (c_type){}, 0)\
    {\
    }\
\
    c_type val;\
\
    emc_type resolve()\
    {\
        emc_type t = emc_ret_type;\
        t.n_pointer_indirections = n_pointer_indirection;\
        return t;\
    }\
};\

OBJCLASS_DEF(object_string, object_type::STRING, emc_type{emc_types::STRING}, std::string)
OBJCLASS_DEF(object_double, object_type::DOUBLE, emc_type{emc_types::DOUBLE}, double)
OBJCLASS_DEF(object_float, object_type::FLOAT, emc_type{emc_types::FLOAT}, float)
OBJCLASS_DEF(object_long, object_type::LONG, emc_type{emc_types::LONG}, int64_t)
OBJCLASS_DEF(object_ulong, object_type::ULONG, emc_type{emc_types::ULONG}, uint64_t)
OBJCLASS_DEF(object_int, object_type::INT, emc_type{emc_types::INT}, int32_t)
OBJCLASS_DEF(object_uint, object_type::UINT, emc_type{emc_types::UINT}, uint32_t)
OBJCLASS_DEF(object_short, object_type::SHORT, emc_type{emc_types::SHORT}, int16_t)
OBJCLASS_DEF(object_ushort, object_type::USHORT, emc_type{emc_types::USHORT}, uint16_t)
OBJCLASS_DEF(object_byte, object_type::BYTE, emc_type{emc_types::BYTE}, uint8_t)
OBJCLASS_DEF(object_sbyte, object_type::SBYTE, emc_type{emc_types::SBYTE}, int8_t)
OBJCLASS_DEF(object_bool, object_type::BOOL, emc_type{emc_types::BOOL}, bool)

class object_struct: public obj {
public:
    ~object_struct()
    {
    }
    
    object_struct(std::string name, std::string nspace, emc_type struct_type, int n_pointer_indirection)
        : struct_type(struct_type)
    {
        this->n_pointer_indirection = n_pointer_indirection;
        type = object_type::STRUCT;
        /* TODO: Fult att föra över n_pointer_indirections här ... */
        this->struct_type.n_pointer_indirections = n_pointer_indirection;
        this->name = name;
        this->nspace = nspace;
    }
    
    object_struct() :
            object_struct("", "", emc_type{emc_types::STRUCT}, 0)
    {
    }

    emc_type struct_type;

    emc_type resolve()
    {
        return struct_type;
    }
};

class object_func_base: public obj {
public:

};

class object_func: public object_func_base {
public:
    ~object_func();
    object_func(ast_node *root, std::string name,
            std::string nspace, ast_node *para_list, ast_node *var_list)
    :
            root(root), para_list(para_list), var_list(var_list)
    {
        type = object_type::FUNC;
        this->name = name;
        this->nspace = nspace;
    }

    ast_node *root; /* TODO: remove */
    ast_node *para_list;
    ast_node *var_list;
    bool c_linkage = false;

    emc_type resolve();
};

/* Base class for a node in the abstract syntax tree. */
class ast_node {
public:

    ast_node()
    {
#ifndef NDEBUG
        extern int ast_node_count; ast_node_count++;
#endif
    }
    virtual ~ast_node()
    {
#ifndef NDEBUG
        extern int ast_node_count; ast_node_count--;
#endif
        delete value_obj;
    }
    
    YYLTYPE loc = {-1, -1, -1, -1};

    ast_node(const ast_node&) = delete;
    ast_node& operator=(const ast_node&) = delete;

    virtual ast_node* clone() {THROW_BUG("");};
    virtual emc_type resolve() = 0;
    virtual obj* resolve_value() {THROW_BUG("");}

    ast_type type = ast_type::INVALID;
    emc_type value_type = emc_type{emc_types::INVALID};
    obj *value_obj = nullptr;
};

enum class emc_operators {
    PLUS,
    MINUS,
    MULT,
    REM,
    RDIV,
    POW,
    INTDIV
};

/* Abstract class for binary operators, with a helper function. */
class ast_node_bin_op : public ast_node {
public:
    ast_node_bin_op(ast_node *first, ast_node *sec) : first(first), sec(sec)
    {}
    virtual ~ast_node_bin_op() 
    {
        delete first;
        delete sec;
    };

    ast_node *first;
    ast_node *sec;

    template <emc_operators op_type>
    void make_value_obj()
    {
        obj* obj_1 = first->resolve_value();
        obj* obj_2 = sec->resolve_value();

        obj* casted_1 = cast_obj_to_type_return_new_obj(obj_1, value_type);
        obj* casted_2 = cast_obj_to_type_return_new_obj(obj_2, value_type);
        
        if (value_type.is_long()) {
            auto *f = dynamic_cast<object_long*>(casted_1);
            auto *s = dynamic_cast<object_long*>(casted_2);

            if constexpr (op_type == emc_operators::PLUS)
                value_obj = new object_long{(int64_t)(f->val + s->val)};
            else if constexpr (op_type == emc_operators::MINUS)
                value_obj = new object_long{(int64_t)(f->val - s->val)};
            else if constexpr (op_type == emc_operators::MULT)
                value_obj = new object_long{(int64_t)(f->val * s->val)};
            else if constexpr (op_type == emc_operators::INTDIV)
                value_obj = new object_long{(int64_t)(f->val / s->val)};
            else if constexpr (op_type == emc_operators::REM)
                value_obj = new object_long{(int64_t)(f->val % s->val)};
            else if constexpr (op_type == emc_operators::POW) {
                double p = pow(f->val, s->val);
                if (p < -9007199254740992 || p > 9007199254740992)
                    THROW_NOT_IMPLEMENTED("To big answer from Long power operator");
                value_obj = new object_long{(int64_t)p};
            } else
                THROW_BUG("");
        } else if (value_type.is_int()) {
            auto *f = dynamic_cast<object_int*>(casted_1);
            auto *s = dynamic_cast<object_int*>(casted_2);
            
            if constexpr (op_type == emc_operators::PLUS)
                value_obj = new object_int{(int32_t)(f->val + s->val)};
            else if constexpr (op_type == emc_operators::MINUS)
                value_obj = new object_int{(int32_t)(f->val - s->val)};
            else if constexpr (op_type == emc_operators::MULT)
                value_obj = new object_int{(int32_t)(f->val * s->val)};
            else if constexpr (op_type == emc_operators::INTDIV)
                value_obj = new object_int{(int32_t)(f->val / s->val)};
            else if constexpr (op_type == emc_operators::REM)
                value_obj = new object_int{(int32_t)(f->val % s->val)};
            else if constexpr (op_type == emc_operators::POW) {
                double p = pow(f->val, s->val);
                check_in_range<double, int32_t>(p);
                value_obj = new object_int{(int32_t)p};
            } else
                THROW_BUG("");
        } else if (value_type.is_short()) {
            auto *f = dynamic_cast<object_short*>(casted_1);
            auto *s = dynamic_cast<object_short*>(casted_2);

            if constexpr (op_type == emc_operators::PLUS)
                value_obj = new object_short{(int16_t)(f->val + s->val)};
            else if constexpr (op_type == emc_operators::MINUS)
                value_obj = new object_short{(int16_t)(f->val - s->val)};
            else if constexpr (op_type == emc_operators::MULT)
                value_obj = new object_short{(int16_t)(f->val * s->val)};
            else if constexpr (op_type == emc_operators::INTDIV)
                value_obj = new object_short{(int16_t)(f->val / s->val)};
            else if constexpr (op_type == emc_operators::REM)
                value_obj = new object_short{(int16_t)(f->val % s->val)};
            else if constexpr (op_type == emc_operators::POW) {
                double p = pow(f->val, s->val);
                check_in_range<double, int16_t>(p);                
                value_obj = new object_short{(int16_t)p};
            } else
                THROW_BUG("");
        } else if (value_type.is_sbyte()) {
            auto *f = dynamic_cast<object_sbyte*>(casted_1);
            auto *s = dynamic_cast<object_sbyte*>(casted_2);
            
            if constexpr (op_type == emc_operators::PLUS)
                value_obj = new object_sbyte{(int8_t)(f->val + s->val)};
            else if constexpr (op_type == emc_operators::MINUS)
                value_obj = new object_sbyte{(int8_t)(f->val - s->val)};
            else if constexpr (op_type == emc_operators::MULT)
                value_obj = new object_sbyte{(int8_t)(f->val * s->val)};
            else if constexpr (op_type == emc_operators::INTDIV)
                value_obj = new object_sbyte{(int8_t)(f->val / s->val)};
            else if constexpr (op_type == emc_operators::REM)
                value_obj = new object_sbyte{(int8_t)(f->val % s->val)};
            else if constexpr (op_type == emc_operators::POW) {
                double p = pow(f->val, s->val);
                check_in_range<double, int8_t>(p);
                value_obj = new object_sbyte{(int8_t)p};
            } else
                THROW_BUG("");
        } else if (value_type.is_ulong()) {
            auto *f = dynamic_cast<object_ulong*>(casted_1);
            auto *s = dynamic_cast<object_ulong*>(casted_2);

            if constexpr (op_type == emc_operators::PLUS)
                value_obj = new object_ulong{(uint64_t)(f->val + s->val)};
            else if constexpr (op_type == emc_operators::MINUS)
                value_obj = new object_ulong{(uint64_t)(f->val - s->val)};
            else if constexpr (op_type == emc_operators::MULT)
                value_obj = new object_ulong{(uint64_t)(f->val * s->val)};
            else if constexpr (op_type == emc_operators::INTDIV)
                value_obj = new object_ulong{(uint64_t)(f->val / s->val)};
            else if constexpr (op_type == emc_operators::REM)
                value_obj = new object_ulong{(uint64_t)(f->val % s->val)};
            else if constexpr (op_type == emc_operators::POW) {
                double p = pow(f->val, s->val);
                if (p < -9007199254740992 || p > 9007199254740992)
                    THROW_NOT_IMPLEMENTED("To big answer from Long power operator");
                value_obj = new object_ulong{(uint64_t)p};
            } else
                THROW_BUG("");
        } else if (value_type.is_uint()) {
            auto *f = dynamic_cast<object_uint*>(casted_1);
            auto *s = dynamic_cast<object_uint*>(casted_2);

            if constexpr (op_type == emc_operators::PLUS)
                value_obj = new object_uint{(uint32_t)(f->val + s->val)};
            else if constexpr (op_type == emc_operators::MINUS)
                value_obj = new object_uint{(uint32_t)(f->val - s->val)};
            else if constexpr (op_type == emc_operators::MULT)
                value_obj = new object_uint{(uint32_t)(f->val * s->val)};
            else if constexpr (op_type == emc_operators::INTDIV)
                value_obj = new object_uint{(uint32_t)(f->val / s->val)};
            else if constexpr (op_type == emc_operators::REM)
                value_obj = new object_uint{(uint32_t)(f->val % s->val)};
            else if constexpr (op_type == emc_operators::POW) {
                double p = pow(f->val, s->val);
                check_in_range<double, uint32_t>(p);
                value_obj = new object_uint{(uint32_t)p};
            } else
                THROW_BUG("");
        } else if (value_type.is_ushort()) {
            auto *f = dynamic_cast<object_ushort*>(casted_1);
            auto *s = dynamic_cast<object_ushort*>(casted_2);

            if constexpr (op_type == emc_operators::PLUS)
                value_obj = new object_ushort{(uint16_t)(f->val + s->val)};
            else if constexpr (op_type == emc_operators::MINUS)
                value_obj = new object_ushort{(uint16_t)(f->val - s->val)};
            else if constexpr (op_type == emc_operators::MULT)
                value_obj = new object_ushort{(uint16_t)(f->val * s->val)};
            else if constexpr (op_type == emc_operators::INTDIV)
                value_obj = new object_ushort{(uint16_t)(f->val / s->val)};
            else if constexpr (op_type == emc_operators::REM)
                value_obj = new object_ushort{(uint16_t)(f->val % s->val)};
            else if constexpr (op_type == emc_operators::POW) {
                double p = pow(f->val, s->val);
                check_in_range<double, uint16_t>(p);
                value_obj = new object_ushort{(uint16_t)p};
            } else
                THROW_BUG("");
        } else if (value_type.is_byte()) {
            auto *f = dynamic_cast<object_byte*>(casted_1);
            auto *s = dynamic_cast<object_byte*>(casted_2);
            
            if constexpr (op_type == emc_operators::PLUS)
                value_obj = new object_byte{(uint8_t)(f->val + s->val)};
            else if constexpr (op_type == emc_operators::MINUS)
                value_obj = new object_byte{(uint8_t)(f->val - s->val)};
            else if constexpr (op_type == emc_operators::MULT)
                value_obj = new object_byte{(uint8_t)(f->val * s->val)};
            else if constexpr (op_type == emc_operators::INTDIV)
                value_obj = new object_byte{(uint8_t)(f->val / s->val)};
            else if constexpr (op_type == emc_operators::REM)
                value_obj = new object_byte{(uint8_t)(f->val % s->val)};
            else if constexpr (op_type == emc_operators::POW) {
                double p = pow(f->val, s->val);
                check_in_range<double, uint8_t>(p);
                value_obj = new object_byte{(uint8_t)p};
            } else
                THROW_BUG("");
        } else if (value_type.is_double()) {
            auto *f = dynamic_cast<object_double*>(casted_1);
            auto *s = dynamic_cast<object_double*>(casted_2);

            if constexpr (op_type == emc_operators::PLUS)
                value_obj = new object_double{(double)(f->val + s->val)};
            else if constexpr (op_type == emc_operators::MINUS)
                value_obj = new object_double{(double)(f->val - s->val)};
            else if constexpr (op_type == emc_operators::MULT)
                value_obj = new object_double{(double)(f->val * s->val)};
            else if constexpr (op_type == emc_operators::RDIV)
                value_obj = new object_double{(double)(f->val / s->val)};
            else if constexpr (op_type == emc_operators::REM)
                value_obj = new object_double{(double)fmod(f->val, s->val)};
            else if constexpr (op_type == emc_operators::POW) {
                double p = pow(f->val, s->val);
                value_obj = new object_double{p};
            } else if constexpr (op_type == emc_operators::INTDIV) {
                double p = f->val / s->val;
                value_obj = new object_double{floor(p)};
            } else
                THROW_BUG("");
        } else if (value_type.is_float()) {
            auto *f = dynamic_cast<object_float*>(casted_1);
            auto *s = dynamic_cast<object_float*>(casted_2);

            if constexpr (op_type == emc_operators::PLUS)
                value_obj = new object_float{(float)(f->val + s->val)};
            else if constexpr (op_type == emc_operators::MINUS)
                value_obj = new object_float{(float)(f->val - s->val)};
            else if constexpr (op_type == emc_operators::MULT)
                value_obj = new object_float{(float)(f->val * s->val)};
            else if constexpr (op_type == emc_operators::RDIV)
                value_obj = new object_float{(float)(f->val / s->val)};
            else if constexpr (op_type == emc_operators::REM)
                value_obj = new object_float{(float)fmodf(f->val, s->val)};
            else if constexpr (op_type == emc_operators::POW) {
                double p = pow(f->val, s->val);
                check_in_range<double, float>(p);
                value_obj = new object_float{(float)p};
            } else if constexpr (op_type == emc_operators::INTDIV) {
                float p = f->val / s->val;
                value_obj = new object_float{floorf(p)};
            } else
                THROW_BUG("");
        } else
            THROW_NOT_IMPLEMENTED("");
        delete casted_1; delete casted_2;
    }
};

class ast_node_typedotchain: public ast_node {
public:
    ast_node_typedotchain()
    {
        type = ast_type::TYPEDOTCHAIN;

    }

    void append_type(std::string type_name)
    {
        v_type_names.push_back(type_name);
    }

    ~ast_node_typedotchain() {}

    std::vector<std::string> v_type_names;

    emc_type resolve()
    {
        std::string full_type_name = resolve_full_type_name();

        value_type = compilation_units.get_current_typestack().find_type(full_type_name);

        return value_type;
    }

    void resolve_name_and_namespace(std::string *name, std::string *nspace)
    {
        /* Push the namespace elements to ns */
        for (int i = 0; i < v_type_names.size() - 1; i++) {
            std::string name = v_type_names[i];
            *nspace += name;
            if (i != v_type_names.size() - 2)
                *nspace += ".";
        }
        /* The last one is the name */
        DEBUG_ASSERT(v_type_names.size(),"");
        *name = v_type_names.back();
    }

    std::string resolve_full_type_name()
    {
        std::string full_type_name;

        /* Append vector to string like A.B.C.D */
        for (int i = 0; i < v_type_names.size(); i++) {
            std::string name = v_type_names[i];
            full_type_name += name;
            if (i != v_type_names.size() - 1)
                full_type_name += ".";
        }

        return full_type_name;
    }

    ast_node* clone()
    {
        auto c = new ast_node_typedotchain {};
        c->v_type_names = v_type_names;
        c->value_type = value_type;
        return c;        
    }
};

class ast_node_typedotnamechain: public ast_node {
public:
    ast_node_typedotnamechain(std::string name, ast_node* typedotchain) :
        name(name),
        typedotchain(typedotchain)
    {
        type = ast_type::TYPEDOTNAMECHAIN;
    }

    ~ast_node_typedotnamechain() { delete typedotchain;}

    ast_node* typedotchain = nullptr;
    std::string name;
    std::string full_name;
    std::string nspace;

    emc_type resolve()
    {
        /* If there is A.B.C. prepended to the name of this object we need to include it
         * in the objects name */
        if (typedotchain) {
            ast_node_typedotchain *tc = dynamic_cast<ast_node_typedotchain*>(typedotchain);
            DEBUG_ASSERT_NOTNULL(tc);
            /* Resolve all the A.B.C before the .d in a A.B.C.d */
            std::string ns = tc->resolve_full_type_name();
            nspace = ns;
            full_name = ns + "." + name;
        } else 
            full_name = name;
        return value_type = emc_type{emc_types::NONE};
    }

    ast_node* clone()
    {
        auto c = new ast_node_typedotnamechain {name, typedotchain ? typedotchain->clone() : 0};
        c->value_type = value_type;
        return c;        
    }
};

class ast_node_usingchain: public ast_node {
public:
    ast_node_usingchain(ast_node* typedotchain)
    {
        type = ast_type::USINGCHAIN;
        auto node = dynamic_cast<ast_node_typedotchain*>(typedotchain);
        DEBUG_ASSERT_NOTNULL(node);
        vec_typedotchains.push_back(node);
    }

    ~ast_node_usingchain() 
    {
        for (auto e : vec_typedotchains)
         delete e;
    }

    std::vector<ast_node_typedotchain*> vec_typedotchains;

    void append_typedotchain(ast_node *typedotchain)
    {
        auto node = dynamic_cast<ast_node_typedotchain*>(typedotchain);
        DEBUG_ASSERT_NOTNULL(node);
        vec_typedotchains.push_back(node);
    }   

    emc_type resolve()
    {
        return value_type = emc_type{emc_types::NONE};
    }

    ast_node* clone()
    {
        auto c = new ast_node_usingchain {vec_typedotchains.front()->clone()};
        for (int i = 1; i < vec_typedotchains.size(); i++) {
            auto node = dynamic_cast<ast_node_typedotchain*>(vec_typedotchains[i]->clone());
            DEBUG_ASSERT_NOTNULL(node);
            c->vec_typedotchains.push_back(node);
        }
        c->value_type = value_type;
        return c;        
    }
};

class ast_node_using: public ast_node {
public:
    ast_node_using(ast_node* usingchain) :
        usingchain(usingchain)
    {
        type = ast_type::USING;
    }
    ast_node_using(ast_node* usingchain, bool using_ns) :
        usingchain(usingchain) , using_ns(using_ns)
    {
        type = ast_type::USING;
    }

    ~ast_node_using() { delete usingchain;}

    ast_node* usingchain = nullptr;
    ast_node* root = nullptr;
    compilation_unit *compunit = nullptr;
    bool using_ns = false;
    emc_type resolve();

    ast_node* clone()
    {
        auto c = new ast_node_using {usingchain};
        c->value_type = value_type;
        return c;        
    }
};

class ast_node_return : public ast_node {
public:
    ast_node_return(ast_node *first)
    : first(first) { type = ast_type::RETURN;}

    ~ast_node_return(){delete first;}

    ast_node *first; /* rvalue to return */

    emc_type resolve()
    {
        if (first)
            return value_type = first->resolve();
        else /* Nothing to return .. */
            return value_type = emc_type{emc_types::NONE};
    }
};

class ast_node_double_literal: public ast_node {
public:
    ast_node_double_literal()
    {
        type = ast_type::DOUBLE_LITERAL;
        d = 0;
    }
    ast_node_double_literal(double d) :
            d(d)
    {
        type = ast_type::DOUBLE_LITERAL;
    }
    ast_node_double_literal(std::string s)
    {
        type = ast_type::DOUBLE_LITERAL;
        d = stod(s);
    }
    ~ast_node_double_literal()
    {
    }

    double d;

    emc_type resolve()
    {
        value_type = emc_type{emc_types::DOUBLE};
        value_type.is_const_expr = true;
        return value_type;
    }

    obj* resolve_value()
    {
        return value_obj = new object_double{d};
    }
};

class ast_node_int_literal: public ast_node {
public:
    ast_node_int_literal()
    {
        type = ast_type::INT_LITERAL;
        i = 0;
    }
    ast_node_int_literal(int i) : i(i)
    {
        type = ast_type::INT_LITERAL;
    }
    ast_node_int_literal(std::string s)
    {
        type = ast_type::INT_LITERAL;
        char *pc;
        errno = 0;
        long long l = strtol(s.c_str(), &pc, 0);

        /* TODO: -2147483648 will be read as '-' '2147483648' by bison so this need rework
         * so that -2147483648 works as a int literal. */
        if (l > std::numeric_limits<int>::max() ||
            l < std::numeric_limits<int>::min())
            THROW_USER_ERROR_LOC("Value out of Int range: " + s);
        if (errno == ERANGE)
            THROW_USER_ERROR_LOC("String not an valid Int: " + s);
        i = (int)l;
    }
    ~ast_node_int_literal()
    {
    }

    int i;

    emc_type resolve()
    {
        value_type = emc_type{emc_types::INT};
        value_type.is_const_expr = true;
        return value_type;
    }

    obj* resolve_value()
    {
        return value_obj = new object_long{i};
    }
};

class ast_node_string_literal: public ast_node {
public:
    ast_node_string_literal(std::string s)
    {
        type = ast_type::STRING_LITERAL;
        deescape_string(s);
        this->s = s;
    }
    ~ast_node_string_literal()
    {
    }

    std::string s;

    emc_type resolve()
    {
        value_type = emc_type{emc_types::STRING};
        return value_type;
    }
};

class ast_node_and: public ast_node {
public:
    ast_node_and() :
            ast_node_and(nullptr, nullptr)
    {
    }
    ast_node_and(ast_node *first, ast_node *sec) :
            first(first), sec(sec)
    {
        type = ast_type::AND;
    }

    ~ast_node_and()
    {
        delete first;
        delete sec;
    }

    ast_node *first;
    ast_node *sec;

    emc_type resolve()
    {
        return value_type = emc_type{emc_types::INT};
    }
};

class ast_node_or: public ast_node {
public:
    ast_node_or() :
            ast_node_or(nullptr, nullptr)
    {
    }
    ast_node_or(ast_node *first, ast_node *sec) :
            first(first), sec(sec)
    {
        type = ast_type::OR;
    }

    ~ast_node_or()
    {
        delete first;
        delete sec;
    }

    ast_node *first;
    ast_node *sec;

    emc_type resolve()
    {
        return value_type = emc_type{emc_types::INT};
    }
};

class ast_node_xor: public ast_node {
public:
    ast_node_xor() :
            ast_node_xor(nullptr, nullptr)
    {
    }
    ast_node_xor(ast_node *first, ast_node *sec) :
            first(first), sec(sec)
    {
        type = ast_type::XOR;
    }

    ~ast_node_xor()
    {
        delete first;
        delete sec;
    }

    ast_node *first;
    ast_node *sec;

    emc_type resolve()
    {
        return value_type = emc_type{emc_types::INT};
    }
};

class ast_node_nor: public ast_node {
public:
    ast_node_nor() :
            ast_node_nor(nullptr, nullptr)
    {
    }
    ast_node_nor(ast_node *first, ast_node *sec) :
            first(first), sec(sec)
    {
        type = ast_type::NOR;
    }

    ~ast_node_nor()
    {
        delete first;
        delete sec;
    }

    ast_node *first;
    ast_node *sec;

    emc_type resolve()
    {
        return value_type = emc_type{emc_types::INT};
    }
};


class ast_node_ptrdef_list: public ast_node {
public:
    ast_node_ptrdef_list()
    {
        type = ast_type::PTRDEF_LIST;
    }
    ~ast_node_ptrdef_list()
    {
        
    }
    
    std::vector<bool> v_const;

    ast_node* clone()
    {
        auto argl = new ast_node_ptrdef_list { };
        argl->v_const = v_const;
        return argl;
    }

    void append_const(bool is_const)
    {
        v_const.push_back(is_const);
    }

    emc_type resolve()
    {
        return emc_types::NONE;
    }
};

class ast_node_nand: public ast_node {
public:
    ast_node_nand() :
            ast_node_nand(nullptr, nullptr)
    {
    }
    ast_node_nand(ast_node *first, ast_node *sec) :
            first(first), sec(sec)
    {
        type = ast_type::NAND;
    }

    ~ast_node_nand()
    {
        delete first;
        delete sec;
    }

    ast_node *first;
    ast_node *sec;

    emc_type resolve()
    {
        return value_type = emc_type{emc_types::INT};
    }
};

class ast_node_xnor: public ast_node {
public:
    ast_node_xnor() :
            ast_node_xnor(nullptr, nullptr)
    {
    }
    ast_node_xnor(ast_node *first, ast_node *sec) :
            first(first), sec(sec)
    {
        type = ast_type::XNOR;
    }

    ~ast_node_xnor()
    {
        delete first;
        delete sec;
    }

    ast_node *first;
    ast_node *sec;

    emc_type resolve()
    {
        return value_type = emc_type{emc_types::INT};
    }
};

class ast_node_not: public ast_node {
public:
    ast_node_not() :
            ast_node_not(nullptr)
    {
    }
    ast_node_not(ast_node *first) :
            first(first)
    {
        type = ast_type::NOT;
    }

    ~ast_node_not()
    {
        delete first;
    }

    ast_node *first;

    emc_type resolve()
    {
        return value_type = emc_type{emc_types::INT};
    }
};

class ast_node_add: public ast_node_bin_op {
public:
    ast_node_add() :
            ast_node_add(nullptr, nullptr)
    {
    }
    ast_node_add(ast_node *first, ast_node *sec) :
            ast_node_bin_op(first, sec)
    {
        type = ast_type::ADD;
    }

    emc_type resolve()
    {
        value_type = standard_type_promotion(first->resolve(), sec->resolve());

        /* Create an object from the value of first and sec's objects */
        if (value_type.is_const_expr)
            make_value_obj<emc_operators::PLUS>();

        return value_type;
    }

    obj* resolve_value()
    {
        return value_obj;
    }
};

class ast_node_sub: public ast_node_bin_op {
public:
    ast_node_sub() :
            ast_node_sub(nullptr, nullptr)
    {
    }
    ast_node_sub(ast_node *first, ast_node *sec) :
            ast_node_bin_op(first, sec)
    {
        type = ast_type::SUB;
    }

    emc_type resolve()
    {
        value_type = standard_type_promotion(first->resolve(), sec->resolve());

        /* Create an object from the value of first and sec's objects */
        if (value_type.is_const_expr)
            make_value_obj<emc_operators::MINUS>();

        return value_type;
    }

    obj* resolve_value()
    {
        return value_obj;
    }
};

class ast_node_mul: public ast_node_bin_op {
public:
    ast_node_mul() :
            ast_node_mul(nullptr, nullptr)
    {
    }
    ast_node_mul(ast_node *first, ast_node *sec) :
            ast_node_bin_op(first, sec)
    {
        type = ast_type::MUL;
    }

    emc_type resolve()
    {
        value_type = standard_type_promotion(first->resolve(), sec->resolve());

        /* Create an object from the value of first and sec's objects */
        if (value_type.is_const_expr)
            make_value_obj<emc_operators::MULT>();

        return value_type;
    }

    obj* resolve_value()
    {
        return value_obj;
    }
};

class ast_node_rem: public ast_node_bin_op {
public:
    ast_node_rem() :
            ast_node_rem(nullptr, nullptr)
    {
    }
    ast_node_rem(ast_node *first, ast_node *sec) :
            ast_node_bin_op(first, sec)
    {
        type = ast_type::REM;
    }

    emc_type resolve()
    {
        value_type = standard_type_promotion(first->resolve(), sec->resolve());

        /* Create an object from the value of first and sec's objects */
        if (value_type.is_const_expr)
            make_value_obj<emc_operators::REM>();

        return value_type;
    }

    obj* resolve_value()
    {
        return value_obj;
    }
};

class ast_node_intdiv: public ast_node_bin_op {
public:
    ast_node_intdiv() :
            ast_node_intdiv(nullptr, nullptr)
    {
    }
    ast_node_intdiv(ast_node *first, ast_node *sec) :
            ast_node_bin_op(first, sec)
    {
        type = ast_type::INTDIV;
    }

    emc_type resolve()
    {
        value_type = standard_type_promotion(first->resolve(), sec->resolve());

        /* Create an object from the value of first and sec's objects */
        if (value_type.is_const_expr)
            make_value_obj<emc_operators::INTDIV>();

        return value_type;
    }

    obj* resolve_value()
    {
        return value_obj;
    }
};

class ast_node_rdiv: public ast_node_bin_op {
public:
    ast_node_rdiv() :
            ast_node_rdiv(nullptr, nullptr)
    {
    }
    ast_node_rdiv(ast_node *first, ast_node *sec) :
            ast_node_bin_op(first, sec)
    {
        type = ast_type::RDIV;
    }

    emc_type resolve()
    {
        auto value_type_tmp = standard_type_promotion(first->resolve(), sec->resolve());
        /* rdiv only operates on floating point types */
        if (!(value_type_tmp.is_double() || value_type_tmp.is_float()))
            value_type = cast_to(value_type_tmp, emc_types::DOUBLE);
        else
            value_type = value_type_tmp;

        /* Create an object from the value of first and sec's objects */
        if (value_type.is_const_expr)
            make_value_obj<emc_operators::RDIV>();

        return value_type;
    }

    obj* resolve_value()
    {
        return value_obj;
    }
};

class ast_node_deref: public ast_node {
public:
    ast_node_deref() :
            ast_node_deref(nullptr)
    {
    }
    ast_node_deref(ast_node *first) :
            first(first)
    {
        type = ast_type::DEREF;
    }

    ~ast_node_deref()
    {
        if (first) delete first;
    }
    
    ast_node *first;

    emc_type resolve()
    {
        value_type = first->resolve();
        if(!value_type.n_pointer_indirections)
            THROW_USER_ERROR_LOC("Dereferencing non-pointer");
        value_type.n_pointer_indirections--;
        return value_type;
    }
};

class ast_node_address: public ast_node {
public:
    ast_node_address() :
            ast_node_address(nullptr)
    {
    }
    ast_node_address(ast_node *first) :
            first(first)
    {
        type = ast_type::ADDRESS;
    }

    ~ast_node_address()
    {
        if (first) delete first;
    }
    
    ast_node *first;

    emc_type resolve()
    {
        value_type = first->resolve();
        value_type.n_pointer_indirections++;
        return value_type;
    }

    ast_node* clone()
    {
        auto c = new ast_node_address { first->clone()};
        c->value_type = value_type;
        return c;
    }
};

class ast_node_pow: public ast_node_bin_op {
public:
    ast_node_pow() :
            ast_node_pow(nullptr, nullptr)
    {
    }
    ast_node_pow(ast_node *first, ast_node *sec) :
            ast_node_bin_op(first, sec)
    {
        type = ast_type::POW;
    }

    emc_type resolve()
    {
        value_type = standard_type_promotion(first->resolve(), sec->resolve());

        if (value_type.is_const_expr)
            make_value_obj<emc_operators::POW>();

        return value_type;
    }

    obj* resolve_value()
    {
        return value_obj;
    }
};

class ast_node_abs: public ast_node {
public:
    ast_node_abs(ast_node *first) :
            first(first)
    {
        type = ast_type::ABS;
    }
    ~ast_node_abs()
    {
        delete first;
    }

    ast_node *first;

    emc_type resolve()
    {
        return value_type = first->resolve();
    }
};

class ast_node_uminus: public ast_node {
public:
    ast_node_uminus() :
            ast_node_uminus(nullptr)
    {
    }
    ast_node_uminus(ast_node *first) :
            first(first)
    {
        type = ast_type::UMINUS;
    }

    ~ast_node_uminus()
    {
        if (first) delete first;
    }
    
    ast_node *first;

    template<class Obj_class, class C_type>
    void make_value_obj(obj* child_obj)
    {
        auto child_obj_t = dynamic_cast<Obj_class*>(child_obj);
        DEBUG_ASSERT_NOTNULL(child_obj_t);
        auto obj_t = new Obj_class{};
        if (std::is_integral<C_type>() && child_obj_t->val == std::numeric_limits<C_type>::lowest()) {
            THROW_USER_ERROR_LOC("Negating int will be out of range: " + std::to_string(child_obj_t->val));
        }
        obj_t->val = -child_obj_t->val;
        value_obj = obj_t;
    }

    emc_type resolve()
    {
        value_type = first->resolve();

        if (value_type.is_const_expr) {
            auto child_obj = first->resolve_value();
             
            switch (child_obj->type) {
            case object_type::DOUBLE:
                make_value_obj<object_double, double>(child_obj);
                break;
            case object_type::FLOAT:
                make_value_obj<object_float, float>(child_obj);
                break;
            case object_type::LONG:
                make_value_obj<object_long, int64_t>(child_obj);
                break;
            case object_type::INT:
                make_value_obj<object_int, int32_t>(child_obj);
                break;
            case object_type::SHORT:
                make_value_obj<object_short, int16_t>(child_obj);
                break;
            case object_type::SBYTE:
                make_value_obj<object_short, int8_t>(child_obj);
                break;
            case object_type::ULONG:
            case object_type::UINT:
            case object_type::USHORT:
            case object_type::BYTE:
                THROW_USER_ERROR_LOC("Can't negate unsigned integer type with unary minus");
                break;
            case object_type::BOOL:
                THROW_USER_ERROR_LOC("Can't negate bool type with unary minus");
                break;
            }
        }

        return value_type;
    }

    obj* resolve_value()
    {
        return value_obj;
    }
};

class ast_node_var: public ast_node {
public:
    std::string name;
    std::string nspace;
    std::string full_name;
    ast_node *typedotnamechain = nullptr;

    ~ast_node_var()
    {
        delete typedotnamechain;
    }
    ast_node_var(ast_node *typedotnamechain)
    :
            typedotnamechain(typedotnamechain)
    {
        type = ast_type::VAR;
    }

    emc_type resolve()
    {
        

        ast_node_typedotnamechain *node = dynamic_cast<ast_node_typedotnamechain*>(typedotnamechain);
        DEBUG_ASSERT_NOTNULL(node);
        node->resolve();
        name = node->name;
        nspace = node->nspace;
        full_name = node->full_name;
        
        auto v = compilation_units.get_current_objstack().find_objects_by_not_mangled_name(name, nspace);
        if (v.size() == 0) /* TODO: Kanske borde throwa "inte hittat än" för att kunna fortsätta? */
            THROW_USER_ERROR_LOC("Object does not exist: " + full_name);
        /* Pick the object in top scope (which is in the front of the vector from find...() */
        auto p = v.front();

        return value_type = p->resolve();
    }

    ast_node* clone()
    {
        auto c = new ast_node_var { typedotnamechain->clone() };
        c->value_type = value_type;
        return c;
    }
};

class ast_node_assign: public ast_node {
public:
    ~ast_node_assign()
    {
        if (first) delete first;
        if (sec) delete sec;
    }
    ast_node_assign(ast_node *first, ast_node *sec)
    :
            first(first), sec(sec)
    {
        type = ast_type::ASSIGN;
    }
    
    ast_node *first;
    ast_node *sec;

    emc_type resolve()
    {
        sec->resolve();
        return value_type = first->resolve();
    }
};

class ast_node_cmp: public ast_node {
public:
    ast_node_cmp() :
            ast_node_cmp(nullptr, nullptr)
    {
    }
    ast_node_cmp(ast_node *first, ast_node *sec) :
            first(first), sec(sec)
    {
        type = ast_type::CMP;
    }

    ~ast_node_cmp()
    {
        if (first) delete first;
        if (sec) delete sec;
    }
    
    ast_node *first;
    ast_node *sec;

    emc_type resolve()
    {
        first->resolve(); sec->resolve();
        return value_type = emc_type{emc_types::INT};
    }
};

class ast_node_explist: public ast_node {
public:
    ast_node_explist(ast_node *first)
    {
        if (first)
            v_nodes.push_back(first);
        type = ast_type::EXPLIST;
    }

    ast_node_explist() : ast_node_explist(nullptr) {}

    ~ast_node_explist()
    {
        for (auto e : v_nodes)
            delete e;
    }
    
    std::vector<ast_node*> v_nodes;

    emc_type resolve()
    {
        /* If we don't find any real type in the list the type is NONE.
         * I.e. can't be used for expression assignments etc.
         */
        emc_type t = emc_type{emc_types::NONE};
        for (auto e : v_nodes) {
            auto et = e->resolve();
            if (t.type == emc_types::NONE && et.type != emc_types::NONE)
                t = et;
        }
        return value_type = t;
    }

    void append_node(ast_node *first)
    {
        v_nodes.push_back(first);
    }
};

class ast_node_doblock: public ast_node {
public:
    ast_node_doblock(ast_node *first) :
            first(first)
    {
        type = ast_type::DOBLOCK;
    }
    ~ast_node_doblock()
    {
        if (first) delete first;
    }
    
    ast_node *first;

    emc_type resolve()
    {
        
        compilation_units.get_current_objstack().push_new_scope();
        value_type = first->resolve();
        compilation_units.get_current_objstack().pop_scope();
        return value_type;
    }
};

class ast_node_elseiflist : public ast_node {
public:
    ast_node_elseiflist(ast_node *cond_e, ast_node *elseif) 
    {
        v_cond_e.push_back(cond_e);
        v_elseif.push_back(elseif);
    }

    ~ast_node_elseiflist() 
    {
        for (auto cond_e : v_cond_e)
            delete cond_e;
        for (auto elseif : v_elseif)
            delete elseif;
    }

    void append_elseif(ast_node* cond_e, ast_node* elseif)
    {
        v_cond_e.push_back(cond_e);
        v_elseif.push_back(elseif);
    }

    emc_type resolve()
    {
        for (auto cond_e : v_cond_e)
            cond_e->resolve();
        for (auto elseif : v_elseif)
            elseif->resolve();

        return emc_type{emc_types::NONE};
    }

    std::vector<ast_node*> v_cond_e;
    std::vector<ast_node*> v_elseif;
};


class ast_node_if: public ast_node {
public:
    ast_node_if(ast_node *cond_e, ast_node *if_el) :
            ast_node_if(cond_e, if_el, nullptr, nullptr, nullptr)
    {
    }
    ast_node_if(ast_node *cond_e, ast_node *if_el, ast_node *else_el) :
                ast_node_if(cond_e, if_el, else_el, nullptr, nullptr) {}
            ast_node_if(ast_node *cond_e, ast_node *if_el, ast_node *else_el, ast_node *also_el) :
                ast_node_if(cond_e, if_el, else_el, also_el, nullptr) {}

    ast_node_if(ast_node *cond_e, ast_node *if_el, ast_node *else_el, ast_node *also_el, ast_node *elseif_el) :
            cond_e(cond_e), if_el(if_el), else_el(else_el), also_el(also_el), elseif_el(elseif_el)
    {
        type = ast_type::IF;
    }
    ~ast_node_if()
    {
        delete cond_e;
        delete if_el;
        delete else_el;
        delete also_el;
        delete elseif_el;
    }

    ast_node *cond_e = 0;
    ast_node *if_el = 0;
    ast_node *elseif_el = 0;
    ast_node *else_el = 0;
    ast_node *also_el = 0;
    /* The ALSO statement is assumed to be in
     * the first IF node and not the linked IFs. */

    /* The type of the value of an IF statement expression is NONE unless
     * all the blocks have a not NONE type, which have to be promotional
     * to eachother.
     */
    emc_type resolve()
    {
        cond_e->resolve();
        compilation_units.get_current_objstack().push_new_scope();
        if_el->resolve();
        compilation_units.get_current_objstack().pop_scope();

        if (elseif_el) {
            auto *elseif_el_t = dynamic_cast<ast_node_elseiflist*>(elseif_el);
            DEBUG_ASSERT_NOTNULL(elseif_el_t);

            for (auto cond_e : elseif_el_t->v_cond_e)
                cond_e->resolve();

            for (auto elseif : elseif_el_t->v_elseif) {
                compilation_units.get_current_objstack().push_new_scope();
                elseif->resolve();
                compilation_units.get_current_objstack().pop_scope();
            }
        }
        if (else_el) {
            compilation_units.get_current_objstack().push_new_scope();
            else_el->resolve();
            compilation_units.get_current_objstack().pop_scope();
        }
        if (also_el) {
            compilation_units.get_current_objstack().push_new_scope();
            also_el->resolve();
            compilation_units.get_current_objstack().pop_scope();
        }

        return value_type = emc_type{emc_types::NONE};
    }
};

class ast_node_while: public ast_node {
public:
    ast_node_while(ast_node *cond_e, ast_node *if_el) :
            ast_node_while(cond_e, if_el, nullptr)
    {
    }
    ast_node_while(ast_node *cond_e, ast_node *if_el, ast_node *else_el) :
            cond_e(cond_e), if_el(if_el), else_el(else_el)
    {
        type = ast_type::WHILE;
    }
    ~ast_node_while()
    {
        delete cond_e;
        delete if_el;
        delete else_el;
    }

    ast_node *cond_e;
    ast_node *if_el;
    ast_node *else_el;

    emc_type resolve()
    {
        cond_e->resolve();
        if (!else_el) {
            
            compilation_units.get_current_objstack().push_new_scope();
            value_type = if_el->resolve();
            compilation_units.get_current_objstack().pop_scope();
            return value_type;
        } else {
            
            compilation_units.get_current_objstack().push_new_scope();
            auto value_type_if = if_el->resolve();
            compilation_units.get_current_objstack().pop_scope();
            compilation_units.get_current_objstack().push_new_scope();
            auto value_type_else = else_el->resolve();
            compilation_units.get_current_objstack().pop_scope();

            auto t = standard_type_promotion_or_invalid(value_type_if, value_type_else);
            if (t.is_valid())
                return value_type = t;
            else
                return value_type = emc_type{emc_types::NONE};
        }
    }
};

class func_para {
public:
    func_para() = default;
    func_para(std::string name) :
            name(name)
    {
    }
    std::string name;
};

/* Class for a list of named definitions in a function definition. */
class ast_node_parlist: public ast_node {
public:
    ast_node_parlist()
    {
        type = ast_type::PARAMETER_LIST;
    }

    ast_node* clone()
    {
        auto pel = new ast_node_parlist { };
        pel->value_type = value_type;
        for (auto e : v_func_paras)
            pel->append_parameter(e);
        return pel;
    }

    emc_type resolve()
    {
        return value_type = emc_type{emc_types::NONE};
    }

    void append_parameter(func_para para)
    {
        v_func_paras.push_back(para);
    }

    /* Parameters, the defintion of the call signature. */
    std::vector<func_para> v_func_paras;
};

class ast_node_vardef_list: public ast_node {
public:
    ast_node_vardef_list()
    {
        type = ast_type::VARDEF_LIST;
    }
    ~ast_node_vardef_list()
    {
        for (auto p : v_defs)
            delete p;
    }
    
    std::vector<ast_node*> v_defs;

    ast_node* clone()
    {
        auto argl = new ast_node_vardef_list { };
        argl->value_type = value_type;
        for (auto e : v_defs)
            argl->append(e->clone());
        return argl;
    }

    void append(ast_node *arg)
    {
        v_defs.push_back(arg);
    }

    emc_type resolve();

};


class ast_node_arglist: public ast_node {
public:
    ast_node_arglist()
    {
        type = ast_type::ARGUMENT_LIST;
    }
    ~ast_node_arglist()
    {
        for (auto p : v_ast_args)
            delete p;
    }
    
    std::vector<ast_node*> v_ast_args;

    ast_node* clone()
    {
        auto argl = new ast_node_arglist { };
        argl->value_type = value_type;
        for (auto e : v_ast_args)
            argl->append_arg(e->clone());
        return argl;
    }

    void append_arg(ast_node *arg)
    {
        v_ast_args.push_back(arg);
    }

    emc_type resolve()
    {
        for (auto e : v_ast_args)
            e->resolve();
        return value_type = emc_type{emc_types::NONE};
    }
};

class ast_node_funcdef: public ast_node {
public:
    
    ast_node_funcdef(ast_node *parlist, ast_node *code_block,
            ast_node* typedotnamechain, ast_node *return_list)
    :
            parlist(parlist), 
            code_block(code_block),
            typedotnamechain(typedotnamechain),
            return_list(return_list)
    {
        type = ast_type::FUNCTION_DEF;
        
        if (!parlist)
            this->parlist = new ast_node_vardef_list{};   
        if (!return_list)
            this->return_list = new ast_node_vardef_list{};  
    }
    ast_node_funcdef(ast_node *parlist, ast_node *code_block,
            ast_node* typedotnamechain, ast_node *return_list,
            bool c_linkage)
    : ast_node_funcdef(parlist, code_block, typedotnamechain, return_list)
    {
        this->c_linkage = c_linkage;
    }

    /* fobj äger ast_noderna kanske är dumt? */
    ~ast_node_funcdef()
    {
        delete code_block;
        delete parlist;
        delete return_list;
        delete typedotnamechain;
    }
    
    ast_node *return_list;
    ast_node *code_block;
    ast_node *parlist; /* Is a return_list */
    ast_node* typedotnamechain = nullptr;
    std::string name;
    std::string nspace;
    std::string mangled_name;
    bool c_linkage = false;

    emc_type resolve();
};

class ast_node_funccall: public ast_node {
public:
    ast_node_funccall(ast_node* typedotnamechain, ast_node *arg_list) :
            typedotnamechain(typedotnamechain), arg_list(arg_list)
    {
        type = ast_type::FUNCTION_CALL;
        if (!arg_list) 
            this->arg_list = new ast_node_arglist{}; /* Make empty dummy arg list for no arg fn:s. */
    }

    ~ast_node_funccall()
    {
        delete arg_list;
        delete typedotnamechain;
    }
    ast_node* typedotnamechain = nullptr;
    std::string mangled_name;
    std::string name;
    std::string nspace;
    ast_node *arg_list;

    emc_type resolve()
    {
        ast_node_typedotnamechain *typenamedot = dynamic_cast<ast_node_typedotnamechain*>(typedotnamechain);
        DEBUG_ASSERT_NOTNULL(typenamedot);
        
        typenamedot->resolve();
        arg_list->resolve();

        name = typenamedot->name;
        nspace = typenamedot->nspace;
        if (name == "one_ptrto_long")
            int b = 0;
        
        std::vector<obj*> v_objs = compilation_units.get_current_objstack().find_objects_by_not_mangled_name(name, nspace);
        if (!v_objs.size()) /* TODO: Kolla så fn */
            THROW_USER_ERROR_LOC("Could not find any function " + nspace + " " + name);

        /* TODO: Make fancier argument matching then exact. */
        obj* choosen_obj = nullptr;
        /* Search the scopes for a matching function signature. v_objs is sorted so that items in the top most
         * stack is first. */
        for (auto obj : v_objs) {
            if (obj->type == object_type::FUNC) {
                object_func *objf = dynamic_cast<object_func*>(obj);
                DEBUG_ASSERT_NOTNULL(objf);
                ast_node_vardef_list *parameter_list = dynamic_cast<ast_node_vardef_list *>(objf->para_list);
                DEBUG_ASSERT_NOTNULL(parameter_list);
                ast_node_arglist *argument_list = dynamic_cast<ast_node_arglist *>(arg_list);
                DEBUG_ASSERT_NOTNULL(argument_list);

                if (parameter_list->v_defs.size() != argument_list->v_ast_args.size())
                    continue;
                bool hit = true;
                /* Check if the parameters in the function object and the arguments of the call
                 * all have the same type. */
                for (int i = 0; i < parameter_list->v_defs.size(); i++) {
                    if (parameter_list->v_defs[i]->value_type != argument_list->v_ast_args[i]->value_type) {
                        hit = false;
                        break;
                    }
                }
                if (hit) {
                    choosen_obj = obj;
                    break;
                }
            } else 
                THROW_USER_ERROR_LOC("There are objects of '" + name + "' that is not a function");
        }
        if (!choosen_obj)
            THROW_USER_ERROR_LOC("There are no objects of '" + name + "' that match the call signature");
        /* This function call will call this mangled name */
        mangled_name = choosen_obj->mangled_name;

        return value_type = choosen_obj->resolve();
    }
};

class ast_node_funcdec: public ast_node {
public:
    ast_node_funcdec(ast_node *parlist, 
            ast_node* typedotnamechain, 
            ast_node *return_list,
            bool c_linkage)
    :
        parlist(parlist),
        typedotnamechain(typedotnamechain),
        return_list(return_list),
        c_linkage(c_linkage)
    {
        type = ast_type::FUNCTION_DECL;
        
        if (!parlist)
            this->parlist = new ast_node_vardef_list{};   
        if (!return_list)
            this->return_list = new ast_node_vardef_list{};  
    }
    /* fobj äger ast_noderna kanske är dumt? */
    ~ast_node_funcdec()
    {
        delete parlist;
        delete return_list;
        delete typedotnamechain;
    }
    
    ast_node *return_list;
    ast_node *parlist; /* Is a return_list */
    ast_node* typedotnamechain = nullptr;
    std::string name;
    std::string nspace;
    std::string mangled_name;
    bool c_linkage;

    emc_type resolve();
};

class ast_node_chainable: public ast_node {
public:
    std::shared_ptr<ast_node> first;
    std::shared_ptr<ast_node> sec;

    /* The chainable:s resolve() is not called when they are chained. So
     * set value_type here. */
    ast_node_chainable(){value_type = emc_type{emc_types::INT};}
};

class ast_node_andchain: public ast_node {
public:
    ast_node_andchain() : ast_node_andchain(nullptr) {}
    ast_node_andchain(ast_node_chainable *first)
    {
        if (first)
            v_children.push_back(first);
        type = ast_type::ANDCHAIN;
    }

    ~ast_node_andchain()
    {
        for (auto e : v_children)
            delete e;
    }

    std::vector<ast_node_chainable*> v_children;

    emc_type resolve()
    {
        for (auto e : v_children) {
            e->first->resolve();
        }
        (*v_children.rbegin())->sec->resolve(); /* Dont forget this one */
        return value_type = emc_type{emc_types::INT}; /* Always an int */
    }

    void append_next(ast_node_chainable *node)
    {
        DEBUG_ASSERT(node != nullptr, "node is null");
        DEBUG_ASSERT(v_children.size(), "v_children is empty");
        DEBUG_ASSERT(node->first == nullptr, "*node->first not a nullptr");
        node->first = v_children.back()->sec;
        v_children.push_back(node);
    }

    std::shared_ptr<ast_node> last_chain_node_leaf()
    {
        return v_children.back()->sec;
    }
};

class ast_node_les: public ast_node_chainable {
public:
    ast_node_les() :
            ast_node_les(nullptr, nullptr)
    {
    }
    ast_node_les(ast_node *first, ast_node *sec)
    {
        this->first = std::shared_ptr<ast_node>(first);
        this->sec = std::shared_ptr<ast_node>(sec);
        type = ast_type::LES;
    }

    ~ast_node_les()
    {
    }

    emc_type resolve()
    {
        first->resolve(); sec->resolve();
        return value_type = emc_type{emc_types::INT};
    }

};

class ast_node_gre: public ast_node_chainable {
public:
    ast_node_gre() :
            ast_node_gre(nullptr, nullptr)
    {
    }
    ast_node_gre(ast_node *first, ast_node *sec)
    {
        this->first = std::shared_ptr<ast_node>(first);
        this->sec = std::shared_ptr<ast_node>(sec);
        type = ast_type::GRE;
    }

    ~ast_node_gre()
    {
    }
    emc_type resolve()
    {
        first->resolve(); sec->resolve();
        return value_type = emc_type{emc_types::INT};
    }
};

class ast_node_equ: public ast_node_chainable {
public:
    ast_node_equ() :
            ast_node_equ(nullptr, nullptr)
    {
    }
    ast_node_equ(ast_node *first, ast_node *sec)
    {
        this->first = std::shared_ptr<ast_node>(first);
        this->sec = std::shared_ptr<ast_node>(sec);
        type = ast_type::EQU;
    }

    ~ast_node_equ()
    {
    }
    emc_type resolve()
    {
        emc_type type1 = first->resolve(); 
        emc_type type2 = sec->resolve();

        /* TODO: This node could be const_expr too if
           both these are that. */
        if (type1.is_const_expr) {
            first->resolve_value();
        }
        if (type2.is_const_expr) {
            sec->resolve_value();
        }

        return value_type = emc_type{emc_types::INT};
    }
};

class ast_node_leq: public ast_node_chainable {
public:
    ast_node_leq() :
            ast_node_leq(nullptr, nullptr)
    {
    }
    ast_node_leq(ast_node *first, ast_node *sec)
    {
        this->first = std::shared_ptr<ast_node>(first);
        this->sec = std::shared_ptr<ast_node>(sec);
        type = ast_type::LEQ;
    }

    ~ast_node_leq()
    {
    }
    emc_type resolve()
    {
        first->resolve(); sec->resolve();
        return value_type = emc_type{emc_types::INT};
    }
};

class ast_node_geq: public ast_node_chainable {
public:
    ast_node_geq() :
            ast_node_geq(nullptr, nullptr)
    {
    }
    ast_node_geq(ast_node *first, ast_node *sec)
    {
        this->first = std::shared_ptr<ast_node>(first);
        this->sec = std::shared_ptr<ast_node>(sec);
        type = ast_type::GEQ;
    }

    ~ast_node_geq()
    {
    }
    emc_type resolve()
    {
        first->resolve(); sec->resolve();
        return value_type = emc_type{emc_types::INT};
    }
};

class ast_node_neq: public ast_node_chainable {
public:
    ast_node_neq() :
            ast_node_neq(nullptr, nullptr)
    {
    }

    ast_node_neq(ast_node *first, ast_node *sec)
    {
        this->first = std::shared_ptr<ast_node>(first);
        this->sec = std::shared_ptr<ast_node>(sec);
        type = ast_type::NEQ;
    }

    ~ast_node_neq()
    {
    }

    emc_type resolve()
    {
        first->resolve(); sec->resolve();
        return value_type = emc_type{emc_types::INT};
    }
};

class ast_node_def: public ast_node {
public:
    ast_node_def(   
                    ast_node* typedotchain, 
                    ast_node *typedotnamechain,
                    ast_node *value_node
                ) : 
                    value_node(value_node),
                    typedotchain(typedotchain),
                    typedotnamechain(typedotnamechain)
    {
        type = ast_type::DEF;
    }

    ~ast_node_def()
    {
        delete value_node;
        delete ptrdef_node;
        delete typedotchain;
        delete typedotnamechain;
    }
    
    std::string type_name;
    std::string var_name;       /* The name of the variable (without any namespace) */
    std::string nspace;         /* Absolute namespace */
    std::string mangled_name;   /* If the def is in filescope the mangled name is here */
    std::string full_name;      /* Full name on the form: Name.Space.objname */
    ast_node *value_node = nullptr;   /* Rh value node Foo Bar.name = *baz()* */
    ast_node *ptrdef_node = nullptr;
    ast_node *typedotchain = nullptr; /* The type: *Foo* Bar.name */
    ast_node *typedotnamechain = nullptr; /* The name with ns: Foo *Bar.name* */
    int n_pointer_indirections = 0;
    bool clinkage = false;

    ast_node* clone()
    {
        auto c = new ast_node_def { typedotchain->clone(), typedotnamechain->clone(),
                value_node ? value_node->clone() : nullptr };
        c->ptrdef_node = ptrdef_node ? ptrdef_node->clone() : nullptr; 
        c->value_type = value_type;
        c->type_name = type_name;
        c->var_name = var_name;
        c->nspace = nspace;
        c->mangled_name = mangled_name;
        c->full_name = full_name;
        c->n_pointer_indirections = n_pointer_indirections;
        return c;        
    }

    emc_type resolve()
    {
        /* Resolve the type node */
        emc_type type = typedotchain->resolve();

        /* Resolve the name node with a potential namespace */
        ast_node_typedotnamechain* typedotnamechain_T = 
            dynamic_cast<ast_node_typedotnamechain*>(typedotnamechain);
        DEBUG_ASSERT_NOTNULL(typedotnamechain_T);
        typedotnamechain_T->resolve();
        var_name = typedotnamechain_T->name;
        nspace = typedotnamechain_T->nspace;

        if (compilation_units.get_current_objstack().is_in_global_scope() && nspace.size())
            THROW_USER_ERROR_LOC("Can't specify a namespace in variable declarations in a scope: " + nspace + "." + var_name);

        /* Set how many pointer indirections this var def has */
        if (ptrdef_node) {
            ptrdef_node->resolve();
            auto ptrdef_node_t = dynamic_cast<ast_node_ptrdef_list*>(ptrdef_node);
            DEBUG_ASSERT_NOTNULL(ptrdef_node_t);
            n_pointer_indirections = (int)ptrdef_node_t->v_const.size();
            type.n_pointer_indirections = n_pointer_indirections;
        }
        /* TODO: Bitfield for const ptr? volatile? */

        obj *od = nullptr;
        if (type.is_double())
            od = new object_double{var_name, 0, n_pointer_indirections};
        else if (type.is_float())
            od = new object_float{var_name, 0, n_pointer_indirections};
        else if (type.is_long())
            od = new object_long{var_name, 0, n_pointer_indirections};
        else if (type.is_int())
            od = new object_int{var_name, 0, n_pointer_indirections};
        else if (type.is_short())
            od = new object_short{var_name, 0, n_pointer_indirections};
        else if (type.is_sbyte())
            od = new object_sbyte{var_name, 0, n_pointer_indirections};
        else if (type.is_bool())
            od = new object_bool{var_name, 0, n_pointer_indirections};
        else if (type.is_ulong())
            od = new object_ulong{var_name, 0, n_pointer_indirections};
        else if (type.is_uint())
            od = new object_uint{var_name, 0, n_pointer_indirections};
        else if (type.is_ushort())
            od = new object_ushort{var_name, 0, n_pointer_indirections};
        else if (type.is_byte())
            od = new object_byte{var_name, 0, n_pointer_indirections};
        else if (type.is_struct())
            od = new object_struct{var_name, "", type, n_pointer_indirections};
        else
            THROW_NOT_IMPLEMENTED("Type not implemented ast_node_def");

        /* Flytta in i ctor för objektet? */
        
        /* If we are in filescope (global scope) any declaration might be in a
         * namespace, so we add it to the specified namespace. */
        if (compilation_units.get_current_objstack().is_in_global_scope())
            if (compilation_units.get_current_typestack().current_scope.size()) {
                if (nspace.size())
                    nspace = compilation_units.get_current_typestack().current_scope + "." + nspace;
                else
                    nspace = compilation_units.get_current_typestack().current_scope;
            }

        od->nspace = nspace;
        full_name = (nspace.size() ? nspace + ".": "") + var_name;
        
        /* Only filescope variables need mangling. TODO: Static variables */
        if (!clinkage && compilation_units.get_current_objstack().is_in_global_scope()) {
            mangled_name = mangle_emc_var_name( var_name, 
                                                nspace);
            od->mangled_name = mangled_name;
        /* Local variables and c-linkage variables have the same mangled name as their name */
        } else {
            od->mangled_name = mangled_name = var_name;
        }

        compilation_units.get_current_objstack().get_top_scope().push_object(od);

        if (value_node)
            value_node->resolve();

        value_type = type;
        /* If the rh node is a constant expression, resolve its value */
        if (value_node && value_node->value_type.is_const_expr) {
            auto child_obj = value_node->resolve_value();
            verify_obj_fits_in_type(child_obj, value_type);
        }
        return value_type;
    }

    emc_type resolve_no_push()
    {
        /* Resolve the type node */
        emc_type type = typedotchain->resolve();

        /* Resolve the name node with a potential namespace */
        ast_node_typedotnamechain* typedotnamechain_T = 
            dynamic_cast<ast_node_typedotnamechain*>(typedotnamechain);
        DEBUG_ASSERT_NOTNULL(typedotnamechain_T);
        typedotnamechain_T->resolve();
        var_name = typedotnamechain_T->name;
        nspace = typedotnamechain_T->nspace;

        /* Set how many pointer indirections this var def has */
        if (ptrdef_node) {
            ptrdef_node->resolve();
            auto ptrdef_node_t = dynamic_cast<ast_node_ptrdef_list*>(ptrdef_node);
            DEBUG_ASSERT_NOTNULL(ptrdef_node_t);
            n_pointer_indirections = (int)ptrdef_node_t->v_const.size();
            type.n_pointer_indirections = n_pointer_indirections;
        }

        if (value_node)
            value_node->resolve();
        return value_type = type;
    }
};

class ast_node_dotop: public ast_node {
public:
    ast_node_dotop(ast_node *first, std::string field_name) :
            first(first), field_name(field_name)
    {
        type = ast_type::DOTOPERATOR;
    }
    ~ast_node_dotop()
    {
        delete first;
    }
    
    ast_node *first;
    std::string field_name;

    emc_type resolve()
    {
        

        first->resolve();

        auto struct_type = first->value_type;
        
        if (!struct_type.is_struct())
            THROW_USER_ERROR_LOC("Dot operator on non struct");

        return value_type = struct_type.find_type_of_child(field_name);
    }
};



class ast_node_struct_def: public ast_node {
public:
    ast_node_struct_def()
    {
        type = ast_type::STRUCT;
    }

    ~ast_node_struct_def() 
    {
        for (auto e : v_fields)
            delete e;
    }

    std::vector<ast_node_def*> v_fields;

    void append_field(ast_node *node)
    {
        DEBUG_ASSERT_NOTNULL(node);
        DEBUG_ASSERT(node->type == ast_type::DEF, "Wrong node type");
        auto field = dynamic_cast<ast_node_def*>(node);
        DEBUG_ASSERT_NOTNULL(field);
        
        v_fields.push_back(field);
    }

    emc_type resolve()
    {
        for (auto e : v_fields)
            e->resolve_no_push();

        auto t = emc_type{emc_types::STRUCT};
        for (auto e : v_fields) {
            t.children_types.push_back(e->value_type);
            t.children_types.back().name = e->var_name; /* def nodes does not store var name in type usually*/
        }

        //resolve_scope.push_type(t); //TODO: Need to be in TYPE's ast_node

        return value_type = t;
    }
};


class ast_node_type: public ast_node {
public:
    ast_node_type(ast_node* typedotchain, ast_node *first) :
        first(first), typedotchain(typedotchain)
    {
        type = ast_type::TYPE;
    }

    ~ast_node_type() { delete first; delete typedotchain;}

    ast_node *first;
    ast_node* typedotchain;
    std::string type_name;
    std::string nspace; /* Relative namespace */
    std::string full_relative_name;
    std::string mangled_name;

    emc_type resolve()
    {
        auto type_node = dynamic_cast<ast_node_typedotchain*>(typedotchain);
        DEBUG_ASSERT_NOTNULL(type_node);
        /* We prepend the current namespace scope to the type (it is stored
           with absolute namespace). */
        nspace = compilation_units.get_current_typestack().current_scope;
        
        type_node->resolve_name_and_namespace(&type_name, &nspace);
        if (nspace.size())
            full_relative_name = nspace + "." + type_name;
        else
            full_relative_name = type_name;
        
        mangled_name = mangle_emc_type_name(full_relative_name);
        
        first->resolve(); 

        auto struct_node = dynamic_cast<ast_node_struct_def*>(first);
        /* We need to supply the value type with what the struct is 
         * is called from here. */
        struct_node->value_type.name = type_name;
        struct_node->value_type.mangled_name = mangled_name;
        compilation_units.get_current_typestack().push_type(full_relative_name, struct_node->value_type);

        return value_type = emc_type{emc_types::NONE};
    }

    ast_node* clone()
    {
        auto c = new ast_node_type { typedotchain->clone(), first->clone() };
        c->value_type = value_type;
        c->type_name = type_name;
        c->nspace = nspace;
        c->full_relative_name = full_relative_name;
        c->mangled_name = mangled_name;
        return c;        
    }
};

class ast_node_listlit: public ast_node {
public:
    ~ast_node_listlit()
    {
        if (first) delete first;
    }
    ast_node_listlit(ast_node *first)
    :
            first(first)
    {
        type = ast_type::LISTLITERAL;
    }
    
    ast_node *first;

    emc_type resolve()
    {
        first->resolve();
        value_type = emc_type{emc_types::LISTLIT};

        /* Populate the type with the children's types in order */
        if (first->type == ast_type::ARGUMENT_LIST) {
            auto arg_list = dynamic_cast<ast_node_arglist*>(first);
            DEBUG_ASSERT_NOTNULL(arg_list);
            for (ast_node* node : arg_list->v_ast_args) {
                value_type.children_types.push_back(node->value_type);
                if (node->value_type.is_const_expr)
                    node->resolve_value();
            }
        } else
            THROW_BUG("Child first is not an arglist");

        return value_type;
    }
};

class ast_node_nspace: public ast_node {
public:
    ~ast_node_nspace()
    {
        delete typedotchain;
    }
    ast_node_nspace(ast_node *typedotchain)
    :
            typedotchain(typedotchain)
    {
        type = ast_type::NAMESPACE;
    }
    
    ast_node *typedotchain;

    ast_node* clone()
    {
        auto c = new ast_node_nspace { typedotchain->clone()};
        c->value_type = value_type;
        return c;
    }

    emc_type resolve()
    {
        auto typedotchain_T = dynamic_cast<ast_node_typedotchain*>(typedotchain);
        DEBUG_ASSERT_NOTNULL(typedotchain_T);

        std::string full_nspace = typedotchain_T->resolve_full_type_name();
        
        compilation_units.get_current_typestack().set_scopes(full_nspace);

        return value_type = emc_type{emc_types::NONE};
    }
};

void push_dummyobject_to_resolve_scope(std::string var_name, emc_type type);

