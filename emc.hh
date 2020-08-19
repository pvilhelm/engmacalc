/*
 * object-typerna borde ha en "convert_to(std::string)" virtuel function
 * för att konverta till en viss typ
 *
 * typer borde inte ligga i scope utan i namespaces ... för att ha stöd för
 * lokala typer
 */

#pragma once

#include <stdexcept>
#include <string>
#include <vector>
#include <iostream>
#include <cmath>
#include <memory>

#include "emc_assert.hh"

enum class ast_type {
    INVALID = 0,
    DOUBLE_LITERAL = 1,
    STRING_LITERAL,
    ADD,
    DOUBLE_SUB,
    DOUBLE_MUL,
    DOUBLE_RDIV,
    DOUBLE_UMINUS,
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
    FUNCTION_CALL,
    ANDCHAIN, /* Dummy node for chained comparations. */
    WHILE,
    DEF,
};

enum class value_type {
    DOUBLE = 1,
    RVAL,
    LVAL,
    LIST,
    STRING,
    INT
};

enum class object_type {
    DOUBLE = 1,
    FUNC,
    STATIC_CFUNC,
    STRING,
    INT
};

enum class emc_types {
    INVALID,
    NONE, /* The node have no type. */
    POINTER,
    INT,
    DOUBLE,
    STRING,
    VOID,
    CONST,
    REFERENCE, /* e.g. REFERENCE, INT (reference to an int object) */
    FUNCTION,
    RETURNING,
};

struct emc_type {
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
    bool is_double()
    {
        if (types.size() && types[0] != emc_types::DOUBLE) return true;
        return false;
    }
    bool is_int()
    {
        if (types.size() && types[0] != emc_types::INT) return true;
        return false;
    }

    std::vector<emc_types> types;
};

emc_type standard_type_promotion(const emc_type &a, const emc_type &b);
emc_type standard_type_promotion_or_invalid(const emc_type &a, const emc_type &b);

/* Forward declarations. */
class scope;
class ast_node;
class expr_value;
class expr_value_double;
class expr_value_list;

using cfunc_callwrapper = expr_value* (*)(expr_value_list *arg_value_list);

/* Functions */
void init_linked_cfunctions();
void init_standard_variables();
void init_builtin_functions();
void init_builtin_types();

/* Util functions */
void deescape_string(std::string &s);
int cmp_helper(expr_value *fv, expr_value *sv);

class obj {
public:
    virtual ~obj()
    {
    }
    ;
    virtual expr_value* eval() = 0;
    virtual expr_value* assign(expr_value *val) = 0;
    virtual emc_type resolve() {return emc_type{emc_types::INVALID};}
    std::string name;
    std::string nspace;
    object_type type;
};

class type_object {
public:
    virtual ~type_object()
    {
    }
    ;

    virtual obj* ctor() = 0;
    virtual obj* ctor(expr_value *val) = 0; /* Copy constructor */

    std::string name;
};

class scope;

class scope_stack {
public:
    void push_new_scope();
    void pop_scope()
    {
        vec_scope.pop_back();
    }
    scope& get_top_scope()
    {
        return vec_scope.back();
    }
    obj* find_object(std::string name, std::string nspace);
    type_object* find_type(std::string name);
    std::vector<scope> vec_scope;
};

class scope {
public:
    ~scope()
    { /* TODO: Fix så att inte minnet läcker ... */
        // for (auto p : vec_objs)
        //  if(p) delete p;
    }
    void push_object(obj *obj)
    {
        if (find_object(obj->name, obj->nspace))
            throw std::runtime_error("push_object: Pushing existing object:"
                    + obj->nspace + obj->name);
        vec_objs.push_back(obj);
    }
    void push_type(type_object *type)
    {
        if (find_type(type->name))
            throw std::runtime_error("push_type: Pushing existing type:"
                    + type->name);
        vec_types.push_back(type);
    }
    void pop_object()
    {
        vec_objs.pop_back();
    }
    void pop_type()
    {
        vec_types.pop_back();
    }
    obj* find_object(std::string name, std::string nspace)
    {
        for (auto p : vec_objs)
            if (p->name == name && p->nspace == nspace)
                return p;
        return nullptr;
    }

    type_object* find_type(std::string name)
    {
        for (auto p : vec_types)
            if (p->name == name)
                return p;
        return nullptr;
    }
    std::vector<obj*> vec_objs;
    std::vector<type_object*> vec_types;
};

/* Base class of the value of an evaluated expression. */
class expr_value {
public:
    virtual ~expr_value()
    {
    }
    ;
    virtual expr_value* clone() = 0;
    value_type type;
};

class expr_value_list: public expr_value {
public:
    expr_value_list()
    {
        type = value_type::LIST;
    }
    ~expr_value_list()
    {
        for (auto p : v_val)
            delete p;
    }
    /* The expression value list takes ownership of the pointer. */
    void append_value(expr_value *val)
    {
        v_val.push_back(val);
    }

    expr_value* clone()
    {
        auto ret = new expr_value_list;
        for (auto &e : v_val)
            ret->v_val.push_back(e->clone());
        return ret;
    }
    std::vector<expr_value*> v_val;
};

class expr_value_double: public expr_value {
public:
    expr_value_double()
    {
        type = value_type::DOUBLE;
        d = 0;
    }
    ;
    expr_value_double(double dd)
    {
        type = value_type::DOUBLE;
        d = dd;
    }
    expr_value_double(std::string s)
    {
        type = value_type::DOUBLE;
        d = stod(s);
    }
    ~expr_value_double()
    {
    }

    double d;

    expr_value* clone()
    {
        return new expr_value_double { d };
    }
};

