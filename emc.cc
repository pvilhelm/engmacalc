#include <cmath>
#include <cstdio>
#include <cstdarg>
#include <sstream>
#include <cctype>

#include "emc_assert.hh"
#include "emc.hh"

#ifndef NDEBUG
int ast_node_count;
int value_expr_count;
#endif

emc_type string_to_type(std::string type_name) {
    if (type_name == "Int")
        return emc_type{emc_types::INT};
    else if (type_name == "Uint")
        return emc_type{emc_types::UINT};
    else if (type_name == "Short")
        return emc_type{emc_types::SHORT};
    else if (type_name == "Ushort")
        return emc_type{emc_types::USHORT};
    else if (type_name == "Byte")
        return emc_type{emc_types::BYTE};
    else if (type_name == "Sbyte")
        return emc_type{emc_types::SBYTE};
    else if (type_name == "Long")
        return emc_type{emc_types::LONG};
    else if (type_name == "Ulong")
        return emc_type{emc_types::ULONG};
    else if (type_name == "Double")
        return emc_type{emc_types::DOUBLE};
    else if (type_name == "Float")
        return emc_type{emc_types::FLOAT};
    else if (type_name == "Bool")
        return emc_type{emc_types::BOOL};
    else if (type_name == "String")
        return emc_type{emc_types::STRING};
    
    /* See if it is a user defined type */
    extern scope_stack resolve_scope;

    return resolve_scope.find_type(type_name); /* Throws if not found */
}

void push_dummyobject_to_resolve_scope(std::string var_name, emc_type type)
{
    extern scope_stack resolve_scope;
    obj *od = nullptr;
    if (type.is_double())
        od = new object_double{var_name, 0, type.n_pointer_indirections};
    else if (type.is_float())
        od = new object_float{var_name, 0, type.n_pointer_indirections};
    else if (type.is_long())
        od = new object_long{var_name, 0, type.n_pointer_indirections};
    else if (type.is_int())
        od = new object_int{var_name, 0, type.n_pointer_indirections};
    else if (type.is_short())
        od = new object_short{var_name, 0, type.n_pointer_indirections};
    else if (type.is_sbyte())
        od = new object_sbyte{var_name, 0, type.n_pointer_indirections};
    else if (type.is_bool())
        od = new object_bool{var_name, 0, type.n_pointer_indirections};
    else if (type.is_ulong())
        od = new object_ulong{var_name, 0, type.n_pointer_indirections};
    else if (type.is_uint())
        od = new object_uint{var_name, 0, type.n_pointer_indirections};
    else if (type.is_ushort())
        od = new object_ushort{var_name, 0, type.n_pointer_indirections};
    else if (type.is_byte())
        od = new object_byte{var_name, 0, type.n_pointer_indirections};
    else if (type.is_struct())
        od = new object_struct{var_name, "", type, type.n_pointer_indirections};
    else
        THROW_NOT_IMPLEMENTED("Type not implemented: " + var_name);
    resolve_scope.get_top_scope().push_object(od);
}

emc_type ast_node_funcdef::resolve()
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

        push_dummyobject_to_resolve_scope(var_name, par->value_type);
    }
    code_block->resolve();
    resolve_scope.pop_scope();

    /* Hack to allow for return names with same names as other things ... */
    resolve_scope.push_new_scope();
    return_list->resolve();
    resolve_scope.pop_scope();

    /* TODO: code_block is probably uneccesary. */
    auto fobj = new object_func { code_block->clone(), name, nspace,
            parlist->clone() , return_list->clone()};
    /* Resolve the mangled name and write it to this and the function object. */
    mangled_name = mangle_emc_fn_name(*fobj);
    fobj->mangled_name = mangled_name;
    /* Push the function object to top scope */
    resolve_scope.get_top_scope().push_object(fobj);

    return value_type = emc_type{emc_types::FUNCTION}; /* TODO: Add types too */
}

