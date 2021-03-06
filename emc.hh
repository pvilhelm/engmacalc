/*
 * object-typerna borde ha en "convert_to(std::string)" virtuel function
 * för att konverta till en viss typ
 *
 * typer borde inte ligga i scope utan i namespaces ... för att ha stöd för
 * lokala typer
 * 
 * Borde finnas en local resolve och en global resolve per ast_node
 * för att det inte ska bli så mkt mek ...
 */

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

#include "emc_assert.hh"

#ifndef NDEBUG
extern int ast_node_count;
extern int value_expr_count;
#endif


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
    FUNCTION_DECLARATION,
    FUNCTION_DEFINITION,
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
    STRUCT
};

enum class object_type {
    DOUBLE = 1,
    FUNC,
    STATIC_CFUNC,
    STRING,
    INT,
    UINT,
    BYTE,
    SHORT,
    USHORT,
    FLOAT,
    LONG,
    ULONG
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
    ULONG
};

struct emc_type {
    emc_type() {}
    /* First element is inner element. I.e.
     * c type "const int *" => POINTER, CONST, INT
     */
    emc_type(std::initializer_list<emc_types> l)
    {
        for (auto e : l)
            types.push_back(e);
    }
  
    bool is_valid()
    {
        if (types.size() && types[0] != emc_types::INVALID) return true;
        return false;
    }

    /* TODO: Fugly. Should be direct lookup here and in jit::emc_type_to_jit_type() */
    bool is_double()
    {
        if (types.size() && types[0] == emc_types::DOUBLE) return true;
        return false;
    }
    bool is_int()
    {
        if (types.size() && types[0] == emc_types::INT) return true;
        return false;
    }
    bool is_short()
    {
        if (types.size() && types[0] == emc_types::SHORT) return true;
        return false;
    }
    bool is_sbyte()
    {
        if (types.size() && types[0] == emc_types::SBYTE) return true;
        return false;
    }
    bool is_byte()
    {
        if (types.size() && types[0] == emc_types::BYTE) return true;
        return false;
    }
    bool is_uint()
    {
        if (types.size() && types[0] == emc_types::UINT) return true;
        return false;
    }
    bool is_ushort()
    {
        if (types.size() && types[0] == emc_types::USHORT) return true;
        return false;
    }
    bool is_long()
    {
        if (types.size() && types[0] == emc_types::LONG) return true;
        return false;
    }
    bool is_ulong()
    {
        if (types.size() && types[0] == emc_types::ULONG) return true;
        return false;
    }
    bool is_float()
    {
        if (types.size() && types[0] == emc_types::FLOAT) return true;
        return false;
    }
    bool is_void()
    {
        if (types.size() && types[0] == emc_types::NONE) return true;
        return false;
    }
    bool is_bool()
    {
        if (types.size() && types[0] == emc_types::BOOL) return true;
        return false;
    }
    bool is_struct()
    {
        if (types.size() && types[0] == emc_types::STRUCT) return true;
        return false;
    }
    /* For structs etc with children types. */
    std::vector<emc_type> children_types;
    
    /* TODO: Should be flags for const etc instead? Do a "root type" and then have
             vector only for structs etc. */
    std::vector<emc_types> types;
};

emc_type standard_type_promotion(const emc_type &a, const emc_type &b);
emc_type standard_type_promotion_or_invalid(const emc_type &a, const emc_type &b);
emc_type string_to_type(std::string);
/* Forward declarations. */
class scope;
class scope_stack;
class ast_node;
class obj;

/* Functions */
void init_linked_cfunctions();
void init_standard_variables();
void init_builtin_functions();
void init_builtin_types();

/* Util functions */
void deescape_string(std::string &s);

class obj {
public:
    virtual ~obj()
    {
    }
    obj(){}
    obj(const obj&) = delete;
    obj& operator=(const obj&) = delete;