class expr_value_int: public expr_value {
public:
    expr_value_int()
    {
        type = value_type::INT;
        i = 0;
    }
    ;
    expr_value_int(int val)
    {
        type = value_type::INT;
        i = val;
    }
    expr_value_int(std::string s)
    {
        type = value_type::INT;
        i = stoi(s);
    }
    ~expr_value_int()
    {
    }

    int i;

    expr_value* clone()
    {
        return new expr_value_int { i };
    }
};

class expr_value_string: public expr_value {
public:
    expr_value_string()
    {
        type = value_type::STRING;
    }
    ;
    expr_value_string(std::string s) :
            s(s)
    {
        type = value_type::STRING;
    }
    ~expr_value_string()
    {
    }

    std::string s;

    expr_value* clone()
    {
        return new expr_value_string { s };
    }
};

class object_string: public obj {
public:
    ~object_string()
    {
    }
    ;

    object_string(std::string name, std::string nspace, std::string val)
    :
            s(val)
    {
        type = object_type::STRING;
        this->name = name;
        this->nspace = nspace;
    }
    ;
    object_string(std::string name, std::string val) :
            object_string(name, "", val)
    {
    }
    object_string(std::string val) :
            object_string("", "", val)
    {
    }
    object_string() :
            object_string("", "", "")
    {
    }

    expr_value* assign(expr_value *val)
    {
        if (val->type != value_type::STRING)
            throw std::runtime_error("Can't assign this type to string");
        expr_value_string *vals = dynamic_cast<expr_value_string*>(val);
        s = vals->s;

        return eval();
    }

    expr_value* eval()
    {
        return new expr_value_string { s };
    }

    std::string s;

    emc_type resolve()
    {
        return emc_type{emc_types::STRING};
    }
};

class object_double: public obj {
public:
    ~object_double()
    {
    }
    ;

    object_double(std::string name, std::string nspace, double val)
    :
            d(val)
    {
        type = object_type::DOUBLE;
        this->name = name;
        this->nspace = nspace;
    }
    ;
    object_double(std::string name, double val)
    :
            object_double(name, "", val)
    {
    }
    object_double(double val) :
            object_double("", "", val)
    {
    }
    object_double() :
            object_double("", "", 0.)
    {
    }

    expr_value* eval()
    {
        return new expr_value_double { d };
    }

    expr_value* assign(expr_value *val)
    {
        if (val->type != value_type::INT && val->type != value_type::DOUBLE)
            throw std::runtime_error("Can't assign this type to double");

        expr_value_string *valt;
        if (val->type == value_type::INT) {
            auto valt = dynamic_cast<expr_value_int*>(val);
            d = valt->i;
        } else if (val->type == value_type::DOUBLE) {
            auto valt = dynamic_cast<expr_value_double*>(val);
            d = valt->d;
        }

        return eval();
    }

    double d;

    emc_type resolve()
    {
        return emc_type{emc_types::DOUBLE};
    }
};

class object_int: public obj {
public:
    ~object_int()
    {
    }
    ;

    object_int(std::string name, std::string nspace, int val)
    :
            i(val)
    {
        type = object_type::INT;
        this->name = name;
        this->nspace = nspace;
    }
    ;
    object_int(std::string name, int val)
    :
            object_int(name, "", val)
    {
    }
    object_int(int val) :
            object_int("", "", val)
    {
    }
    object_int() :
            object_int("", "", 0)
    {
    }

    expr_value* eval()
    {
        return new expr_value_int { i };
    }

    expr_value* assign(expr_value *val)
    {
        if (val->type != value_type::INT && val->type != value_type::DOUBLE)
            throw std::runtime_error("Can't assign this type to int");

        expr_value_string *valt;
        if (val->type == value_type::INT) {
            auto valt = dynamic_cast<expr_value_int*>(val);
            i = valt->i;
        } else if (val->type == value_type::DOUBLE) {
            auto valt = dynamic_cast<expr_value_double*>(val);
            i = valt->d;
        }

        return eval();
    }

    int i;

    emc_type resolve()
    {
        return emc_type{emc_types::INT};
    }
};

class object_func_base: public obj {
public:
    virtual expr_value* feval(expr_value_list *arg_value_list) = 0;
};

class object_func: public object_func_base {
public:
    ~object_func()
    {
    }
    object_func(ast_node *root, std::string name,
            std::string nspace, ast_node *para_list)
    :
            root(root), para_list(para_list)
    {
        type = object_type::FUNC;
        this->name = name;
        this->nspace = nspace;
    }

    ast_node *root;
    ast_node *para_list;
    expr_value* eval()
    {
        throw std::runtime_error("func eval");
    }

    /* Evaluate the function. */
    expr_value* feval(expr_value_list *arg_value_list);

    expr_value* assign(expr_value *val)
    {
        throw std::runtime_error("Not implemented function pointers yet");
    }

    emc_type resolve()
    {
        return emc_type{emc_types::FUNCTION};
    }
};

class object_static_cfunc: public object_func_base {
public:
    ~object_static_cfunc()
    {
    }
    object_static_cfunc(std::string name, cfunc_callwrapper fn_ptr)
    :
            fn_ptr(fn_ptr)
    {
        type = object_type::STATIC_CFUNC;
        this->name = name;
    }

    cfunc_callwrapper fn_ptr = nullptr;

    /* Evaluate the function. */
    expr_value* feval(expr_value_list *arg_value_list)
    {
        return fn_ptr(arg_value_list);
    }

    expr_value* eval()
    {
        throw std::runtime_error("cfunc eval");
    }

    expr_value* assign(expr_value *val)
    {
        throw std::runtime_error("Not implemented function pointers yet");
    }

    emc_type resolve()
    {
        return emc_type{emc_types::FUNCTION};
    }
};

