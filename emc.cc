#include <cmath>
#include <cstdio>
#include <cstdarg>

#include "emc.hh"

#ifndef NDEBUG
int ast_node_count;
int value_expr_count;
#endif

emc_type ast_node_funcdec::resolve()
{
    extern scope_stack resolve_scope;
    parlist->resolve();
    
    resolve_scope.push_new_scope();
    auto parlist_t = dynamic_cast<ast_node_vardef_list*>(parlist);
    DEBUG_ASSERT_NOTNULL(parlist_t);
    parlist_t->resolve();

    /* Create variables with the arguments' names in the function scope. */
    for (int i = 0; i < parlist_t->v_defs.size(); i++) {
        auto par = dynamic_cast<ast_node_def*>(parlist_t->v_defs[i]);
        std::string var_name = par->var_name;

        /* TODO: Måste tänka över typerna lite ... */
        obj *obj = nullptr;
        if (par->type_name == "Double")
            obj = new object_double { var_name, "", 0 };
        else if (par->type_name == "Int")
            obj = new object_int { var_name, "", 0 };
        else
            throw std::runtime_error("Type not implemented: " + par->type_name);
        resolve_scope.get_top_scope().push_object(obj);
    }
    code_block->resolve();
    resolve_scope.pop_scope();

    /* Hack to allow for return names with same names as other things ... */
    resolve_scope.push_new_scope();
    vardef_list->resolve();
    resolve_scope.pop_scope();

    auto fobj = new object_func { code_block->clone(), name, nspace,
            parlist->clone() , vardef_list->clone()};
    resolve_scope.get_top_scope().push_object(fobj);

    return value_type = emc_type{emc_types::FUNCTION}; /* TODO: Add types too */
}

emc_type ast_node_vardef_list::resolve()
{
    if (v_defs.size() == 0)
        throw std::runtime_error("0 vars in deflist not supported yet");
    
    for (auto e : v_defs) {
        auto ee = dynamic_cast<ast_node_def*>(e);
        emc_type value = ee->resolve_no_push();
    }

    return value_type = v_defs.front()->value_type;
}

void scope_stack::clear()
{
    for (auto &e : vec_scope)
        e.clear();
}

object_func::~object_func()
{
    delete root;
    delete para_list;
    delete var_list;
}
    
emc_type object_func::resolve()
{
    return var_list->resolve();
}


emc_type standard_type_promotion(const emc_type &a, const emc_type &b)
{
    auto ans = standard_type_promotion_or_invalid(a, b);
    if (a.types[0] == emc_types::INVALID)
        throw std::runtime_error("emc_types has no valid standard promotion");
    return ans;
}

void scope_stack::debug_print()
{
    for (auto &scope : vec_scope) {
        std::cout << "Scope:" << std::endl;
        for (auto obj : scope.vec_objs) {
            obj->debug_print();
        }
    }
}

bool truthy_value(expr_value *val)
{
    if (val->type == value_type::DOUBLE) {
        auto *v = dynamic_cast<expr_value_double*>(val);
        if (v->d != 0)
            return true;
        return false;
    } else if (val->type == value_type::INT) {
        auto *v = dynamic_cast<expr_value_int*>(val);
        if (v->i != 0)
            return true;
        return false;
    } else if (val->type == value_type::STRING) {
        auto *v = dynamic_cast<expr_value_string*>(val);
        if (v->s.size() != 0)
            return true;
        return false;
    } else
        throw std::runtime_error("trutchy_value not implemented type");
}

emc_type standard_type_promotion_or_invalid(const emc_type &a, const emc_type &b)
{
    if (!a.types.size() || !b.types.size())
            throw std::runtime_error("Either emc_type has no elements");

    auto at = a.types[0];
    auto bt = a.types[0];

    if (at == bt)
        return emc_type{at};

    if ((at == emc_types::DOUBLE && bt == emc_types::INT)
            || (bt == emc_types::DOUBLE && at == emc_types::INT))
        return emc_type{emc_types::DOUBLE};
    else if (at == emc_types::STRING && bt == emc_types::STRING)
        return emc_type{emc_types::STRING};
    return emc_type{emc_types::INVALID};
}

/* Replaces any c-like escaped characters in a string with the proper values.
 * Eg. '\' 'a' => 7 (bell)
 *
 * Only hex and octal escaped numbers valid (not UTF escapes)
 */
