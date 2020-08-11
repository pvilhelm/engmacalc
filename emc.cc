

#include "emc.hh"

void scope_stack::push_new_scope()
{
   scope s;
   vec_scope.push_back(s);
}

obj *scope_stack::find_object(std::string name, std::string  nspace)
{
  /* Search backwards so that the top scope matches first. */
  for (auto it = vec_scope.rbegin(); it != vec_scope.rend(); it++)
    {
      obj*p = it->find_object(name, nspace);
      if (p)
        return p;
    }

  return nullptr;
}

expr_value *object_func::feval(expr_value_list *arg_value_list)
{
  extern scope_stack scopes;
  scopes.push_new_scope();
  auto scope = scopes.get_top_scope();
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
    auto obj = new object_double{var_name, "", val->d};
    scopes.get_top_scope().push_object(obj);
  }

  auto func_value = root->eval();
  /* Destroy the function scope. */
  scopes.pop_scope();

  return func_value;
}