class type_double: public type_object {
public:
    ~type_double()
    {
    }
    type_double()
    {
        name = "Double";
    }
    obj* ctor()
    {
        return new object_double { };
    }
    obj* ctor(expr_value *val)
    {
        double d;
        if (val->type == value_type::DOUBLE) {
            d = dynamic_cast<expr_value_double*>(val)->d;
        } else if (val->type == value_type::INT) {
            d = (double) dynamic_cast<expr_value_int*>(val)->i;
        } else
            throw std::runtime_error("Invalid conversion");
        return new object_double { d };
    }
    ;
};

class type_int: public type_object {
public:
    ~type_int()
    {
    }
    type_int()
    {
        name = "Int";
    }
    obj* ctor()
    {
        return new object_int { };
    }
    obj* ctor(expr_value *val)
    {
        int i;
        if (val->type == value_type::DOUBLE) {
            i = (int) dynamic_cast<expr_value_double*>(val)->d;
        } else if (val->type == value_type::INT) {
            i = dynamic_cast<expr_value_int*>(val)->i;
        } else
            throw std::runtime_error("Invalid conversion");
        return new object_int { i };
    }
    ;
};

class type_string: public type_object {
public:
    ~type_string()
    {
    }
    type_string()
    {
        name = "String";
    }
    obj* ctor()
    {
        return new object_string { };
    }
    obj* ctor(expr_value *val)
    {
        std::string s;
        if (val->type == value_type::STRING) {
            s = dynamic_cast<expr_value_string*>(val)->s;
        } else
            throw std::runtime_error("Invalid conversion");
        return new object_string { s };
    }
    ;
};

class expr_value_lval: public expr_value {
public:
    /* Does not own object. */
    obj *object;

    expr_value_lval(obj *obj)
    :
            object(obj)
    {
        type = value_type::LVAL;
    }

    expr_value* clone()
    {
        return new expr_value_lval { object };
    }
};

/* Base class for a node in the abstract syntax tree. */
class ast_node {
public:
    virtual ~ast_node()
    {
    }
    ;

    virtual expr_value* eval() = 0;
    virtual ast_node* clone() = 0;
    virtual emc_type resolve() = 0;
    ast_type type;
    emc_type value_type = emc_type{emc_types::INVALID};
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

    expr_value* eval()
    {
        auto evd = new expr_value_double { d };

        return evd;
    }

    ast_node* clone()
    {
        return new ast_node_double_literal { d };
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

    expr_value* eval()
    {
        auto evd = new expr_value_string { s };

        return evd;
    }

    ast_node* clone()
    {
        return new ast_node_string_literal { s };
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

    emc_type resolve()
    {
        return value_type = standard_type_promotion(first->resolve(), sec->resolve());
    }

    ast_node* clone()
    {
        return new ast_node_add { first->clone(), sec->clone() };
    }

    expr_value* eval()
    {
        auto fv = first->eval();
        auto sv = sec->eval();

        if (fv->type == value_type::DOUBLE
                || sv->type == value_type::DOUBLE) {
            /* Swap so that we always get a double in one specific. */
            auto fv1 = fv->type == value_type::DOUBLE ? fv : sv;
            auto sv1 = fv->type == value_type::DOUBLE ? sv : fv;

            auto d_fv1 = dynamic_cast<expr_value_double*>(fv1);

            switch (sv1->type) {
            case value_type::DOUBLE:
                {
                    auto d_sv1 = dynamic_cast<expr_value_double*>(sv1);
                    double ans = d_fv1->d + d_sv1->d;
                    delete sv;
                    delete fv;
                    return new expr_value_double { ans };
                }
            case value_type::INT:
                {
                    auto i_sv1 = dynamic_cast<expr_value_int*>(sv1);
                    double ans = d_fv1->d + i_sv1->i;
                    delete sv;
                    delete fv;
                    return new expr_value_double { ans };
                }
            default:
                {
                    delete sv;
                    delete fv;
                    throw std::runtime_error(
                            "AST node add:  types not implemented");
                }
            }
        } else if (fv->type == value_type::INT
                || sv->type == value_type::INT) {
            /* Swap so that we always get a int in one specific. */
            auto fv1 = fv->type == value_type::INT ? fv : sv;
            auto sv1 = fv->type == value_type::INT ? sv : fv;

            auto d_fv1 = dynamic_cast<expr_value_int*>(fv1);

            switch (sv1->type) {
            case value_type::DOUBLE:
                {
                    auto d_sv1 = dynamic_cast<expr_value_double*>(sv1);
                    double ans = d_fv1->i + d_sv1->d;
                    delete sv;
                    delete fv;
                    return new expr_value_double { ans };
                }
            case value_type::INT:
                {
                    auto i_sv1 = dynamic_cast<expr_value_int*>(sv1);
                    int ans = d_fv1->i + i_sv1->i;
                    delete sv;
                    delete fv;
                    return new expr_value_int { ans };
                }
            default:
                {
                    delete sv;
                    delete fv;
                    throw std::runtime_error(
                            "AST node add:  types not implemented");
                }
            }
        } else if (fv->type == value_type::STRING
                && sv->type == value_type::STRING) {
            auto s_fv = dynamic_cast<expr_value_string*>(fv);
            auto s_sv = dynamic_cast<expr_value_string*>(sv);

            std::string ans = s_fv->s + s_sv->s;

            delete sv;
            delete fv;

            return new expr_value_string { ans };
        } else {
            delete sv;
            delete fv;
            throw std::runtime_error("AST node add:  types not implemented");
        }

    }

    ast_node *first;
    ast_node *sec;
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
        type = ast_type::DOUBLE_SUB;
    }

    ~ast_node_sub()
    {
        delete first;
        delete sec;
    }

    ast_node* clone()
    {
        return new ast_node_sub { first->clone(), sec->clone() };
    }

    emc_type resolve()
    {
        return value_type = standard_type_promotion(first->resolve(), sec->resolve());
    }

    expr_value* eval()
    {
        auto fv = first->eval();
        auto sv = sec->eval();

        if (fv->type == value_type::DOUBLE
                && sv->type == value_type::DOUBLE) {
            auto d_fv = dynamic_cast<expr_value_double*>(fv);
            auto d_sv = dynamic_cast<expr_value_double*>(sv);

            double ans = d_fv->d - d_sv->d;

            delete fv;
            delete sv;

            return new expr_value_double { ans };
        } else if (fv->type == value_type::DOUBLE
                && sv->type == value_type::INT) {
            auto d_fv = dynamic_cast<expr_value_double*>(fv);
            auto i_sv = dynamic_cast<expr_value_int*>(sv);

            double ans = d_fv->d - i_sv->i;

            delete fv;
            delete sv;

            return new expr_value_double { ans };
        } else if (fv->type == value_type::INT
                && sv->type == value_type::DOUBLE) {
            auto d_fv = dynamic_cast<expr_value_int*>(fv);
            auto d_sv = dynamic_cast<expr_value_double*>(sv);

            double ans = d_fv->i - d_sv->d;

            delete fv;
            delete sv;

            return new expr_value_double { ans };
        } else if (fv->type == value_type::INT
                && sv->type == value_type::INT) {
            auto d_fv = dynamic_cast<expr_value_int*>(fv);
            auto d_sv = dynamic_cast<expr_value_int*>(sv);

            int ans = d_fv->i - d_sv->i;

            delete fv;
            delete sv;

            return new expr_value_int { ans };
        } else {
            delete fv;
            delete sv;
            throw std::runtime_error("AST node sub:  types not implemented");
        }

    }

    ast_node *first;
    ast_node *sec;
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
        type = ast_type::DOUBLE_MUL;
    }

