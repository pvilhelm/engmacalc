#include <cmath>
#include <cstdio>
#include <cstdarg>
#include <sstream>
#include <cctype>

#include "emc_assert.hh"
/* Bison and flex requires this include order. */
#include "emc.hh"
typedef void *yyscan_t;
#include "emc.tab.h"
#include "lexer.h"
/* End of stupid include order. */
#include "util_string.hh"

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
    

    return compilation_units.get_current_typestack().find_type(type_name); /* Throws if not found */
}

/* Allocate a new obj by casting from object to type. 
   Throws if the source object value wont fit in the target
   object.
   
   Caller deletes pointer. */
obj* cast_obj_to_type_return_new_obj(obj* obj, emc_type type)
{
/* Macro that checks range and returns a new object with the specified type */
#define MAKE_OBJ_RETURN(obj_class, obj_val_type)\
do {\
    auto obj_t = dynamic_cast<obj_class*>(obj);\
    DEBUG_ASSERT_NOTNULL(obj_t);\
    if (type.is_long()) {\
        check_in_range<obj_val_type, int64_t>(obj_t->val);\
        return new object_long{(int64_t)obj_t->val};\
    } else if (type.is_int()) {\
        check_in_range<obj_val_type, int32_t>(obj_t->val);\
        return new object_int{(int32_t)obj_t->val};\
    } else if (type.is_short()) {\
        check_in_range<obj_val_type, int16_t>(obj_t->val);\
        return new object_short{(int16_t)obj_t->val};\
    } else if (type.is_sbyte()) {\
        check_in_range<obj_val_type, int8_t>(obj_t->val);\
        return new object_sbyte{(int8_t)obj_t->val};\
    } else if (type.is_ulong()) {\
        check_in_range<obj_val_type, uint64_t>(obj_t->val);\
        return new object_ulong{(uint64_t)obj_t->val};\
    } else if (type.is_uint()) {\
        check_in_range<obj_val_type, uint32_t>(obj_t->val);\
        return new object_uint{(uint32_t)obj_t->val};\
    } else if (type.is_ushort()) {\
        check_in_range<obj_val_type, uint16_t>(obj_t->val);\
        return new object_ushort{(uint16_t)obj_t->val};\
    } else if (type.is_byte()) {\
        check_in_range<obj_val_type, uint8_t>(obj_t->val);\
        return new object_byte{(uint8_t)obj_t->val};\
    } else if (type.is_double()) {\
        check_exact_as_double<obj_val_type>(obj_t->val);\
        return new object_double{(double)obj_t->val};\
    } else if (type.is_float()) {\
        check_exact_as_float<obj_val_type>(obj_t->val);\
        return new object_float{(float)obj_t->val};\
    }\
    THROW_BUG("");\
} while((0))

    switch (obj->type) {
    case object_type::LONG:
        MAKE_OBJ_RETURN(object_long, int64_t);
    case object_type::INT:
        MAKE_OBJ_RETURN(object_int, int32_t);
    case object_type::SHORT:
        MAKE_OBJ_RETURN(object_short, int16_t);
    case object_type::SBYTE:
        MAKE_OBJ_RETURN(object_sbyte, int8_t);
    case object_type::ULONG:
        MAKE_OBJ_RETURN(object_ulong, uint64_t);
    case object_type::UINT:
        MAKE_OBJ_RETURN(object_uint, uint32_t);
    case object_type::USHORT:
        MAKE_OBJ_RETURN(object_ushort, uint16_t);
    case object_type::BYTE:
        MAKE_OBJ_RETURN(object_byte, uint8_t);
    case object_type::DOUBLE:
        MAKE_OBJ_RETURN(object_double, double);
    case object_type::FLOAT:
        MAKE_OBJ_RETURN(object_double, float);
    default:
        THROW_BUG("");
    }

#undef MAKE_OBJ_RETURN
}