    virtual emc_type resolve() {return emc_type{emc_types::INVALID};}
    virtual void debug_print()
    {
        std::cout << "Object, Namespace: " << nspace <<
                "Name: " << name << " Type: " << (int)type << std::endl;
    }
    std::string name;
    std::string nspace;
    object_type type;
};

class type_object {
public:
    virtual ~type_object()
    {
    }

    virtual obj* ctor() = 0;
    virtual void debug_print()
    {
        std::cout << "Type object, Name: " << name << std::endl;
    }
    std::string name;
};

class scope;
class scope_stack;

/*
class scopes {
public:
    scope_stack runtime_scopestack;
    scope_stack resolve_scopestack;
};
*/

class scope_stack {
public:
    /* There is always one root scope in the scope stack. */
    scope_stack() {push_new_scope();}
    scope_stack(const scope_stack&) = delete;
    scope_stack operator=(const scope_stack&) = delete;

    void push_new_scope();

    void pop_scope()
    {
        DEBUG_ASSERT(vec_scope.size() >= 1, 
            "Trying to pop scope_stack too far");
        vec_scope.pop_back();
    }

    scope& get_top_scope()
    {
        DEBUG_ASSERT(vec_scope.size() >= 1, 
            "Trying to access scope in empty scope_stack");
        return vec_scope.back();
    }

    scope& get_global_scope()
    {
        DEBUG_ASSERT(vec_scope.size() >= 1, 
            "Trying to access scope in empty scope_stack");
        return vec_scope.front();
    }

    obj* find_object(std::string name, std::string nspace);
    emc_type find_type(std::string name);

    void push_type(std::string name, emc_type type)
    {
        auto it = map_typename_to_type.find(name);
        if (it != map_typename_to_type.end())
            throw std::runtime_error("Type " + name + " in map already");
        map_typename_to_type[name] = type;
    }
    std::vector<scope> vec_scope;

    std::map<std::string, emc_type> map_typename_to_type;

    void debug_print();

    void clear();
};

class scope {
public:
    ~scope()
    { /* TODO: Fix så att inte minnet läcker ... */
        for (auto p : vec_objs)
            delete p;
        for (auto p : vec_types)
            delete p;
    }
    scope(){}
    scope(scope &&s) 
    {
        std::swap(vec_objs, s.vec_objs);
        std::swap(vec_types, s.vec_types);
    }
    /* Since this objects keeps track of pointers there can
     * be no copy or assignment ctor.
     */ 
    scope(const scope&) = delete;
    scope operator=(const scope&) = delete;

    std::vector<obj*> vec_objs;
    std::vector<type_object*> vec_types;

    void push_object(obj *obj)
    {
        if (find_object(obj->name, obj->nspace))
            throw std::runtime_error("push_object: Pushing existing object:"
                    + obj->nspace + obj->name);
        vec_objs.push_back(obj);
    }

    void pop_object()
    {
        vec_objs.pop_back();
    }

    obj* find_object(std::string name, std::string nspace)
    {
        for (auto p : vec_objs)
            if (p->name == name && p->nspace == nspace)
                return p;
        return nullptr;
    }