    ~ast_node_mul()
    {
        if (first) delete first;
        if (sec) delete sec;
    }

    emc_type resolve()
    {
        return value_type = standard_type_promotion(first->resolve(), sec->resolve());
    }

    ast_node* clone()
    {
        return new ast_node_mul { first->clone(), sec->clone() };
    }

    expr_value* eval()
    {
        auto fv = first->eval();
        auto sv = sec->eval();

        if (fv->type == value_type::DOUBLE
                && sv->type == value_type::DOUBLE) {
            auto d_fv = dynamic_cast<expr_value_double*>(fv);
            auto d_sv = dynamic_cast<expr_value_double*>(sv);

            double ans = d_fv->d * d_sv->d;

            delete fv;
            delete sv;

            return new expr_value_double { ans };
        } else if (fv->type == value_type::DOUBLE
                && sv->type == value_type::INT) {
            auto d_fv = dynamic_cast<expr_value_double*>(fv);
            auto i_sv = dynamic_cast<expr_value_int*>(sv);

            double ans = d_fv->d * i_sv->i;

            delete fv;
            delete sv;

            return new expr_value_double { ans };
        } else if (fv->type == value_type::INT
                && sv->type == value_type::DOUBLE) {
            auto i_fv = dynamic_cast<expr_value_int*>(fv);
            auto d_sv = dynamic_cast<expr_value_double*>(sv);

            double ans = i_fv->i * d_sv->d;

            delete fv;
            delete sv;

            return new expr_value_double { ans };
        } else if (fv->type == value_type::INT
                && sv->type == value_type::INT) {
            auto i_fv = dynamic_cast<expr_value_int*>(fv);
            auto i_sv = dynamic_cast<expr_value_int*>(sv);

            int ans = i_fv->i * i_sv->i;

            delete fv;
            delete sv;

            return new expr_value_int { ans };
        } else {
            delete fv;
            delete sv;
            throw std::runtime_error("AST node mul:  types not implemented");
        }

    }