/* TODO: Should have LOC parameter to do nicer errors */
void verify_obj_fits_in_type(obj* obj, emc_type type)
{
    DEBUG_ASSERT_NOTNULL(obj);
    /* TODO: Verify that ulong and long can be compared like this .. */
    /* TODO: Refactor to a long max and long min, this is stupid. */

#define DO_CHECK(obj_class, ctype) do {\
auto obj_t = dynamic_cast<obj_class*>(obj);\
DEBUG_ASSERT_NOTNULL(obj_t);\
\
if (obj_t->val > std::numeric_limits<ctype>::max() ||\
    obj_t->val < std::numeric_limits<ctype>::lowest())\
    THROW_BUG("Invalid range slipped through to code generation: " <<\
        obj_t->val << " does not fit in a " #ctype );\
} while((0))

    /* TODO: Floats, only if exact conversion maybe? */
    if (type.is_long()) {
        switch (obj->type) {
        case object_type::ULONG:
            DO_CHECK(object_ulong, int64_t);
            break;
        case object_type::LONG:
        case object_type::INT:
        case object_type::SHORT:
        case object_type::SBYTE:
        case object_type::UINT:
        case object_type::USHORT:
        case object_type::BYTE:
            break; /* These all fits */
        default:
            THROW_NOT_IMPLEMENTED("");
        }
    } else if (type.is_int()) {
        switch (obj->type) {
        case object_type::ULONG:
            DO_CHECK(object_ulong, int32_t);
            break;
        case object_type::UINT:
            DO_CHECK(object_uint, int32_t);
            break;
        case object_type::LONG:
            DO_CHECK(object_long, int32_t);
            break;
        case object_type::INT:
        case object_type::SHORT:
        case object_type::SBYTE:
        case object_type::USHORT:
        case object_type::BYTE:
            break; /* These all fits */
        default:
            THROW_NOT_IMPLEMENTED("");
        }
    } else if (type.is_short()) {
        switch (obj->type) {
        case object_type::ULONG:
            DO_CHECK(object_ulong, int16_t);
            break;
        case object_type::UINT:
            DO_CHECK(object_uint, int16_t);
            break;
        case object_type::USHORT:
            DO_CHECK(object_ushort, int16_t);
            break;
        case object_type::LONG:
            DO_CHECK(object_long, int16_t);
            break;
        case object_type::INT:
            DO_CHECK(object_int, int16_t);
            break;
        case object_type::SHORT:
        case object_type::SBYTE:
        case object_type::BYTE:
            break; /* These all fits */
        default:
            THROW_NOT_IMPLEMENTED("");
        }
    } else if (type.is_sbyte()) {
        switch (obj->type) {
        case object_type::ULONG:
            DO_CHECK(object_ulong, int8_t);
            break;
        case object_type::UINT:
            DO_CHECK(object_uint, int8_t);
            break;
        case object_type::USHORT:
            DO_CHECK(object_ushort, int8_t);
            break;
        case object_type::LONG:
            DO_CHECK(object_long, int8_t);
            break;
        case object_type::INT:
            DO_CHECK(object_int, int8_t);
            break;
        case object_type::SHORT:
            DO_CHECK(object_short, int8_t);
            break;
        case object_type::BYTE:
            DO_CHECK(object_byte, int8_t);
            break;
        case object_type::SBYTE:
            break; /* These all fits */
        default:
            THROW_NOT_IMPLEMENTED("");
        }
    } else if (type.is_ulong())  {
        switch (obj->type) {
        case object_type::LONG:
            DO_CHECK(object_long, uint64_t);
            break;
        case object_type::INT:
            DO_CHECK(object_int, uint64_t);
            break;
        case object_type::SHORT:
            DO_CHECK(object_short, uint64_t);
            break;
        case object_type::SBYTE:
            DO_CHECK(object_sbyte, uint64_t);
            break;
        case object_type::BYTE:
        case object_type::ULONG:
        case object_type::UINT:
        case object_type::USHORT:
            break; /* These all fits */
        default:
            THROW_NOT_IMPLEMENTED("");
        }
    } else if (type.is_uint()) {
        switch (obj->type) {
        case object_type::LONG:
            DO_CHECK(object_long, uint32_t);
            break;
        case object_type::INT:
            DO_CHECK(object_int, uint32_t);
            break;
        case object_type::SHORT:
            DO_CHECK(object_short, uint32_t);
            break;
        case object_type::SBYTE:
            DO_CHECK(object_sbyte, uint32_t);
            break;
        case object_type::ULONG:
            DO_CHECK(object_ulong, uint32_t);
            break;
        case object_type::UINT:
        case object_type::USHORT:
        case object_type::BYTE:
            break; /* These all fits */
        default:
            THROW_NOT_IMPLEMENTED("");
        }
    } else if (type.is_ushort()) {
        switch (obj->type) {
        case object_type::LONG:
            DO_CHECK(object_long, uint16_t);
            break;
        case object_type::INT:
            DO_CHECK(object_int, uint16_t);
            break;
        case object_type::SHORT:
            DO_CHECK(object_short, uint16_t);
            break;
        case object_type::SBYTE:
            DO_CHECK(object_sbyte, uint16_t);
            break;
        case object_type::ULONG:
            DO_CHECK(object_ulong, uint16_t);
            break;
        case object_type::UINT:
            DO_CHECK(object_uint, uint16_t);
            break;
        case object_type::USHORT:
        case object_type::BYTE:
            break; /* These all fits */
        default:
            THROW_NOT_IMPLEMENTED("");
        }
    } else if (type.is_byte()) {
        switch (obj->type) {
        case object_type::LONG:
            DO_CHECK(object_long, uint8_t);
            break;
        case object_type::INT:
            DO_CHECK(object_int, uint8_t);
            break;
        case object_type::SHORT:
            DO_CHECK(object_short, uint8_t);
            break;
        case object_type::SBYTE:
            DO_CHECK(object_sbyte, uint8_t);
            break;
        case object_type::ULONG:
            DO_CHECK(object_ulong, uint8_t);
            break;
        case object_type::UINT:
            DO_CHECK(object_uint, uint8_t);
            break;
        case object_type::USHORT:
            DO_CHECK(object_ushort, uint8_t);
            break;
        case object_type::BYTE:
            break; /* These all fits */
        default:
            THROW_NOT_IMPLEMENTED("");
        }
    } 

#undef DO_CHECK
}