void deescape_string(std::string &s)
{
    bool is_escaped = false;
    bool octal_escape = false;
    bool hex_escape = false;

    for (int i = 0; i < s.size(); i++) {
        char c = s[i];
        bool is_last = i + 1 == s.size();
        redo:
        if (is_escaped) {
            is_escaped = false;
            bool hit = true;

            switch (c) {
            case 'n':
                s.replace(i - 1, 2, "\n");
                break;
            case 'a':
                s.replace(i - 1, 2, "\a");
                break;
            case 'v':
                s.replace(i - 1, 2, "\v");
                break;
            case 't':
                s.replace(i - 1, 2, "\t");
                break;
            case 'r':
                s.replace(i - 1, 2, "\r");
                break;
            case '\\':
                s.replace(i - 1, 2, "\\");
                break;
            case '\'':
                s.replace(i - 1, 2, "\'");
                break;
            case '\"':
                s.replace(i - 1, 2, "\"");
                break;
            case '\b':
                s.replace(i - 1, 2, "\b");
                break;
            case '\?':
                s.replace(i - 1, 2, "\?");
                break;
            default:
                hit = false;
            }

            if (hit) {
                i--;
                continue;
            }

            if (std::isdigit(c)) { /* Octal escape sequence. */
                octal_escape = true;
                goto redo;
            }

            if (c == 'x') {
                hex_escape = true;
                if (is_last)
                    throw std::runtime_error(
                            "Not valid escape in string literal: " + s);
                continue;
            }

            throw std::runtime_error(
                    "Not valid escape in string literal: " + s);

        } else if (octal_escape) {
            /* s[i] is a digit here */
            /* At most 3 characters. */
            for (int j = i; j < s.size() && j < i + 3; j++) {
                char c = s[j];
                bool is_last = j + 1 == s.size() || j == i + 2;
                if (!std::isdigit(c) || is_last) {
                    int n;
                    if (is_last && !std::isdigit(c))
                        n = j - i;
                    else
                        n = j - i + 1;
                    // s[n - 1] == '\\'
                    unsigned int oct;
                    try {
                        oct = std::stoul(s.substr(i, n), 0, 8);
                    } catch (std::invalid_argument &e) {
                        throw std::runtime_error(
                                "Not valid octal escape in string literal: "
                                        + s);
                    }
                    if (oct > 255)
                        throw std::runtime_error(
                                "Not valid octal escape in string literal: "
                                        + s);
                    s.erase(i - 1, n);
                    s[i - 1] = oct;
                    i--;
                    octal_escape = false;
                    break;
                }
            }
        } else if (hex_escape) {
            /* s[i] is a the first digit here */
            /* At most 2 characters. */
            std::cout << "i : " << i << std::endl;
            for (int j = i; (j < s.size()) && (j < i + 2); j++) {
                std::cout << "inner i : " << i << " j " << j << std::endl;
                char c = s[j];
                bool is_last = j + 1 == s.size() || j == i + 1;
                bool is_hex = std::isdigit(c) || (c <= 'F' && c >= 'A')
                        || (c <= 'f' && c >= 'a');
                if (!is_hex || is_last) {
                    int n;
                    if (is_last && !is_hex)
                        n = j - i;
                    else
                        n = j - i + 1;
                    // s[n - 1] == '\\'
                    unsigned int hex;
                    try {
                        std::cout << "Hex: " << s.substr(i, n) << std::endl;
                        hex = std::stoul(s.substr(i, n), 0, 16);
                    } catch (std::invalid_argument &e) {
                        throw std::runtime_error(
                                "Not valid hex escape in string literal: " + s);
                    }
                    if (hex > 255)
                        throw std::runtime_error(
                                "Not valid hex escape in string literal: " + s);
                    s.erase(i - 2, n + 1);
                    s[i - 2] = hex;
                    i -= 2;
                    hex_escape = false;
                    break;
                }
            }
        } else if (c == '\\') {
            is_escaped = true;
        }
    }
}