    ast_node *first;
    ast_node *sec;
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
        type = ast_type::DOUBLE_RDIV;
    }

    ~ast_node_rdiv()
    {
        if (first) delete first;
        if (sec) delete sec;
    }

    emc_type resolve()
    {
        return value_type = standard_type_promotion(first->resolve(), sec->resolve());
    }

    ast_node* clone()
    {
        return new ast_node_rdiv { first->clone(), sec->clone() };
    }

    expr_value* eval()
    {
        auto fv = first->eval();
        auto sv = sec->eval();

        if (fv->type == value_type::DOUBLE
                && sv->type == value_type::DOUBLE) {
            auto d_fv = dynamic_cast<expr_value_double*>(fv);
            auto d_sv = dynamic_cast<expr_value_double*>(sv);

            double ans = d_fv->d / d_sv->d;

            delete fv;
            delete sv;

            return new expr_value_double { ans };
        } else if (fv->type == value_type::DOUBLE
                && sv->type == value_type::INT) {
            auto d_fv = dynamic_cast<expr_value_double*>(fv);
            auto i_sv = dynamic_cast<expr_value_int*>(sv);

            double ans = d_fv->d / i_sv->i;

            delete fv;
            delete sv;

            return new expr_value_double { ans };
        } else if (fv->type == value_type::INT
                && sv->type == value_type::DOUBLE) {
            auto i_fv = dynamic_cast<expr_value_int*>(fv);
            auto d_sv = dynamic_cast<expr_value_double*>(sv);

            double ans = i_fv->i / d_sv->d;

            delete fv;
            delete sv;

            return new expr_value_double { ans };
        } else if (fv->type == value_type::INT
                && sv->type == value_type::INT) {
            auto i_fv = dynamic_cast<expr_value_int*>(fv);
            auto i_sv = dynamic_cast<expr_value_int*>(sv);

            int ans = i_fv->i / i_sv->i;

            delete fv;
            delete sv;

            return new expr_value_int { ans };
        } else {
            delete fv;
            delete sv;
            throw std::runtime_error("AST node rdiv:  types not implemented");
        }

    }

    ast_node *first;
    ast_node *sec;
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

    ast_node* clone()
    {
        return new ast_node_pow { first->clone(), sec->clone() };
    }

    emc_type resolve()
    {
        return value_type = standard_type_promotion(first->resolve(), sec->resolve());
    }

    expr_value* eval()
    {
        auto fv = first->eval();
        auto sv = sec->eval();

        if (fv->type == value_type::DOUBLE
                && sv->type == value_type::DOUBLE) {
            auto d_fv = dynamic_cast<expr_value_double*>(fv);
            auto d_sv = dynamic_cast<expr_value_double*>(sv);

            double ans = pow(d_fv->d, d_sv->d);

            delete fv;
            delete sv;

            return new expr_value_double { ans };
        } else if (fv->type == value_type::DOUBLE
                && sv->type == value_type::INT) {
            auto d_fv = dynamic_cast<expr_value_double*>(fv);
            auto i_sv = dynamic_cast<expr_value_int*>(sv);

            double ans = pow(d_fv->d, i_sv->i);

            delete fv;
            delete sv;

            return new expr_value_double { ans };
        } else if (fv->type == value_type::INT
                && sv->type == value_type::DOUBLE) {
            auto i_fv = dynamic_cast<expr_value_int*>(fv);
            auto d_sv = dynamic_cast<expr_value_double*>(sv);

            double ans = pow(i_fv->i, d_sv->d);

            delete fv;
            delete sv;

            return new expr_value_double { ans };
        } else if (fv->type == value_type::INT
                && sv->type == value_type::INT) {
            auto i_fv = dynamic_cast<expr_value_int*>(fv);
            auto i_sv = dynamic_cast<expr_value_int*>(sv);

            double ans_d = pow((double) i_fv->i, (double) i_sv->i);
            int ans = std::round(ans_d);
            delete fv;
            delete sv;

            return new expr_value_int { ans };
        } else {
            delete fv;
            delete sv;
            throw std::runtime_error("AST node pow:  types not implemented");
        }

    }

    ast_node *first;
    ast_node *sec;
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
        return new ast_node_abs { first->clone() };
    }

    emc_type resolve()
    {
        return value_type = first->resolve();
    }

    expr_value* eval()
    {
        auto fv = first->eval();

        if (fv->type == value_type::DOUBLE) {
            auto d_fv = dynamic_cast<expr_value_double*>(fv);

            double ans = d_fv->d < 0 ? -d_fv->d : d_fv->d;
            delete fv;
            return new expr_value_double { ans };
        } else if (fv->type == value_type::INT) {
            auto i_fv = dynamic_cast<expr_value_int*>(fv);

            int ans = i_fv->i < 0 ? -i_fv->i : i_fv->i;
            delete fv;
            return new expr_value_int { ans };
        }
        throw std::runtime_error("AST node unary minus types not implemented");
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
        type = ast_type::DOUBLE_UMINUS;
    }

    ~ast_node_uminus()
    {
        if (first) delete first;
    }

    ast_node* clone()
    {
        return new ast_node_uminus { first->clone() };
    }

    emc_type resolve()
    {
        return value_type = first->resolve();
    }

    expr_value* eval()
    {
        auto fv = first->eval();

        if (fv->type == value_type::DOUBLE) {
            auto d_fv = dynamic_cast<expr_value_double*>(fv);

            double ans = -d_fv->d;
            delete fv;
            return new expr_value_double { ans };
        } else if (fv->type == value_type::INT) {
            auto i_fv = dynamic_cast<expr_value_int*>(fv);

            int ans = -i_fv->i;
            delete fv;
            return new expr_value_int { ans };
        }
        throw std::runtime_error("AST node unary minus types not implemented");

    }

    ast_node *first;
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
        return p->resolve();
    }

    ast_node* clone()
    {
        return new ast_node_var { name, nspace };
    }

    expr_value* eval()
    {
        extern scope_stack scopes;
        auto p = scopes.find_object(name, nspace);
        if (!p)
            throw std::runtime_error("Object does not exist: >>" +
                    nspace + name + "<<");
        return p->eval();
    }

    expr_value* leval()
    {
        extern scope_stack scopes;
        auto p = scopes.find_object(name, nspace);
        return new expr_value_lval { p };
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

    ast_node* clone()
    {
        return new ast_node_assign { first, sec };
    }

    emc_type resolve()
    {
        return value_type = first->resolve();
    }

    expr_value* eval()
    {
        if (first->type != ast_type::VAR)
            throw std::runtime_error("Left hand of assigned not lvalue");
        auto lvar = dynamic_cast<ast_node_var*>(first);
        expr_value *lval_ = lvar->leval();
        if (lval_ == nullptr || lval_->type != value_type::LVAL)
            throw std::runtime_error("ast_node_assign bugg");
        auto lval = dynamic_cast<expr_value_lval*>(lval_);

        auto rval = sec->eval();

        /* If the obj doesnt exist we create it. */
        if (!lval->object) {
            extern scope_stack scopes;

            if (rval->type == value_type::DOUBLE) {
                auto obj = new object_double { lvar->name, lvar->nspace, 0. };
                scopes.get_top_scope().push_object(obj);
                lval->object = obj;
            } else if (rval->type == value_type::STRING) {
                auto obj = new object_string { lvar->name, lvar->nspace, "" };
                scopes.get_top_scope().push_object(obj);
                lval->object = obj;
            } else if (rval->type == value_type::INT) {
                auto obj = new object_int { lvar->name, lvar->nspace, 0 };
                scopes.get_top_scope().push_object(obj);
                lval->object = obj;
            }
        }

        auto ans = lval->object->assign(rval);
        delete rval;

        return ans;
    }

    ast_node *first;
    ast_node *sec;
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

    emc_type resolve()
    {
        return value_type = emc_type{emc_types::INT};
    }

    ast_node* clone()
    {
        return new ast_node_cmp { first, sec };
    }

    expr_value* eval()
    {
        auto fv = first->eval();
        auto sv = sec->eval();

        auto ans_val = cmp_helper(fv, sv);

        delete fv;
        delete sv;

        return new expr_value_int { ans_val };
    }

    ast_node *first;
    ast_node *sec;
};