void push_dummyobject_to_resolve_scope(std::string var_name, emc_type type)
{
    
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
    compilation_units.get_current_objstack().get_top_scope().push_object(od);
}

emc_type ast_node_funcdef::resolve()
{
    ast_node_typedotnamechain *typedotnamenode = dynamic_cast<ast_node_typedotnamechain*>(typedotnamechain);
    DEBUG_ASSERT_NOTNULL(typedotnamenode);
    typedotnamenode->resolve();

    name = typedotnamenode->name;
    /* Append current namespace to relative namespace if any */
    if (compilation_units.get_current_typestack().current_scope.size())
        if (typedotnamenode->nspace.size())
            nspace = compilation_units.get_current_typestack().current_scope + "." + typedotnamenode->nspace;
        else
            nspace = compilation_units.get_current_typestack().current_scope; 
    else
        nspace = typedotnamenode->nspace;

    parlist->resolve(); /* TODO: Reduntant to parlist_t->resolve()? */
    
    compilation_units.get_current_objstack().push_new_scope();
    auto parlist_t = dynamic_cast<ast_node_vardef_list*>(parlist);
    DEBUG_ASSERT_NOTNULL(parlist_t);
    //parlist_t->resolve();

    /* Create variables with the arguments' names in the function scope. */
    for (int i = 0; i < parlist_t->v_defs.size(); i++) {
        auto par = dynamic_cast<ast_node_def*>(parlist_t->v_defs[i]);
        std::string var_name = par->var_name;

        push_dummyobject_to_resolve_scope(var_name, par->value_type);
    }
    code_block->resolve();
    compilation_units.get_current_objstack().pop_scope();

    /* Hack to allow for return names with same names as other things ... */
    compilation_units.get_current_objstack().push_new_scope();
    return_list->resolve();
    compilation_units.get_current_objstack().pop_scope();

    /* TODO: code_block is probably uneccesary. */
    auto fobj = new object_func { 0, name, nspace,
            parlist->clone() , return_list->clone()};
    /* Resolve the mangled name and write it to this and the function object. */
    /* TODO: ALlow for clinkage, see funcdec */
    mangled_name = mangle_emc_fn_name(*fobj);
    fobj->mangled_name = mangled_name;
    /* Push the function object to top scope */
    compilation_units.get_current_objstack().get_top_scope().push_object(fobj);

    return value_type = emc_type{emc_types::FUNCTION}; /* TODO: Add types too */
}