int cmp_helper(expr_value *fv, expr_value *sv)
{
    if (fv->type == value_type::DOUBLE
            && sv->type == value_type::DOUBLE) {
        auto d_fv = dynamic_cast<expr_value_double*>(fv);
        auto d_sv = dynamic_cast<expr_value_double*>(sv);
        int ans;
        if (d_fv->d < d_sv->d)
            return -1;
        else if (d_fv->d > d_sv->d)
            return 1;
        else if (d_fv->d == d_sv->d)
            return 0;
        return 3; /* NaN */

    } else if (fv->type == value_type::INT
            && sv->type == value_type::DOUBLE) {
        auto i_fv = dynamic_cast<expr_value_int*>(fv);
        auto d_sv = dynamic_cast<expr_value_double*>(sv);
        int ans;
        if (i_fv->i < d_sv->d)
            return -1;
        else if (i_fv->i > d_sv->d)
            return 1;
        else if (i_fv->i == d_sv->d)
            return 0;
        return 3; /* NaN */
    } else if (fv->type == value_type::DOUBLE
            && sv->type == value_type::INT) {
        auto d_fv = dynamic_cast<expr_value_double*>(fv);
        auto i_sv = dynamic_cast<expr_value_int*>(sv);
        if (d_fv->d < i_sv->i)
            return -1;
        else if (d_fv->d > i_sv->i)
            return 1;
        else if (d_fv->d == i_sv->i)
            return 0;
        return 3;

    } else if (fv->type == value_type::INT
            && sv->type == value_type::INT) {
        auto i_fv = dynamic_cast<expr_value_int*>(fv);
        auto i_sv = dynamic_cast<expr_value_int*>(sv);

        if (i_fv->i < i_sv->i)
            return -1;
        else if (i_fv->i > i_sv->i)
            return 1;
        else if (i_fv->i == i_sv->i)
            return 0;
        return 3;

    } else {
        throw std::runtime_error("AST node cmp:  types not implemented");
    }
}

void scope_stack::push_new_scope()
{

    vec_scope.emplace_back(scope{});
}

obj* scope_stack::find_object(std::string name, std::string nspace)
{
    /* Search backwards so that the top scope matches first. */
    for (auto it = vec_scope.rbegin(); it != vec_scope.rend(); it++)
            {
        obj *p = it->find_object(name, nspace);
        if (p)
            return p;
    }

    return nullptr;
}

type_object* scope_stack::find_type(std::string name)
{
    /* Search backwards so that the top scope matches first. */
    for (auto it = vec_scope.rbegin(); it != vec_scope.rend(); it++)
            {
        type_object *p = it->find_type(name);
        if (p)
            return p;
    }

    return nullptr;
}

expr_value* object_func::feval(expr_value_list *arg_value_list)
{
    extern scope_stack scopes;
    scopes.push_new_scope();
    auto &scope = scopes.get_top_scope();
    if (!para_list)
        throw std::runtime_error("para_list == null");
    auto para_listc = dynamic_cast<ast_node_parlist*>(para_list);
    if (!para_listc)
        throw std::runtime_error("para_listc == null");
    auto s1 = arg_value_list->v_val.size();
    auto s2 = para_listc->v_func_paras.size();
    if (s1 != s2)
        throw std::runtime_error("Wrong amount of parameters in function call");

    /* Create variables with the arguments' names in the function scope. */
    for (int i = 0; i < para_listc->v_func_paras.size(); i++) {
        std::string var_name = para_listc->v_func_paras[i].name;
        auto val = static_cast<expr_value_double*>(arg_value_list->v_val[i]);
        auto obj = new object_double { var_name, "", val->d };
        scopes.get_top_scope().push_object(obj);
    }

    auto func_value = root->eval();
    /* Destroy the function scope. */
    scopes.pop_scope();

    return func_value;
}

//@formatter:off

#define CHECK_N_ARGS(n) \
	do {if (arg_value_list->v_val.size() != (n))\
			throw std::runtime_error("Wrong amount of arguments");} while(0)

#define N_ARGS \
    (arg_value_list->v_val.size())\


#define GET_NTH_ARG_AS_DOUBLE(n, arg) \
	do {auto p_ev = arg_value_list->v_val[(n)];\
		auto p_evd = dynamic_cast<expr_value_double*>(p_ev);\
		if (!p_evd)\
				throw std::runtime_error("Argument not correct type");\
		arg = p_evd->d;} while(0)

#define GET_NTH_ARG_AS_STRING(n, arg) \
	do {auto p_ev = arg_value_list->v_val[(n)];\
		auto p_evd = dynamic_cast<expr_value_string*>(p_ev);\
		if (!p_evd)\
				throw std::runtime_error("Argument not correct type");\
		arg = p_evd->s;} while(0)

#define ONE_D_ARG_ONE_D_RETURN_ARG_MUL_ANS_MUL( cfunc_c_name, wrapper_fn_name, arg_mul, ans_mul) \
static expr_value* wrapper_fn_name (expr_value_list *arg_value_list)  \
{ \
	CHECK_N_ARGS(1); \
	 \
	double arg; \
	GET_NTH_ARG_AS_DOUBLE(0, arg); \
	 \
	double ans = cfunc_c_name (arg arg_mul); \
	 \
	return new expr_value_double{ans ans_mul}; \
} \