class ast_node_temporary: public ast_node {
public:
    ast_node *first;
    expr_value *val = nullptr;

    ast_node_temporary(ast_node *first) :
            first(first)
    {
        type = ast_type::TEMP;
    }
    ~ast_node_temporary()
    {
        delete val;
    }

    ast_node* clone()
    {
        return new ast_node_temporary { first };
    }

    emc_type resolve()
    {
        return value_type = first->resolve();
    }

    expr_value* eval()
    {
        if (!val)
            val = first->eval();
        if (!val)
            throw std::runtime_error("temp bugg null val");
        return val->clone();
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

    ast_node* clone()
    {
        /* TODO: Fixa så att ctorn inte tar first och att alla element läggs till med append_node */
        auto iter = v_nodes.begin();
        if (iter == v_nodes.end())
            throw std::runtime_error("Bugg ast_node_explist");

        auto pel = new ast_node_explist { (*iter++)->clone() };

        for (; iter != v_nodes.end(); iter++)
            pel->append_node((*iter)->clone());

        return pel;
    }

    void append_node(ast_node *first)
    {
        v_nodes.push_back(first);
    }

    expr_value* eval()
    {
        expr_value *ans = 0;
        for (auto p : v_nodes)
           ans = p->eval();
        if (!ans) /* Expressionslist can be empty eg. all EOLs. */
            ans = new expr_value_int{0};
        return ans;
    }

    std::vector<ast_node*> v_nodes;
};

class ast_node_doblock: public ast_node {
public:
    ast_node *first;

    ast_node_doblock(ast_node *first) :
            first(first)
    {
        type = ast_type::DOBLOCK;
    }
    ~ast_node_doblock()
    {
        if (first) delete first;
    }

    emc_type resolve()
    {
        return value_type = first->resolve();
    }

    ast_node* clone()
    {
        return new ast_node_doblock { first->clone() };
    }

    expr_value* eval()
    {
        extern scope_stack scopes;

        scopes.push_new_scope();
        auto val = first->eval();
        scopes.pop_scope();
        return val;
    }
};

class ast_node_if: public ast_node {
public:
    ast_node *cond_e;
    ast_node *if_el;
    ast_node *else_el; /* else_el can be a linked IF or another expression (for IF ELSE ... ) */
    ast_node *also_el = 0;
    /* The ALSO statement is assumed to be in
     * the first IF node and not the linked IFs. */

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

    /* The type of the value of an IF statement expression is NONE unless
     * all the blocks have a not NONE type, which have to be promotional
     * to eachother.
     */
    emc_type resolve()
    {
        if (!else_el && !also_el)
            return if_el->resolve();
        else if (else_el && !also_el) {
            auto value_type_if = if_el->resolve();
            auto value_type_else = else_el->resolve();

            auto t = standard_type_promotion_or_invalid(value_type_if, value_type_else);
            if (t.is_valid())
                return t;
            else
                return emc_type{emc_types::NONE};
        } else {
            auto value_type_if = if_el->resolve();
            auto value_type_else = else_el->resolve();
            auto value_type_also = also_el->resolve();

            auto t = standard_type_promotion_or_invalid(value_type_if, value_type_else);
            t = standard_type_promotion_or_invalid(t, value_type_also);
            if (t.is_valid())
                return t;
            else
                return emc_type{emc_types::NONE};
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
        return new ast_node_if { cond_e->clone(), if_el->clone(),
                else_el ? else_el->clone() : nullptr };
    }

    expr_value* eval()
    {
        bool ifs_executed = false; /* Keep track of if any IF-conditions were true */
        auto ans = ifeval(ifs_executed);

        if (ifs_executed && also_el) {
            delete ans;

            extern scope_stack scopes;

            scopes.push_new_scope();
            ans = also_el->eval();
            scopes.pop_scope();
        }

        return ans;
    }

    /* This eval() evaluates all the linked if:s also */
    expr_value* ifeval(bool &ifs_executed)
    {
        extern scope_stack scopes;

        auto ev_cond = std::unique_ptr<expr_value>(cond_e->eval());
        if (ev_cond->type != value_type::DOUBLE)
            throw std::runtime_error("ast_node_if Cond type not implemented");
        auto ed = std::unique_ptr<expr_value_double>(
                dynamic_cast<expr_value_double*>(ev_cond.release()));

        expr_value *ans = 0;

        if (ed->d) {
            scopes.push_new_scope();
            ans = if_el->eval();
            scopes.pop_scope();
            ifs_executed = true;
        } else if (else_el) {

            /* Push a scope unless the else node is a linked IF node (that pushes it's own scope). */
            if (else_el->type != ast_type::IF) {
                scopes.push_new_scope();
                ans = else_el->eval();
                scopes.pop_scope();
                return ans;
            }
            auto ifnode = dynamic_cast<ast_node_if*>(else_el);
            return ifnode->ifeval(ifs_executed); /* Chain on the "one if has been executed (for the ALSO) */
        } else
            ans = new expr_value_double { 0. };

        return ans;
    }
};

class ast_node_while: public ast_node {
public:
    ast_node *cond_e;
    ast_node *if_el;
    ast_node *else_el;
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