emc_type ast_node_funcdec::resolve()
{
    ast_node_typedotnamechain *typedotnamenode = dynamic_cast<ast_node_typedotnamechain*>(typedotnamechain);
    DEBUG_ASSERT_NOTNULL(typedotnamenode);
    typedotnamenode->resolve();

    name = typedotnamenode->name;
    /* Append current namespace to relative namespace if any */
    if (compilation_units.get_current_typestack().current_scope.size()) {
        nspace = compilation_units.get_current_typestack().current_scope;
        if (typedotnamenode->nspace.size())
            nspace += "." + typedotnamenode->nspace;
    } else
        nspace = typedotnamenode->nspace;
    
    parlist->resolve();
    
    compilation_units.get_current_objstack().push_new_scope();
    auto parlist_t = dynamic_cast<ast_node_vardef_list*>(parlist);
    DEBUG_ASSERT_NOTNULL(parlist_t);
    parlist_t->resolve();

    compilation_units.get_current_objstack().pop_scope();

    /* Hack to allow for return names with same names as other things ... */
    compilation_units.get_current_objstack().push_new_scope();
    return_list->resolve();
    compilation_units.get_current_objstack().pop_scope();

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
    compilation_units.get_current_objstack().get_top_scope().push_object(fobj);

    return value_type = emc_type{emc_types::FUNCTION}; /* TODO: Add types too */
}

emc_type ast_node_using::resolve()
{
    auto usingchain_t = dynamic_cast<ast_node_usingchain*>(usingchain);
    DEBUG_ASSERT_NOTNULL(usingchain_t);

    usingchain_t->resolve();

    if (usingchain_t->vec_typedotchains.size() > 1)
        THROW_NOT_IMPLEMENTED("");

    std::string path = usingchain_t->vec_typedotchains[0]->resolve_full_type_name();
    
    /* Push the namespace to the stack of "usings" if it was a USING IMPORT */
    if (using_ns) {
        auto &ts = compilation_units.get_current_typestack();
        ts.push_using(path);
    }


    /* See if we already are using the file */
    auto *cup = compilation_units.find_compilation_unit(path);
    std::string dir_path = copy_and_replace_all_substrs(path, ".", "/");
    
    if (cup) {
        THROW_NOT_IMPLEMENTED("");
    } else {
        
        namespace fs = std::filesystem;
        /* TODO: add support for looking for "system headers" in some folders */
        /* TODO: add support for Foo.emh (headers) for refering to some object file */
        bool dir_exists = fs::is_directory(dir_path);
        if (!dir_exists)
            THROW_BUG("Using do not resolve to a directory: " + dir_path);
        /* Look for .em file in that dir */
        std::string file_path = dir_path + "/" + split_last(dir_path, "/") + ".em";
        bool file_exists = fs::is_regular_file(file_path);
        if (!file_exists)
            THROW_BUG("Using do not resolve to a file: " + file_path);

        /* Push the scope and type stacks. */
        extern int curr_line; /* In the lexer TODO: Not globals */
        extern int curr_col;

        int curr_line_poped = curr_line;
        curr_line = 1;
        int curr_col_poped = curr_col;
        curr_col = 1;

        compilation_units.push_compilation_unit(path);
        /* Scan the file and parse it */
        {
            auto &cu = compilation_units.get_current_compilation_unit();

            yyscan_t scanner;
            yylex_init(&scanner);
            auto f = fopen(file_path.c_str(),"r");
            if (!f)
                THROW_BUG("Could not open file: " + file_path);
            yyset_in(f, scanner);

            do {
                int err = yyparse(scanner);
                if (cu.ast_root) {
                    cu.ast_root->resolve();
                    cu.v_nodes.push_back(cu.ast_root);
                    cu.ast_root = nullptr;
                }
                if (err) {
                    yylex_destroy(scanner);
                    THROW_BUG("Cannont parse file properly: " + file_path); 
                }
            } while (!cu.parsed_eol);
            
            // Closes f too
            yylex_destroy(scanner);
        }
        /* Pop the scope and type stacks */
        auto &new_cu = compilation_units.get_current_compilation_unit();
        compilation_units.pop_compilation_unit();
        auto &cu = compilation_units.get_current_compilation_unit();

        /* Link in the type and object stacks from new to current.
           So the linked scopes can be searched by current's type and object
           stacks. */
        cu.objstack.linked_objscope_stacks.push_back(&new_cu.objstack);
        cu.typestack.linked_typescope_stacks.push_back(&new_cu.typestack);

        /* Link this ast_node to the cu so the tree walker can find it */
        compunit = &new_cu;

        curr_line = curr_line_poped;
        curr_col = curr_col_poped;
    }

    return value_type = emc_type{emc_types::NONE};
}