emc_type ast_node_funcdec::resolve()
{
    extern scope_stack resolve_scope;
    parlist->resolve();
    
    resolve_scope.push_new_scope();
    auto parlist_t = dynamic_cast<ast_node_vardef_list*>(parlist);
    DEBUG_ASSERT_NOTNULL(parlist_t);
    parlist_t->resolve();

    resolve_scope.pop_scope();

    /* Hack to allow for return names with same names as other things ... */
    resolve_scope.push_new_scope();
    return_list->resolve();
    resolve_scope.pop_scope();

    auto fobj = new object_func { 0, name, nspace,
            parlist->clone() , return_list->clone()};

    
    /* TODO: mangle_emc_fn_name() borde ta parlist, name och returnlist och fobj borde
     * få sitt manglade namn i sin ctor. */

    /* If c linkage is specified there is no name mangleing. */
    if (c_linkage) { 
        fobj->c_linkage = true;
        fobj->mangled_name = name;
        mangled_name = name;
    /* Resolve the mangled name and write it to this and the function object. */
    } else {
        mangled_name = mangle_emc_fn_name(*fobj);
        fobj->mangled_name = mangled_name;
    }
    /* Push the function object to top scope */
    /* TODO: Declarations collide with definitions. Should be some flag. */
    resolve_scope.get_top_scope().push_object(fobj);

    return value_type = emc_type{emc_types::FUNCTION}; /* TODO: Add types too */
}

emc_type ast_node_vardef_list::resolve()
{
    for (auto e : v_defs) {
        auto ee = dynamic_cast<ast_node_def*>(e);
        emc_type value = ee->resolve_no_push();
    }

    if (v_defs.size() == 0)
        return value_type = emc_type{emc_types::NONE};
    else
        return value_type = v_defs.front()->value_type;
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
    if (!ans.is_valid())
        THROW_BUG("emc_types has no valid standard promotion");
    return ans;
}