    ast_node* clone()
    {
        return new ast_node_while { cond_e->clone(), if_el->clone(),
                else_el ? else_el->clone() : nullptr };
    }

    emc_type resolve()
    {
        if (!else_el)
            return if_el->resolve();
        else {
            auto value_type_if = if_el->resolve();
            auto value_type_else = else_el->resolve();

            auto t = standard_type_promotion_or_invalid(value_type_if, value_type_else);
            if (t.is_valid())
                return t;
            else
                return emc_type{emc_types::NONE};
        }
    }

    expr_value* eval()
    {
        extern scope_stack scopes;

        auto ev_cond = cond_e->eval();
        if (ev_cond->type != value_type::DOUBLE)
            throw std::runtime_error(
                    "ast_node_while Cond type not implemented");
        auto ed = dynamic_cast<expr_value_double*>(ev_cond);

        expr_value *ans;

        bool initial_value = false;
        if (ed->d) initial_value = true;
        delete ev_cond;

        if (initial_value) {
            scopes.push_new_scope();
            redo:
            ans = if_el->eval();

            ev_cond = cond_e->eval();
            if (ev_cond->type != value_type::DOUBLE)
                throw std::runtime_error(
                        "ast_node_while Cond type not implemented");
            ed = dynamic_cast<expr_value_double*>(ev_cond);
            if (ed->d) {
                delete ev_cond;
                delete ans;

                goto redo;
            }

            scopes.pop_scope();
        }

        /* See if the else should be executed. */
        if (!initial_value && else_el) {
            scopes.push_new_scope();
            ans = else_el->eval();
            scopes.pop_scope();
        } else if (!initial_value) {
            /* If the while loop was executed 0 times without a else return empty value. */
            ans = new expr_value_double { 0. };
        }

        return ans;
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

    expr_value* eval()
    {
        throw std::runtime_error("Can't eval arglist.");
    }
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

    ast_node* clone()
    {
        auto argl = new ast_node_arglist { };
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
        return value_type = emc_type{emc_types::NONE};
    }

    expr_value* eval()
    {
        auto val_list = new expr_value_list;
        for (auto node : v_ast_args)
            val_list->v_val.push_back(node->eval());
        return val_list;
    }

    std::vector<ast_node*> v_ast_args;
};

class ast_node_funcdec: public ast_node {
public:
    ast_node_funcdec(ast_node *parlist, ast_node *code_block,
            std::string name, std::string nspace)
    :
            parlist(parlist), code_block(code_block),
                    name(name), nspace(nspace)
    {
        type = ast_type::FUNCTION_DECLARATION;
    }
    /* fobj äger ast_noderna kanske är dumt? */
    ~ast_node_funcdec()
    {
        delete code_block;
        delete parlist;
    }

    emc_type resolve()
    {
        extern scope_stack resolve_scope;
        auto aa = dynamic_cast<ast_node_parlist*>(parlist);
        if (!aa)
            throw std::runtime_error("ast_node* clone()");
        extern scope_stack resolve_scope;
        auto fobj = new object_func { code_block->clone(), name, nspace,
                parlist->clone() };
        resolve_scope.get_top_scope().push_object(fobj);

        return value_type = emc_type{emc_types::FUNCTION}; /* TODO: Add types too */
    }

    ast_node* clone()
    {
        return new ast_node_funcdec { parlist->clone(),
                code_block->clone(),
                name, nspace };
    }

    /* Evaluating the function declaration creates the function object
     * and clones the relevant ast nodes.
     */
    expr_value* eval()
    {
        auto aa = dynamic_cast<ast_node_parlist*>(parlist);
        if (!aa)
            throw std::runtime_error("qweqweqweqweqweesasdfdsasdffd");
        extern scope_stack scopes;
        auto fobj = new object_func { code_block->clone(), name, nspace,
                parlist->clone() };
        scopes.get_top_scope().push_object(fobj);

        return new expr_value_double { 0. };
    }

    ast_node *code_block;
    ast_node *parlist;
    std::string name;
    std::string nspace;
};

class ast_node_funccall: public ast_node {
public:
    ast_node_funccall(std::string nspace, std::string name, ast_node *arg_list) :
            nspace(nspace), arg_list(arg_list), name(name)
    {
        type = ast_type::FUNCTION_CALL;
    }
    std::string name;
    std::string nspace;
    ast_node *arg_list;

    ast_node* clone()
    {
        return new ast_node_funccall { nspace,
                name,
                arg_list->clone() };
    }

    emc_type resolve()
    {
        extern scope_stack resolve_scope;
        auto obj = resolve_scope.find_object(name, nspace);
        if (!obj) /* TODO: Kolla så fn */
            throw std::runtime_error(
                    "Could not find function " + nspace + " " + name);
        return value_type = obj->resolve();
    }

    expr_value* eval()
    {
        extern scope_stack scopes;

        auto obj = scopes.find_object(name, nspace);
        if (!obj)
            throw std::runtime_error(
                    "Could not find function " + nspace + " " + name);
        if (obj->type != object_type::FUNC
                && obj->type != object_type::STATIC_CFUNC)
            throw std::runtime_error(
                    "Symbol " + nspace + " " + name + " not a function.");

        auto fobj = dynamic_cast<object_func_base*>(obj);

        /* The expr_value_list from eval() owns the
         * pointers to the corresponding expr_value:s. */
        auto val = dynamic_cast<expr_value_list*>(arg_list->eval());
        if (!val)
            throw std::runtime_error("feval kan inte göras med null obj");
        return fobj->feval(val);
    }
};

class ast_node_chainable: public ast_node {
public:
    std::shared_ptr<ast_node> first;
    std::shared_ptr<ast_node> sec;
};

class ast_node_andchain: public ast_node {
public:
    std::vector<ast_node_chainable*> v_children;
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