void compilation_unit::clear()
{
    objstack.clear();
    typestack.clear();
    for (auto e : v_nodes)
        delete e;
    v_nodes.clear();
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

emc_type cast_to(const emc_type &a, const emc_types &target)
{
    emc_type ans{target};
    if (!ans.is_primitive())
        THROW_NOT_IMPLEMENTED("");

    ans.is_const_expr = a.is_const_expr;

    return ans;
}

/* Implicit type conversion for native types. */
emc_type standard_type_promotion_or_invalid(const emc_type &a, const emc_type &b)
{
    auto at = a.type;
    auto bt = b.type;

    bool is_const_expr = a.is_const_expr && b.is_const_expr;

    if (at == bt)
        return emc_type{at, is_const_expr};

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
        return emc_type{emc_types::DOUBLE, is_const_expr};
    else if (   at == emc_types::DOUBLE && bt == emc_types::FLOAT || 
                bt == emc_types::DOUBLE && at == emc_types::FLOAT)
        return emc_type{emc_types::DOUBLE, is_const_expr};
    else if (   at == emc_types::FLOAT && b_is_xintish || 
                bt == emc_types::FLOAT && a_is_xintish)
        return emc_type{emc_types::FLOAT, is_const_expr};
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
        return emc_type{emc_types::LONG, is_const_expr};
    else if (   at == emc_types::INT && bt == emc_types::SHORT || 
                bt == emc_types::INT && at == emc_types::SHORT ||
                at == emc_types::INT && bt == emc_types::SBYTE || 
                bt == emc_types::INT && at == emc_types::SBYTE ||
                at == emc_types::INT && bt == emc_types::USHORT || 
                bt == emc_types::INT && at == emc_types::USHORT ||
                at == emc_types::INT && bt == emc_types::BYTE || 
                bt == emc_types::INT && at == emc_types::BYTE)
        return emc_type{emc_types::INT, is_const_expr};
    else if (   at == emc_types::SHORT && bt == emc_types::SBYTE || 
                bt == emc_types::SHORT && at == emc_types::SBYTE ||
                at == emc_types::SHORT && bt == emc_types::BYTE || 
                bt == emc_types::SHORT && at == emc_types::BYTE)
        return emc_type{emc_types::SHORT, is_const_expr};
    else if (   at == emc_types::ULONG && bt == emc_types::UINT || 
                bt == emc_types::ULONG && at == emc_types::UINT ||
                at == emc_types::ULONG && bt == emc_types::USHORT || 
                bt == emc_types::ULONG && at == emc_types::USHORT ||
                at == emc_types::ULONG && bt == emc_types::BYTE || 
                bt == emc_types::ULONG && at == emc_types::BYTE)
        return emc_type{emc_types::ULONG, is_const_expr};
    else if (   at == emc_types::UINT && bt == emc_types::USHORT || 
                bt == emc_types::UINT && at == emc_types::USHORT ||
                at == emc_types::UINT && bt == emc_types::BYTE || 
                bt == emc_types::UINT && at == emc_types::BYTE)
        return emc_type{emc_types::UINT, is_const_expr};
    else if (   at == emc_types::USHORT && bt == emc_types::BYTE || 
                bt == emc_types::USHORT && at == emc_types::BYTE)
        return emc_type{emc_types::USHORT, is_const_expr};
    return emc_type{emc_types::INVALID};
}



void objscope_stack::push_new_scope()
{
    vec_scope.emplace_back(objscope{});
}


void objscope_stack::clear()
{
    for (auto &e : vec_scope)
        e.clear();
}

void objscope_stack::debug_print()
{
    for (auto &scope : vec_scope) {
        std::cout << "Scope:" << std::endl;
        for (auto obj : scope.vec_objs) {
            obj->debug_print();
        }
    }
}

obj* objscope_stack::find_object(std::string name)
{
    
    /* Search backwards so that the top scope matches first. */
    for (auto it = vec_scope.rbegin(); it != vec_scope.rend(); it++) {
        obj *p = it->find_object(name);
        if (p)
            return p;
    }

    /* Search built-in obj scope */
    if (this != &builtin_objstack) {
        obj *p = builtin_objstack.find_object(name);
        if (p)
            return p;
    }

    /* Search in linked scope stacks */
    for (auto *objstack : linked_objscope_stacks) {
        obj *p = objstack->find_object(name);
        if (p)
            return p;
    }

    return nullptr;
}

std::vector<obj*> objscope_stack::find_objects_by_not_mangled_name_helper(std::string name, std::string nspace)
{
    std::vector<obj*> ans;
    /* We are not searching for a namespace match */
    if (nspace.size() == 0) {
        /* Search backwards so that the top scope matches first. */
        for (auto it = vec_scope.rbegin(); it != vec_scope.rend(); it++) {
            auto tmp_v = it->find_objects_by_not_mangled_name(name, "");
            for (auto e : tmp_v)
                ans.push_back(e);
        }
    /* Namespace specified. In that case the object can only be in global scope */
    } else {
        auto tmp_v = get_global_scope().find_objects_by_not_mangled_name(name, nspace);
        for (auto e : tmp_v)
            ans.push_back(e);
    }
    /* Also search the top scope with current namespace prepended to access eg.
     * Foo.Bar.b as b if we are in Foo.Bar */
    if (compilation_units.get_current_typestack().current_scope.size()) {
        std::string current_scope = compilation_units.get_current_typestack().current_scope;
        std::string full_nspace;
        if (nspace.size() && current_scope.size())
            full_nspace = current_scope + "." + nspace;
        else if (nspace.size())
            full_nspace = nspace;
        else if (current_scope.size())
            full_nspace = current_scope;
        else
            full_nspace = "";
        auto tmp_v = get_global_scope().find_objects_by_not_mangled_name(name, full_nspace);
        for (auto e : tmp_v)
            ans.push_back(e);
    }

    /* Search built-in obj scope */
    if (this != &builtin_objstack) {
        auto tmp_v = builtin_objstack.find_objects_by_not_mangled_name(name, nspace);
        for (auto e : tmp_v)
            ans.push_back(e);
    }

    /* Search all linked stacks too */
    for (auto *objstack : linked_objscope_stacks) {
        auto tmp_v = objstack->find_objects_by_not_mangled_name(name, nspace);
        for (auto e : tmp_v)
            ans.push_back(e);
    }

    return ans;
}

emc_type typescope_stack::find_type(std::string name)
{
    emc_type ans;
    bool hit = false;
    /* First search looking from current scope */
    hit = find_type_helper(name, ans);
    if (hit)
        return ans;

    /* Now relook but with the namespace specified by USING directives. */
    auto &ts = compilation_units.get_current_typestack();
    for (auto scopes : ts.using_scopes) {
        for (std::string ns : scopes) {
            if (ns.size()) {
                hit = find_type_helper(ns + "." + name, ans);
                if (hit)
                    return ans;
            }
        }
    }
    THROW_BUG("Could not find type: " + name);
}

std::vector<obj*> objscope_stack::find_objects_by_not_mangled_name(std::string name, std::string nspace)
{
    std::vector<obj*> ans;
    /* First search looking from current scope */
    auto tmp_vec = find_objects_by_not_mangled_name_helper(name, nspace);
    for (auto e : tmp_vec)
        ans.push_back(e);

    /* Now relook but with the namespace specified by USING directives. */
    auto &ts = compilation_units.get_current_typestack();
    for (auto scopes : ts.using_scopes) {
        for (std::string ns : scopes) {
            if (ns.size()) {
                auto tmp_vec = find_objects_by_not_mangled_name_helper(name, ns);
                for (auto e : tmp_vec)
                    ans.push_back(e);
            }
        }
    }

    return ans;
}

void init_linked_cfunctions()
{

}

/* Helper function for init_linked_cfunctions */
void register_double_var(std::string name, double d)
{
    
    auto obj = new object_double {name, "", d, 0};
    compilation_units.get_current_objstack().get_top_scope().push_object(obj);
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
    builtin_typestack.push_type("Byte", {emc_types::BYTE});
    builtin_typestack.push_type("Sbyte", {emc_types::SBYTE});
    builtin_typestack.push_type("Short", {emc_types::SHORT});
    builtin_typestack.push_type("Ushort", {emc_types::USHORT});
    builtin_typestack.push_type("Int", {emc_types::INT});
    builtin_typestack.push_type("Uint", {emc_types::UINT});
    builtin_typestack.push_type("Long", {emc_types::LONG});
    builtin_typestack.push_type("Ulong", {emc_types::ULONG});
    builtin_typestack.push_type("Double", {emc_types::DOUBLE});
    builtin_typestack.push_type("Float", {emc_types::FLOAT});

}



const std::map<emc_types, std::string> map_emc_types_to_mangled_shortversion = {
    {emc_types::LONG, "L"},
    {emc_types::ULONG, "UL"},
    {emc_types::INT, "I"},
    {emc_types::UINT, "UI"},
    {emc_types::SHORT, "H"},
    {emc_types::USHORT, "UH"},
    {emc_types::SBYTE, "SB"},
    {emc_types::BYTE, "UB"},
    {emc_types::BOOL, "B"},
    {emc_types::FLOAT, "F"},
    {emc_types::DOUBLE, "D"},
};

/* Mangles a object with a namespace.
 * E.g. Foo.Bar.i becomes engma_c58b_var_NFooBarN_i */
std::string mangle_emc_var_name(std::string name, std::string nspace)
{
    std::string mangled_name = "engma_c58b_var_";

    if (nspace.size()) {
        /* '_' is used as a delimiter so we need to double them if some is in a string */
        nspace = copy_and_replace_all_substrs(nspace, "_", "__");
        /* Namespaces' '.' can be replaced with just the capital letter as delimiter. */
        nspace = copy_and_replace_all_substrs(nspace, ".", "");
        nspace = "N" + nspace + "N_";
    }
    return mangled_name + nspace + copy_and_replace_all_substrs(name, "_", "__");
}

/* Mangles a Engma function name to its symbol name that will be used for linking.
 *  Note That underscores in name and types are replaces with two underscores.
 *  Namespaces are eg. NamespaceType
        TODO: Hide all type data for structs somewhere as symbols? Eg. engma_type_struct_foo_I_I_I
        TODO: Need to prevent Engma names from ending with _ to not collide whit Foo_._foo 
 */
std::string mangle_emc_fn_name(const object_func &fn_obj)
{
    std::ostringstream ss;
    ss << "engma_c58b_fn";

    /* Begin with the return type. */
    if (fn_obj.var_list->value_type.is_primitive()) {
        ss << "_";
        auto iter = map_emc_types_to_mangled_shortversion.find(fn_obj.var_list->value_type.type);
        if (iter == map_emc_types_to_mangled_shortversion.end())
            THROW_BUG("Could not find mangled short version of type: " + std::to_string((int)fn_obj.var_list->value_type.type));
        /* Add a 'P' per pointer indirection. */
        if (fn_obj.var_list->value_type.n_pointer_indirections)
            for (int i = 0; i < fn_obj.var_list->value_type.n_pointer_indirections; i++)
                ss << "P";
        ss << iter->second;
    } else if (fn_obj.var_list->value_type.is_void()) {
        
    } else
        THROW_NOT_IMPLEMENTED("Mangle of type not impelmented: " + std::to_string((int)fn_obj.var_list->value_type.type));
    
    
    /* Mangle the functions namespace, if any. */
    if (fn_obj.nspace.size()) {
        std::string nspace = fn_obj.nspace;
        /* '_' is used as a delimiter so we need to double them if some is in a string */
        nspace = copy_and_replace_all_substrs(nspace, "_", "__");
        /* Namespaces' '.' can be replaced with just the capital letter as delimiter. */
        nspace = copy_and_replace_all_substrs(nspace, ".", "");
        nspace = "N" + nspace + "N";
        ss << "_" << nspace;
    }

    /* Function name */
    ss << "_" << copy_and_replace_all_substrs(fn_obj.name, "_", "__");
    /* Parameters */
    ast_node_vardef_list *parlist_node = dynamic_cast<ast_node_vardef_list*>(fn_obj.para_list);
    DEBUG_ASSERT_NOTNULL(parlist_node);
    for (ast_node *para : parlist_node->v_defs) {
        if (para->value_type.is_primitive()) {
            ss << "_";
            /* Add a 'P' per pointer indirection. */
            if (para->value_type.n_pointer_indirections)
                for (int i = 0; i < para->value_type.n_pointer_indirections; i++)
                    ss << "P";
            auto iter = map_emc_types_to_mangled_shortversion.find(para->value_type.type);
            if (iter == map_emc_types_to_mangled_shortversion.end())
                THROW_BUG("Could not find mangled short version of type: " + std::to_string((int)para->value_type.type));
            ss << iter->second;
        } else if (para->value_type.is_struct()) {
            DEBUG_ASSERT(para->value_type.mangled_name.size(),"");
            std::string s = copy_and_replace_all_substrs(para->value_type.mangled_name,"engma_c58b_type_","");
            ss << "_S" << s << "S";
        } else
            THROW_NOT_IMPLEMENTED("Mangle of type not impelmented: " + std::to_string((int)para->value_type.type));
    }

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

std::string mangle_emc_type_name(std::string full_path)
{
    std::string mangled_name = "engma_c58b_type_";
    std::string nspace;
    std::string name;
    
    if (full_path.find(".") != std::string::npos) {
        nspace = strip_last(full_path, ".");
        name = split_last(full_path, ".");
    } else {
        name = full_path;
    }

    if (nspace.size()) {
        /* '_' is used as a delimiter so we need to double them if some is in a string */
        nspace = copy_and_replace_all_substrs(nspace, "_", "__");
        /* Namespaces' '.' can be replaced with just the capital letter as delimiter. */
        nspace = copy_and_replace_all_substrs(nspace, ".", "");
        nspace = "N" + nspace + "N_";
    }
    return mangled_name + nspace + copy_and_replace_all_substrs(name, "_", "__");
}