    void clear()
    {
        for (auto p : vec_objs)
            delete p;
        vec_objs.clear();
        for (auto p : vec_types)
            delete p;
        vec_types.clear();
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
    classname(std::string name, std::string nspace, c_type val)\
    :\
            val(val)\
    {\
        type = object_type;\
        this->name = name;\
        this->nspace = nspace;\
    }\
    ;\
    classname(std::string name, c_type val) :\
            classname(name, "", val)\
    {\
    }\
    classname(c_type val) :\
            classname("", "", val)\
    {\
    }\
    classname() :\
            classname("", "", (c_type){})\
    {\
    }\
\
    c_type val;\
\
    emc_type resolve()\
    {\
        return emc_ret_type;\
    }\
};\

/* TODO: Architecture independent types ie. uint8_t etc */
OBJCLASS_DEF(object_string, object_type::STRING, emc_type{emc_types::STRING}, std::string)
OBJCLASS_DEF(object_double, object_type::DOUBLE, emc_type{emc_types::DOUBLE}, double)
OBJCLASS_DEF(object_int, object_type::INT, emc_type{emc_types::INT}, int32_t)
OBJCLASS_DEF(object_uint, object_type::UINT, emc_type{emc_types::UINT}, uint32_t)
OBJCLASS_DEF(object_short, object_type::SHORT, emc_type{emc_types::SHORT}, int16_t)
OBJCLASS_DEF(object_ushort, object_type::USHORT, emc_type{emc_types::USHORT}, uint16_t)

OBJCLASS_DEF(object_long, object_type::LONG, emc_type{emc_types::LONG}, int64_t)
OBJCLASS_DEF(object_ulong, object_type::ULONG, emc_type{emc_types::ULONG}, uint64_t)


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
    }
    
    ast_node(const ast_node&) = delete;
    ast_node& operator=(const ast_node&) = delete;

    virtual ast_node* clone() = 0;
    virtual emc_type resolve() = 0;
    ast_type type = ast_type::INVALID;
    emc_type value_type = emc_type{emc_types::INVALID};
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

    ast_node* clone()
    {
        auto c = new ast_node_return { first ? first->clone() : 0 };
        c->value_type = value_type;
        return c;
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
        d = stof(s);
    }
    ~ast_node_double_literal()
    {
    }

    double d;

    emc_type resolve()
    {
        return value_type = emc_type{emc_types::DOUBLE};
    }

    ast_node* clone()
    {
        auto c = new ast_node_double_literal { d };
        c->value_type = value_type;
        return c;
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

        if (l > std::numeric_limits<int>::max() ||
            l < std::numeric_limits<int>::min())
            throw std::runtime_error("Value out of Int range: " + s);
        if (errno == ERANGE)
            throw std::runtime_error("String not an valid Int: " + s);
        i = (int)l;
    }
    ~ast_node_int_literal()
    {
    }

    int i;

    emc_type resolve()
    {
        return value_type = emc_type{emc_types::INT};
    }

    ast_node* clone()
    {
        auto c = new ast_node_int_literal { i };
        c->value_type = value_type;
        return c;
    }
};

class ast_node_string_literal: public ast_node {
public:
    ast_node_string_literal(std::string s) :
            s(s)
    {
        type = ast_type::STRING_LITERAL;
    }
    ~ast_node_string_literal()
    {
    }

    std::string s;

    emc_type resolve()
    {
        return value_type = emc_type{emc_types::STRING};
    }