    emc_type resolve()
    {
        for (auto e : v_children)
            e->resolve();
        return value_type = emc_type{emc_types::INT};
    }

    ast_node* clone()
    {
        auto p = new ast_node_andchain {};
        for (auto e : v_children)
            p->append_next(dynamic_cast<ast_node_chainable*>(e->clone()));
        return p;
    }

    void append_next(ast_node_chainable *node)
    {
        DEBUG_ASSERT(node != nullptr, "node is null");
        DEBUG_ASSERT(v_children.size(), "v_children is empty");
        DEBUG_ASSERT(node->first == nullptr, "node->first not a nullptr");
        node->first = v_children.back()->sec;
        v_children.push_back(node);
    }

    std::shared_ptr<ast_node> last_chain_node_leaf()
    {
        return v_children.back()->sec;
    }

    expr_value* eval()
    {
        bool ans = true; /* Assume that the andchain will yield true */

        DEBUG_ASSERT(v_children.size(),"Evaluating empty andchain");

        for (auto child : v_children) {
            auto e = child->eval();
            if (e->type != value_type::INT)
                throw std::runtime_error("andchain Type not implemented");
            auto ed = dynamic_cast<expr_value_int*>(e);

            ans = (ed->i != 0) && ans;

            delete e;
        }

        return new expr_value_int { ans };
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
        return new ast_node_les { first->clone(), sec->clone() };
    }

    emc_type resolve()
    {
        return value_type = emc_type{emc_types::INT};
    }

    expr_value* eval()
    {
        auto fv = first->eval();
        auto sv = sec->eval();

        int a = cmp_helper(fv, sv);

        delete fv;
        delete sv;

        if (a == -1)
            return new expr_value_int { 1 };
        return new expr_value_int { 0 };
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
        return value_type = emc_type{emc_types::INT};
    }
    ast_node* clone()
    {
        return new ast_node_gre { first->clone(), sec->clone() };
    }

    expr_value* eval()
    {
        auto fv = first->eval();
        auto sv = sec->eval();

        int a = cmp_helper(fv, sv);

        delete fv;
        delete sv;

        if (a == 1)
            return new expr_value_int { 1 };
        return new expr_value_int { 0 };

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
        return value_type = emc_type{emc_types::INT};
    }
    ast_node* clone()
    {
        return new ast_node_equ { first->clone(), sec->clone() };
    }

    expr_value* eval()
    {
        auto fv = first->eval();
        auto sv = sec->eval();

        int a = cmp_helper(fv, sv);

        delete fv;
        delete sv;

        if (a == 0)
            return new expr_value_int { 1 };
        return new expr_value_int { 0 };

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
        return value_type = emc_type{emc_types::INT};
    }
    ast_node* clone()
    {
        return new ast_node_leq { first->clone(), sec->clone() };
    }

    expr_value* eval()
    {
        auto fv = first->eval();
        auto sv = sec->eval();

        int a = cmp_helper(fv, sv);

        delete fv;
        delete sv;

        if (a == -1 || a == 0)
            return new expr_value_int { 1 };
        return new expr_value_int { 0 };

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
        return value_type = emc_type{emc_types::INT};
    }
    ast_node* clone()
    {
        return new ast_node_geq { first->clone(), sec->clone() };
    }

    expr_value* eval()
    {
        auto fv = first->eval();
        auto sv = sec->eval();

        int a = cmp_helper(fv, sv);

        delete fv;
        delete sv;

        if (a == 0 || a == 1)
            return new expr_value_int { 1 };
        return new expr_value_int { 0 };

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
        return value_type = emc_type{emc_types::INT};
    }
    ast_node* clone()
    {
        return new ast_node_neq { first->clone(), sec->clone() };
    }

    expr_value* eval()
    {
        auto fv = first->eval();
        auto sv = sec->eval();

        int a = cmp_helper(fv, sv);

        delete fv;
        delete sv;

        if (a == 1 || a == -1 || a == 3)
            return new expr_value_int { 1 };
        return new expr_value_int { 0 };

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

    ast_node* clone()
    {
        return new ast_node_def { type_name, var_name,
                value_node ? value_node->clone() : nullptr };
    }
    emc_type resolve()
    {
        extern scope_stack resolve_scope;

        type_object *type = resolve_scope.get_top_scope().find_type(type_name);
        obj *obj;
        obj = type->ctor();
        obj->name = var_name;

        resolve_scope.get_top_scope().push_object(obj);

        /* TODO: This wont work for user types ... */
        if (type_name == "Int")
            return value_type = emc_type{emc_types::INT};
        else if (type_name == "Double")
            return value_type = emc_type{emc_types::DOUBLE};
        else if (type_name == "String")
            return value_type = emc_type{emc_types::STRING};
        throw std::runtime_error("Not implemented proper types ... ");
    }
    expr_value* eval()
    {
        extern scope_stack scopes;

        expr_value *val = nullptr;
        if (value_node)
            val = value_node->eval();

        type_object *type = scopes.get_top_scope().find_type(type_name);
        if (!type)
            throw std::runtime_error("Type does not exist: " + type_name);

        obj *obj;

        if (value_node)
            obj = type->ctor(val);
        else
            obj = type->ctor();
        obj->name = var_name;

        scopes.get_top_scope().push_object(obj);

        return val;
    }

    std::string type_name;
    std::string var_name;
    ast_node *value_node = nullptr;
};