/* Implicit type conversion for native types. */
emc_type standard_type_promotion_or_invalid(const emc_type &a, const emc_type &b)
{
    auto at = a.type;
    auto bt = b.type;

    if (at == bt)
        return emc_type{at};

    bool a_is_intish = at == emc_types::INT || at == emc_types::SHORT || 
                        at == emc_types::LONG || at == emc_types::SBYTE;
    bool b_is_intish = bt == emc_types::INT || bt == emc_types::SHORT || 
                        bt == emc_types::LONG || bt == emc_types::SBYTE;
    bool a_is_uintish = at == emc_types::UINT || at == emc_types::USHORT || 
                        at == emc_types::ULONG || at == emc_types::BYTE;
    bool b_is_uintish = bt == emc_types::UINT || bt == emc_types::USHORT || 
                        bt == emc_types::ULONG || bt == emc_types::BYTE;
    bool a_is_xintish = a_is_intish || a_is_uintish;                                         
    bool b_is_xintish = b_is_intish || b_is_uintish;

    /* TODO: Make a array table instead maybe? */
    if (        at == emc_types::DOUBLE && b_is_xintish || 
                bt == emc_types::DOUBLE && a_is_xintish)
        return emc_type{emc_types::DOUBLE};
    else if (   at == emc_types::DOUBLE && bt == emc_types::FLOAT || 
                bt == emc_types::DOUBLE && at == emc_types::FLOAT)
        return emc_type{emc_types::DOUBLE};
    else if (   at == emc_types::FLOAT && b_is_xintish || 
                bt == emc_types::FLOAT && a_is_xintish)
        return emc_type{emc_types::FLOAT};
    else if (   at == emc_types::LONG && bt == emc_types::INT || 
                bt == emc_types::LONG && at == emc_types::INT ||
                at == emc_types::LONG && bt == emc_types::SHORT || 
                bt == emc_types::LONG && at == emc_types::SHORT ||
                at == emc_types::LONG && bt == emc_types::SBYTE || 
                bt == emc_types::LONG && at == emc_types::SBYTE ||
                at == emc_types::LONG && bt == emc_types::UINT || 
                bt == emc_types::LONG && at == emc_types::UINT ||
                at == emc_types::LONG && bt == emc_types::USHORT || 
                bt == emc_types::LONG && at == emc_types::USHORT ||
                at == emc_types::LONG && bt == emc_types::BYTE || 
                bt == emc_types::LONG && at == emc_types::BYTE)
        return emc_type{emc_types::LONG};
    else if (   at == emc_types::INT && bt == emc_types::SHORT || 
                bt == emc_types::INT && at == emc_types::SHORT ||
                at == emc_types::INT && bt == emc_types::SBYTE || 
                bt == emc_types::INT && at == emc_types::SBYTE ||
                at == emc_types::INT && bt == emc_types::USHORT || 
                bt == emc_types::INT && at == emc_types::USHORT ||
                at == emc_types::INT && bt == emc_types::BYTE || 
                bt == emc_types::INT && at == emc_types::BYTE)
        return emc_type{emc_types::INT};
    else if (   at == emc_types::SHORT && bt == emc_types::SBYTE || 
                bt == emc_types::SHORT && at == emc_types::SBYTE ||
                at == emc_types::SHORT && bt == emc_types::BYTE || 
                bt == emc_types::SHORT && at == emc_types::BYTE)
        return emc_type{emc_types::SHORT};
    else if (   at == emc_types::ULONG && bt == emc_types::UINT || 
                bt == emc_types::ULONG && at == emc_types::UINT ||
                at == emc_types::ULONG && bt == emc_types::USHORT || 
                bt == emc_types::ULONG && at == emc_types::USHORT ||
                at == emc_types::ULONG && bt == emc_types::BYTE || 
                bt == emc_types::ULONG && at == emc_types::BYTE)
        return emc_type{emc_types::ULONG};
    else if (   at == emc_types::UINT && bt == emc_types::USHORT || 
                bt == emc_types::UINT && at == emc_types::USHORT ||
                at == emc_types::UINT && bt == emc_types::BYTE || 
                bt == emc_types::UINT && at == emc_types::BYTE)
        return emc_type{emc_types::UINT};
    else if (   at == emc_types::USHORT && bt == emc_types::BYTE || 
                bt == emc_types::USHORT && at == emc_types::BYTE)
        return emc_type{emc_types::USHORT};
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

void scope_stack::push_new_scope()
{
    vec_scope.emplace_back(scope{});
}


void scope_stack::clear()
{
    for (auto &e : vec_scope)
        e.clear();
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

obj* scope_stack::find_object(std::string name, std::string nspace)
{
    /* Search backwards so that the top scope matches first. */
    for (auto it = vec_scope.rbegin(); it != vec_scope.rend(); it++) {
        obj *p = it->find_object(name, nspace);
        if (p)
            return p;
    }

    return nullptr;
}

std::vector<obj*> scope_stack::find_objects_by_not_mangled_name(std::string name, std::string nspace)
{
    std::vector<obj*> ans;
    /* Search backwards so that the top scope matches first. */
    for (auto it = vec_scope.rbegin(); it != vec_scope.rend(); it++) {
        auto tmp_v = it->find_objects_by_not_mangled_name(name, nspace);
        for (auto e : tmp_v)
            ans.push_back(e);
    }

    return ans;
}

emc_type scope_stack::find_type(std::string name)
{
    auto it = map_typename_to_type.find(name);
    ASSERT(it != map_typename_to_type.end(), "Type not found: " + name);
        
    return it->second;
}

void init_linked_cfunctions()
{

}

/* Helper function for init_linked_cfunctions */
void register_double_var(std::string name, double d)
{
    extern scope_stack resolve_scope;
    auto obj = new object_double {name, "", d, 0};
    resolve_scope.get_top_scope().push_object(obj);
}

void init_standard_variables()
{   /* TODO: Fixa så att detta görs som ast nod kanske? */
    /*
    register_double_var("pi", 3.14159265358979323846);
    register_double_var("e", 2.7182818284590452354);
    register_double_var("nan", NAN); 
    */
}

void init_builtin_functions()
{

}

void init_builtin_types()
{
    extern scope_stack resolve_scope;
 
    resolve_scope.push_type("Byte", {emc_types::BYTE});
    resolve_scope.push_type("Sbyte", {emc_types::SBYTE});
    resolve_scope.push_type("Short", {emc_types::SHORT});
    resolve_scope.push_type("Ushort", {emc_types::USHORT});
    resolve_scope.push_type("Int", {emc_types::INT});
    resolve_scope.push_type("Uint", {emc_types::UINT});
    resolve_scope.push_type("Long", {emc_types::LONG});
    resolve_scope.push_type("Ulong", {emc_types::ULONG});
    resolve_scope.push_type("Double", {emc_types::DOUBLE});
    resolve_scope.push_type("Float", {emc_types::FLOAT});
}

std::string copy_and_replace_all_substrs(const std::string &source, const std::string &from, const std::string &to)
{
    std::string ans;
    ans.reserve(source.length());

    std::string::size_type lastPos = 0;
    std::string::size_type findPos;

    while(std::string::npos != (findPos = source.find(from, lastPos)))
    {
        ans.append(source, lastPos, findPos - lastPos);
        ans += to;
        lastPos = findPos + from.length();
    }

    ans += source.substr(lastPos);
    
    return ans;
}

const std::map<emc_types, std::string> map_emc_types_to_mangled_shortversion = {
    {emc_types::LONG, "L"},
    {emc_types::ULONG, "Ul"},
    {emc_types::INT, "I"},
    {emc_types::UINT, "U"},
    {emc_types::SHORT, "S"},
    {emc_types::USHORT, "Us"},
    {emc_types::SBYTE, "Sb"},
    {emc_types::BYTE, "Ub"},
    {emc_types::BOOL, "B"},
    {emc_types::FLOAT, "F"},
    {emc_types::DOUBLE, "D"},
};

/* Mangles a Engma function name to its symbol name that will be used for linking.
 *  Note That underscores in name and types are replaces with two underscores.
 *  Namespaces are eg. NamespaceType
        TODO: Hide all type data for structs somewhere as symbols? Eg. engma_type_struct_foo_I_I_I
 */
std::string mangle_emc_fn_name(const object_func &fn_obj)
{
    std::ostringstream ss;
    ss << "engma_c58b_fn";

    /* Begin with the return type. */
    if (fn_obj.var_list->value_type.is_primitive()) {
        auto iter = map_emc_types_to_mangled_shortversion.find(fn_obj.var_list->value_type.type);
        if (iter == map_emc_types_to_mangled_shortversion.end())
            THROW_BUG("Could not find mangled short version of type: " + std::to_string((int)fn_obj.var_list->value_type.type));
        ss << "_" << iter->second;
    } else if (fn_obj.var_list->value_type.is_void()) {
        ss << "_" << "N";
    } else
        THROW_NOT_IMPLEMENTED("Mangle of type not impelmented: " + std::to_string((int)fn_obj.var_list->value_type.type));
    
    /* Function name */
    ss << "_" << copy_and_replace_all_substrs(fn_obj.name, "_", "__");
    /* Parameters */
    ast_node_vardef_list *parlist_node = dynamic_cast<ast_node_vardef_list*>(fn_obj.para_list);
    DEBUG_ASSERT_NOTNULL(parlist_node);
    for (ast_node *para : parlist_node->v_defs) {
        if (para->value_type.is_primitive()) {
            auto iter = map_emc_types_to_mangled_shortversion.find(para->value_type.type);
            if (iter == map_emc_types_to_mangled_shortversion.end())
                THROW_BUG("Could not find mangled short version of type: " + std::to_string((int)para->value_type.type));
            ss << "_" << iter->second;
        } else
            THROW_NOT_IMPLEMENTED("Mangle of type not impelmented: " + std::to_string((int)para->value_type.type));
    }

    /* Sentinel token to allow for future expansion */
    ss << "_";

    return ss.str();
}

std::string demangle_emc_fn_name(std::string c_fn_name)
{
/* TODO: Do if needed. */
#if 0
    std::ostringstream ss;

    /* Magic sequence */
    size_t pos = c_fn_name.find("engma_c58b_fn_");

    if (pos != 0)
        throw std::runtime_error("Function name '" + c_fn_name + "' is not a mangled Engma name");

    pos += std::string{"engma_c58b_fn_"}.size();
    if (pos == c_fn_name.size())
        throw std::runtime_error("Function name '" + c_fn_name + "' is not a mangled Engma name");
    /* Search for name, which will be starting with lowercase. */
    bool in_token = false;
    for (; pos < c_fn_name.size(); pos++) {
        char c = c_fn_name[pos];
        if (in_token) {
            if (c == '_'
        } else {
            
        }
    }

    return ss.str();  
#endif
    THROW_NOT_IMPLEMENTED("");  
}