    ast_node* clone()
    {
        auto c = new ast_node_string_literal { s };
        c->value_type = value_type;
        return c;
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

    ast_node* clone()
    {
        auto c = new ast_node_and { first->clone(), sec->clone() };
        c->value_type = value_type;
        return c;
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

    ast_node* clone()
    {
        auto c = new ast_node_or { first->clone(), sec->clone() };
        c->value_type = value_type;
        return c;
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

    ast_node* clone()
    {
        auto c = new ast_node_xor { first->clone(), sec->clone() };
        c->value_type = value_type;
        return c;
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

    ast_node* clone()
    {
        auto c = new ast_node_nor { first->clone(), sec->clone() };
        c->value_type = value_type;
        return c;
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

    ast_node* clone()
    {
        auto c = new ast_node_nand { first->clone(), sec->clone() };
        c->value_type = value_type;
        return c;
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

    ast_node* clone()
    {
        auto c = new ast_node_xnor { first->clone(), sec->clone() };
        c->value_type = value_type;
        return c;
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

    ast_node* clone()
    {
        auto c = new ast_node_not { first->clone() };
        c->value_type = value_type;
        return c;
    }
};

class ast_node_add: public ast_node {
public:
    ast_node_add() :
            ast_node_add(nullptr, nullptr)
    {
    }
    ast_node_add(ast_node *first, ast_node *sec) :
            first(first), sec(sec)
    {
        type = ast_type::ADD;
    }

    ~ast_node_add()
    {
        delete first;
        delete sec;
    }

    ast_node *first;
    ast_node *sec;

    emc_type resolve()
    {
        return value_type = standard_type_promotion(first->resolve(), sec->resolve());
    }

    ast_node* clone()
    {
        auto c = new ast_node_add { first->clone(), sec->clone() };
        c->value_type = value_type;
        return c;
    }
};

class ast_node_sub: public ast_node {
public:
    ast_node_sub() :
            ast_node_sub(nullptr, nullptr)
    {
    }
    ast_node_sub(ast_node *first, ast_node *sec) :
            first(first), sec(sec)
    {
        type = ast_type::SUB;
    }

    ~ast_node_sub()
    {
        delete first;
        delete sec;
    }

    ast_node *first;
    ast_node *sec;

    ast_node* clone()
    {
        auto c = new ast_node_sub { first->clone(), sec->clone() };
        c->value_type = value_type;
        return c;
    }

    emc_type resolve()
    {
        return value_type = standard_type_promotion(first->resolve(), sec->resolve());
    }
};

class ast_node_mul: public ast_node {
public:
    ast_node_mul() :
            ast_node_mul(nullptr, nullptr)
    {
    }
    ast_node_mul(ast_node *first, ast_node *sec) :
            first(first), sec(sec)
    {
        type = ast_type::MUL;
    }

    ~ast_node_mul()
    {
        if (first) delete first;
        if (sec) delete sec;
    }
    
    ast_node *first;
    ast_node *sec;

    emc_type resolve()
    {
        return value_type = standard_type_promotion(first->resolve(), sec->resolve());
    }

    ast_node* clone()
    {
        auto c = new ast_node_mul { first->clone(), sec->clone() };
        c->value_type = value_type;
        return c;
    }
};

class ast_node_rdiv: public ast_node {
public:
    ast_node_rdiv() :
            ast_node_rdiv(nullptr, nullptr)
    {
    }
    ast_node_rdiv(ast_node *first, ast_node *sec) :
            first(first), sec(sec)
    {
        type = ast_type::RDIV;
    }

    ~ast_node_rdiv()
    {
        if (first) delete first;
        if (sec) delete sec;
    }
    
    ast_node *first;
    ast_node *sec;

    emc_type resolve()
    {
        return value_type = standard_type_promotion(first->resolve(), sec->resolve());
    }

    ast_node* clone()
    {
        auto c = new ast_node_rdiv { first->clone(), sec->clone() };
        c->value_type = value_type;
        return c;
    }
};

class ast_node_pow: public ast_node {
public:
    ast_node_pow() :
            ast_node_pow(nullptr, nullptr)
    {
    }
    ast_node_pow(ast_node *first, ast_node *sec) :
            first(first), sec(sec)
    {
        type = ast_type::POW;
    }

    ~ast_node_pow()
    {
        if (first) delete first;
        if (sec) delete sec;
    }
    
    ast_node *first;
    ast_node *sec;

    ast_node* clone()
    {
        auto c = new ast_node_pow { first->clone(), sec->clone() };
        c->value_type = value_type;
        return c;
    }

    emc_type resolve()
    {
        return value_type = standard_type_promotion(first->resolve(), sec->resolve());
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

    ast_node* clone()
    {
        auto c = new ast_node_abs { first->clone() };
        c->value_type = value_type;
        return c;
    }

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

    ast_node* clone()
    {
        auto c = new ast_node_uminus { first->clone() };
        c->value_type = value_type;
        return c;
    }

    emc_type resolve()
    {
        return value_type = first->resolve();
    }
};

class ast_node_var: public ast_node {
public:
    std::string name;
    std::string nspace;

    ~ast_node_var()
    {
    }
    ast_node_var(std::string name, std::string nspace = "")
    :
            name(name), nspace(nspace)
    {
        type = ast_type::VAR;
    }
    ast_node_var() :
            ast_node_var("", "")
    {
    }

    emc_type resolve()
    {
        extern scope_stack resolve_scope;
        auto p = resolve_scope.find_object(name, nspace);
        if (!p) /* TODO: Kanske borde throwa "inte hittat än" för att kunna fortsätta? */
            throw std::runtime_error("Object does not exist: >>" +
                    nspace + name + "<<");
        return value_type = p->resolve();
    }

    ast_node* clone()
    {
        auto c = new ast_node_var { name, nspace };
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

    ast_node* clone()
    {
        auto c = new ast_node_assign { first->clone(), sec->clone() };
        c->value_type = value_type;
        return c;
    }

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

    ast_node* clone()
    {
        auto c = new ast_node_cmp { first->clone(), sec->clone() };
        c->value_type = value_type;
        return c;
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
            if (t.types[0] == emc_types::NONE && et.types[0] != emc_types::NONE)
                t = et;
        }
        return value_type = t;
    }

    ast_node* clone()
    {
        /* TODO: Fixa så att ctorn inte tar first och att alla element läggs till med append_node */
        auto iter = v_nodes.begin();
        if (iter == v_nodes.end())
            throw std::runtime_error("Bugg ast_node_explist");

        auto pel = new ast_node_explist { (*iter)->clone() };
        pel->value_type = value_type;
        pel->v_nodes[0]->value_type = (*iter++)->value_type;

        for (; iter != v_nodes.end(); iter++) {
            auto c = (*iter)->clone();
            c->value_type = (*iter)->value_type;
            pel->append_node(c);
        }
        return pel;
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
        extern scope_stack resolve_scope;
        resolve_scope.push_new_scope();
        value_type = first->resolve();
        resolve_scope.pop_scope();
        return value_type;
    }

    ast_node* clone()
    {
        auto c = new ast_node_doblock { first->clone() };
        c->value_type = value_type;
        return c;
    }
};



class ast_node_if: public ast_node {
public:
    ast_node_if(ast_node *cond_e, ast_node *if_el) :
            ast_node_if(cond_e, if_el, nullptr)
    {
    }
    ast_node_if(ast_node *cond_e, ast_node *if_el, ast_node *else_el) :
                ast_node_if(cond_e, if_el, else_el, nullptr) {}
    ast_node_if(ast_node *cond_e, ast_node *if_el, ast_node *else_el, ast_node *also_el) :
            cond_e(cond_e), if_el(if_el), else_el(else_el), also_el(also_el)
    {
        type = ast_type::IF;
    }
    ~ast_node_if()
    {
        delete cond_e;
        delete if_el;
        delete else_el;
        delete also_el;
    }
    
    ast_node *cond_e;
    ast_node *if_el;
    ast_node *else_el; /* else_el can be a linked IF or another expression (for IF ELSE ... ) */
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
        if (!else_el && !also_el) {
            extern scope_stack resolve_scope;
            resolve_scope.push_new_scope();
            value_type = if_el->resolve();
            resolve_scope.pop_scope();
            return value_type;
        } else if (else_el && !also_el) {
            extern scope_stack resolve_scope;
            resolve_scope.push_new_scope();
            auto value_type_if = if_el->resolve();
            resolve_scope.pop_scope();
            resolve_scope.push_new_scope();
            auto value_type_else = else_el->resolve();
            resolve_scope.pop_scope();

            auto t = standard_type_promotion_or_invalid(value_type_if, value_type_else);
            if (t.is_valid())
                return value_type = t;
            else
                return value_type = emc_type{emc_types::NONE};
        } else {
            extern scope_stack resolve_scope;
            resolve_scope.push_new_scope();
            auto value_type_if = if_el->resolve();
            resolve_scope.pop_scope();
            resolve_scope.push_new_scope();
            auto value_type_else = else_el->resolve();
            resolve_scope.pop_scope();
            resolve_scope.push_new_scope();
            auto value_type_also = also_el->resolve();
            resolve_scope.pop_scope();

            auto t = standard_type_promotion_or_invalid(value_type_if, value_type_else);
            t = standard_type_promotion_or_invalid(t, value_type_also);
            if (t.is_valid())
                return value_type = t;
            else
                return value_type = emc_type{emc_types::NONE};
        }
    }

    void append_linked_if(ast_node *node)
    {
        auto p = &else_el;
        while(*p)
            p = &(dynamic_cast<ast_node_if*>(*p)->else_el);
        *p = node;
    }

    ast_node* clone()
    {
        auto c = new ast_node_if { cond_e->clone(), if_el->clone(),
                else_el ? else_el->clone() : nullptr };
        c->value_type = value_type;
        return c;        
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

    ast_node* clone()
    {
        auto c = new ast_node_while { cond_e->clone(), if_el->clone(),
                else_el ? else_el->clone() : nullptr };
        c->value_type = value_type;
        return c;        
    }

    emc_type resolve()
    {
        cond_e->resolve();
        if (!else_el) {
            extern scope_stack resolve_scope;
            resolve_scope.push_new_scope();
            value_type = if_el->resolve();
            resolve_scope.pop_scope();
            return value_type;
        } else {
            extern scope_stack resolve_scope;
            resolve_scope.push_new_scope();
            auto value_type_if = if_el->resolve();
            resolve_scope.pop_scope();
            resolve_scope.push_new_scope();
            auto value_type_else = else_el->resolve();
            resolve_scope.pop_scope();

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
            std::string name, std::string nspace, ast_node *return_list)
    :
            parlist(parlist), code_block(code_block),
                    name(name), nspace(nspace), return_list(return_list)
    {
        type = ast_type::FUNCTION_DECLARATION;
        
        if (!parlist)
            this->parlist = new ast_node_vardef_list{};   
        if (!return_list)
            this->return_list = new ast_node_vardef_list{};  
    }
    /* fobj äger ast_noderna kanske är dumt? */
    ~ast_node_funcdef()
    {
        delete code_block;
        delete parlist;
        delete return_list;
    }
    
    ast_node *return_list;
    ast_node *code_block;
    ast_node *parlist; /* Is a return_list */
    std::string name;
    std::string nspace;

    emc_type resolve();

    ast_node* clone()
    {
        auto c =  new ast_node_funcdef { parlist->clone(),
                code_block->clone(),
                name, nspace, return_list->clone() };
        c->value_type = value_type;
        return c;
    }
};

class ast_node_funccall: public ast_node {
public:
    ast_node_funccall(std::string nspace, std::string name, ast_node *arg_list) :
            nspace(nspace), arg_list(arg_list), name(name)
    {
        type = ast_type::FUNCTION_CALL;
        if (!arg_list) 
            this->arg_list = new ast_node_arglist{}; /* Make empty dummy arg list for no arg fn:s. */
    }

    ~ast_node_funccall()
    {
        delete arg_list;
    }

    std::string name;
    std::string nspace;
    ast_node *arg_list;

    ast_node* clone()
    {
        auto c =  new ast_node_funccall { nspace,
                name,
                arg_list->clone() };
        c->value_type = value_type;
        return c;
    }

    emc_type resolve()
    {
        arg_list->resolve();
        extern scope_stack resolve_scope;
        auto obj = resolve_scope.find_object(name, nspace);
        if (!obj) /* TODO: Kolla så fn */
            throw std::runtime_error(
                    "Could not find function " + nspace + " " + name);
        return value_type = obj->resolve();
    }
};

class ast_node_funcdec: public ast_node {
public:
    ast_node_funcdec(ast_node *parlist, 
            std::string name, std::string nspace, ast_node *return_list)
    :
            parlist(parlist),
                    name(name), nspace(nspace), return_list(return_list)
    {
        type = ast_type::FUNCTION_DEFINITION;
        
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
    }
    
    ast_node *return_list;
    ast_node *parlist; /* Is a return_list */
    std::string name;
    std::string nspace;

    emc_type resolve();

    ast_node* clone()
    {
        auto c =  new ast_node_funcdec { parlist->clone(),
                name, nspace, return_list->clone() };
        c->value_type = value_type;
        return c;
    }
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

    ast_node* clone()
    {
        auto p = new ast_node_andchain {};
        p->value_type = value_type;
        if (v_children.size() >= 1)
            p->v_children.push_back(dynamic_cast<ast_node_chainable*>(v_children.front()->clone()));

        for (int i = 1; i < v_children.size(); i++)
            p->append_next(dynamic_cast<ast_node_chainable*>(v_children[i]->clone()));
        return p;
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

    ast_node* clone()
    {
        auto c = new ast_node_les { first->clone(), sec->clone() };
        c->value_type = value_type;
        return c;
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
    ast_node* clone()
    {
        auto c = new ast_node_gre { first->clone(), sec->clone() };
        c->value_type = value_type;
        return c;
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
        first->resolve(); sec->resolve();
        return value_type = emc_type{emc_types::INT};
    }
    ast_node* clone()
    {
        auto c = new ast_node_equ { first->clone(), sec->clone() };
        c->value_type = value_type;
        return c;
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
    ast_node* clone()
    {
        auto c = new ast_node_leq { first->clone(), sec->clone() };
        c->value_type = value_type;
        return c;
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
    ast_node* clone()
    {
        auto c = new ast_node_geq { first->clone(), sec->clone() };
        c->value_type = value_type;
        return c;
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

    ast_node* clone()
    {
        auto c = new ast_node_neq { first->clone(), sec->clone() };
        c->value_type = value_type;
        return c;
    }
};

class ast_node_def: public ast_node {
public:
    ast_node_def(std::string type_name, std::string var_name,
            ast_node *value_node) :
            type_name(type_name), var_name(var_name), value_node(value_node)
    {
        type = ast_type::DEF;
    }

    ~ast_node_def()
    {
        if (value_node) delete value_node;
    }
    
    std::string type_name; /* TODO: Borde va ngt object istället. */
    std::string var_name;
    ast_node *value_node = nullptr;

    ast_node* clone()
    {
        auto c = new ast_node_def { type_name, var_name,
                value_node ? value_node->clone() : nullptr };
        c->value_type = value_type;
        return c;        
    }

    emc_type resolve()
    {
        /* TODO: Typ implementationen är ful ... */
        extern scope_stack resolve_scope;

        emc_type type = resolve_scope.find_type(type_name);
 
        obj *od = nullptr;
        if (type.is_double())
            od = new object_double{var_name, 0};
        else if (type.is_int())
            od = new object_int{var_name, 0};
        else
            throw std::runtime_error("Type not implemented ast_node_def");
        resolve_scope.get_top_scope().push_object(od);
 

        if (value_node)
            value_node->resolve();
 
        return value_type = type;
    }

    emc_type resolve_no_push()
    {
        return value_type = string_to_type(type_name);
    }
};


class ast_node_type: public ast_node {
public:
    ast_node_type(std::string type_name, ast_node *first) :
        first(first), type_name(type_name)
    {
        type = ast_type::TYPE;
    }

    ~ast_node_type() { delete first; }

    ast_node *first;
    std::string type_name;

    emc_type resolve()
    {
        first->resolve(); 
        return value_type = emc_type{emc_types::NONE};
    }

    ast_node* clone()
    {
        auto c = new ast_node_type { type_name, first->clone() };
        c->value_type = value_type;
        return c;        
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
        extern scope_stack resolve_scope;

        for (auto e : v_fields)
            e->resolve_no_push();

        auto t = emc_type{emc_types::STRUCT};
        for (auto e : v_fields)
            t.children_types.push_back(e->value_type);

        //resolve_scope.push_type(t); //TODO: Need to be in TYPE's ast_node

        return value_type = t;
    }

    ast_node* clone()
    {
        auto c = new ast_node_struct_def {};
        c->value_type = value_type;
        return c;        
    }
};