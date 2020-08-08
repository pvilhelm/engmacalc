

#include "emc.hh"

void scope_stack::push_new_scope()
{
   vec_scope.push_back((scope){});
}

obj *scope_stack::find_object (std::string name, std::string  nspace)
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