#define TWO_D_ARG_ONE_D_RETURN_ARG_MUL_ANS_MUL( cfunc_c_name, wrapper_fn_name, arg_mul, ans_mul) \
static expr_value* wrapper_fn_name (expr_value_list *arg_value_list)  \
{ \
	CHECK_N_ARGS(2); \
	 \
	double arg1; \
	GET_NTH_ARG_AS_DOUBLE(0, arg1); \
	double arg2; \
	GET_NTH_ARG_AS_DOUBLE(1, arg2); \
	 \
	double ans = cfunc_c_name (arg1 arg_mul, arg2 arg_mul); \
	 \
	return new expr_value_double{ans ans_mul}; \
} \

#define ONE_D_ARG_ONE_D_RETURN(cfunc_c_name, wrapper_fn_name) \
	ONE_D_ARG_ONE_D_RETURN_ARG_MUL_ANS_MUL( cfunc_c_name, wrapper_fn_name,   ,   )
#define TWO_D_ARG_ONE_D_RETURN(cfunc_c_name, wrapper_fn_name) \
	TWO_D_ARG_ONE_D_RETURN_ARG_MUL_ANS_MUL( cfunc_c_name, wrapper_fn_name,   ,   )

#define PI_D 3.14159265358979323846

ONE_D_ARG_ONE_D_RETURN(std::isnan, static_cfunc_d_isnan_d)
ONE_D_ARG_ONE_D_RETURN(std::isinf, static_cfunc_d_isinf_d)
ONE_D_ARG_ONE_D_RETURN(std::isfinite, static_cfunc_d_isfinite_d)
ONE_D_ARG_ONE_D_RETURN(std::sqrt, static_cfunc_d_sqrt_d)
TWO_D_ARG_ONE_D_RETURN(std::fmod, static_cfunc_d_fmod_d_d)
TWO_D_ARG_ONE_D_RETURN(std::remainder, static_cfunc_d_remainder_d_d)

ONE_D_ARG_ONE_D_RETURN(std::sin, static_cfunc_d_sin_d)
ONE_D_ARG_ONE_D_RETURN(std::cos, static_cfunc_d_cos_d)
ONE_D_ARG_ONE_D_RETURN(std::tan, static_cfunc_d_tan_d)
ONE_D_ARG_ONE_D_RETURN(std::asin, static_cfunc_d_asin_d)
ONE_D_ARG_ONE_D_RETURN(std::acos, static_cfunc_d_acos_d)
ONE_D_ARG_ONE_D_RETURN(std::atan, static_cfunc_d_atan_d)
TWO_D_ARG_ONE_D_RETURN(std::atan2, static_cfunc_d_atan2_d_d)


ONE_D_ARG_ONE_D_RETURN_ARG_MUL_ANS_MUL(std::sin, static_cfunc_d_sind_d			, *(PI_D/180),  		  )
ONE_D_ARG_ONE_D_RETURN_ARG_MUL_ANS_MUL(std::cos, static_cfunc_d_cosd_d			, *(PI_D/180),  		  )
ONE_D_ARG_ONE_D_RETURN_ARG_MUL_ANS_MUL(std::tan, static_cfunc_d_tand_d			, *(PI_D/180),  		  )
ONE_D_ARG_ONE_D_RETURN_ARG_MUL_ANS_MUL(std::asin, static_cfunc_d_asind_d		,  			 , *(PI_D/180))
ONE_D_ARG_ONE_D_RETURN_ARG_MUL_ANS_MUL(std::acos, static_cfunc_d_acosd_d		,  			 , *(PI_D/180))
ONE_D_ARG_ONE_D_RETURN_ARG_MUL_ANS_MUL(std::atan, static_cfunc_d_atand_d		,  			 , *(PI_D/180))
TWO_D_ARG_ONE_D_RETURN_ARG_MUL_ANS_MUL(std::atan2, static_cfunc_d_atan2d_d_d	,  		     , *(PI_D/180))

ONE_D_ARG_ONE_D_RETURN(std::sinh, static_cfunc_d_sinh_d)
ONE_D_ARG_ONE_D_RETURN(std::cosh, static_cfunc_d_cosh_d)
ONE_D_ARG_ONE_D_RETURN(std::tanh, static_cfunc_d_tanh_d)
ONE_D_ARG_ONE_D_RETURN(std::asinh, static_cfunc_d_asinh_d)
ONE_D_ARG_ONE_D_RETURN(std::acosh, static_cfunc_d_acosh_d)
ONE_D_ARG_ONE_D_RETURN(std::atanh, static_cfunc_d_atanh_d)

ONE_D_ARG_ONE_D_RETURN(std::log, static_cfunc_d_log_d)
ONE_D_ARG_ONE_D_RETURN(std::log10, static_cfunc_d_log10_d)
ONE_D_ARG_ONE_D_RETURN(std::log2, static_cfunc_d_log2_d)

ONE_D_ARG_ONE_D_RETURN(std::ceil, static_cfunc_d_ceil_d)
ONE_D_ARG_ONE_D_RETURN(std::floor, static_cfunc_d_floor_d)
ONE_D_ARG_ONE_D_RETURN(std::trunc, static_cfunc_d_trunc_d)
ONE_D_ARG_ONE_D_RETURN(std::round, static_cfunc_d_round_d)



static expr_value* static_cfunc__printf__(expr_value_list *arg_value_list)
{
    int n_args = arg_value_list->v_val.size();
    if (n_args == 0)
        throw std::runtime_error("No arguments");
    int i_arg = 1;

    int ans = 0;

    std::string format;
    GET_NTH_ARG_AS_STRING(0, format);

    if (n_args == 1) {
        ans = std::printf("%s", format.c_str());
        return new expr_value_double { (double) ans };
    }

    int pos0 = 0;

    pos0 = format.find('%');
    if (pos0 == std::string::npos)
        throw std::runtime_error("Invalid format specifier: " + format);

    if (pos0 == format.size() - 1)
        throw std::runtime_error("Invalid format specifier: " + format);

    int err = 0;
    pos0 = 0;
    int pos1 = 0;
    bool in_percent = false;
    bool in_spec = false;

    for (int i = 0; i < format.size(); i++) {
        char cc = format[i];
        bool is_last_char = i + 1 == format.size();

        if (!in_spec && !in_percent && !is_last_char) {
            if (cc == '%') {
                in_percent = true;
            }
        } else if (in_percent && !in_spec && !is_last_char) {
            if (cc != '%') {
                in_percent = false;
                in_spec = true;
            } else
                /* Escaped % with %% */
                in_percent = false;
        } else {
            /* Next specifier or escaped % */
            if (cc == '%' || is_last_char) {
                if (i_arg == n_args)
                    throw std::runtime_error(
                            "Too few arguments for format specifier 2");
                if (arg_value_list->v_val[i_arg]->type == value_type::DOUBLE) {
                    double d;

                    GET_NTH_ARG_AS_DOUBLE(i_arg, d);
                    if (is_last_char)
                        err = std::printf(format.substr(pos0).c_str(), d);
                    else
                        err = std::printf(format.substr(pos0, i - pos0).c_str(),
                                d);
                    if (err == -1)
                        break;
                    else
                        ans += err;
                } else if (arg_value_list->v_val[i_arg]->type
                        == value_type::STRING) {
                    std::string s;
                    GET_NTH_ARG_AS_STRING(i_arg, s);
                    if (is_last_char)
                        err = std::printf(format.substr(pos0).c_str(),
                                s.c_str());
                    else
                        err = std::printf(format.substr(pos0).c_str(),
                                s.c_str());
                    if (err == -1)
                        break;
                    else
                        ans += err;
                }
                i_arg++;
                in_percent = true;
                pos0 = i;
            }
        }
    }

    if (err == -1)
        return new expr_value_double { (double) -1 };

    if (i_arg != n_args)
        throw std::runtime_error("Too few arguments for format specifier");

    return new expr_value_double { (double) ans };
}

/* Helper function for init_linked_cfunctions */
void register_static_cfunc(std::string name, cfunc_callwrapper fptr)
{
    extern scope_stack scopes;
    auto fobj = new object_static_cfunc { name, fptr };
    scopes.get_top_scope().push_object(fobj);
}

void init_linked_cfunctions()
{
    register_static_cfunc("remainder", static_cfunc_d_remainder_d_d);
    register_static_cfunc("mod", static_cfunc_d_fmod_d_d);
    register_static_cfunc("sqrt", static_cfunc_d_sqrt_d);
    register_static_cfunc("isnan", static_cfunc_d_isnan_d);
    register_static_cfunc("isinf", static_cfunc_d_isinf_d);
    register_static_cfunc("isfinite", static_cfunc_d_isfinite_d);

    register_static_cfunc("sin", static_cfunc_d_sin_d);
    register_static_cfunc("cos", static_cfunc_d_cos_d);
    register_static_cfunc("tan", static_cfunc_d_tan_d);
    register_static_cfunc("asin", static_cfunc_d_asin_d);
    register_static_cfunc("acos", static_cfunc_d_acos_d);
    register_static_cfunc("atan", static_cfunc_d_atan_d);
    register_static_cfunc("atan2", static_cfunc_d_atan2_d_d);

    register_static_cfunc("sind", static_cfunc_d_sind_d);
    register_static_cfunc("cosd", static_cfunc_d_cosd_d);
    register_static_cfunc("tand", static_cfunc_d_tand_d);
    register_static_cfunc("asind", static_cfunc_d_asind_d);
    register_static_cfunc("acosd", static_cfunc_d_acosd_d);
    register_static_cfunc("atand", static_cfunc_d_atand_d);
    register_static_cfunc("atan2d", static_cfunc_d_atan2d_d_d);

    register_static_cfunc("sinh", static_cfunc_d_sinh_d);
    register_static_cfunc("cosh", static_cfunc_d_cosh_d);
    register_static_cfunc("tanh", static_cfunc_d_tanh_d);
    register_static_cfunc("asinh", static_cfunc_d_asinh_d);
    register_static_cfunc("acosh", static_cfunc_d_acosh_d);
    register_static_cfunc("atanh", static_cfunc_d_atanh_d);

    register_static_cfunc("ln", static_cfunc_d_log_d);
    register_static_cfunc("log", static_cfunc_d_log10_d);
    register_static_cfunc("log2", static_cfunc_d_log2_d);

    register_static_cfunc("ceil", static_cfunc_d_ceil_d);
    register_static_cfunc("floor", static_cfunc_d_floor_d);
    register_static_cfunc("trunc", static_cfunc_d_trunc_d);
    register_static_cfunc("round", static_cfunc_d_round_d);

    register_static_cfunc("printf", static_cfunc__printf__);
}

/* Helper function for init_linked_cfunctions */
void register_double_var(std::string name, double d)
{
    extern scope_stack scopes;
    auto obj = new object_double { name, "", d };
    scopes.get_top_scope().push_object(obj);
}

void init_standard_variables()
{
    register_double_var("pi", 3.14159265358979323846);
    register_double_var("e", 2.7182818284590452354);
    register_double_var("nan", NAN);
}

static double rad2deg(double d)
{
    return d * (180 / PI_D);
}

static double deg2rad(double d)
{
    return d * (PI_D / 180);
}

static expr_value* builtin_func_i_print_vardiac(expr_value_list *arg_value_list)
{
    for (auto val : arg_value_list->v_val) {
        if (val->type == value_type::DOUBLE) {
            auto d_val = dynamic_cast<expr_value_double*>(val);
            std::cout << d_val->d;
        } else if (val->type == value_type::INT) {
            auto i_val = dynamic_cast<expr_value_int*>(val);
            std::cout << i_val->i;
        } else if (val->type == value_type::STRING) {
            auto s_val = dynamic_cast<expr_value_string*>(val);
            std::cout << s_val->s;
        } else
            throw std::runtime_error("Invalid type to print");
    }

    delete arg_value_list;

    return new expr_value_int{0};
}

static expr_value* builtin_func_i_printnl_vardiac(expr_value_list *arg_value_list)
{
    builtin_func_i_print_vardiac(arg_value_list);
    std::cout << "\n";

    return new expr_value_int{0};
}

ONE_D_ARG_ONE_D_RETURN(deg2rad, builtin_func_d_deg2rad_d)
ONE_D_ARG_ONE_D_RETURN(rad2deg, builtin_func_d_rad2deg_d)

void init_builtin_functions()
{
    register_static_cfunc("deg2rad", builtin_func_d_deg2rad_d);
    register_static_cfunc("rad2deg", builtin_func_d_rad2deg_d);
    register_static_cfunc("print", builtin_func_i_print_vardiac);
    register_static_cfunc("printnl", builtin_func_i_printnl_vardiac);
}

void init_builtin_types()
{
    extern scope_stack scopes;
    extern scope_stack resolve_scope;

    std::vector<scope_stack*> v = {&scopes, &resolve_scope};

    for (auto e : v) {
        e->get_top_scope().push_type(new type_double);
        e->get_top_scope().push_type(new type_int);
        e->get_top_scope().push_type(new type_string);
    }
}

