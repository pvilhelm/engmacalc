#pragma once

#include <stdexcept>
#include <string>
#include <vector>
#include <iostream>
#include <cmath>

enum class ast_type {
                     DOUBLE_LITERAL = 1,
                     DOUBLE_ADD,
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
};

enum class value_type {
                            DOUBLE = 1,
                            RVAL,
                            LVAL,
                            LIST
};

enum class object_type {
                        DOUBLE = 1,
                        FUNC,
};

/* Forward declarations. */
class scope;
class ast_node;
class expr_value;
class expr_value_double;


class obj {
public:
  virtual ~obj() {};
  virtual expr_value* eval() = 0;
  std::string name;
  std::string nspace;
  object_type type;
};


class scope;

class scope_stack {
public:
  void push_new_scope();
  void pop_scope() { vec_scope.pop_back(); }
  scope &get_top_scope() { return vec_scope.back(); }  
  obj *find_object(std::string name, std::string  nspace);
  
  std::vector<scope> vec_scope;
};

class scope {
public:
  ~scope()
  {
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
  void pop_object() { vec_objs.pop_back(); }
  obj *find_object(std::string name, std::string nspace)
  {
    for (auto p : vec_objs)
      if (p->name == name && p->nspace == nspace)
        return p;
    return nullptr;
  }
  std::vector<obj*> vec_objs;
};

/* Base class of the value of an evaluated expression. */
class expr_value {
public:
  virtual ~expr_value() {};
  virtual expr_value* clone() = 0;
  value_type type;
};

class expr_value_list : public expr_value {
public:
  expr_value_list() { type = value_type::LIST; }
  ~expr_value_list() { for (auto p : v_val) delete p; }
  /* The expression value list takes ownership of the pointer. */
  void append_value(expr_value *val) { v_val.push_back(val); }
  
  expr_value *clone()
  {
    auto ret = new expr_value_list;
    for (auto &e : v_val)
      ret->v_val.push_back(e->clone());
    return ret;
  }
  std::vector<expr_value*> v_val;
};

class expr_value_double : public expr_value {
public:
  expr_value_double()
  { type = value_type::DOUBLE; };
  expr_value_double(double dd)
  { type = value_type::DOUBLE; d = dd;}
  expr_value_double(std::string s)
  { type = value_type::DOUBLE; d = stod(s);}
  ~expr_value_double() {}

  double d;
  
  expr_value *clone()
  {
    return new expr_value_double{d};
  }
};


class object_double : public obj {
public:
  ~object_double(){};

  object_double(std::string name, std::string nspace, double val)
    : d(val)
  { type = object_type::DOUBLE; this->name = name; this->nspace = nspace;};
  object_double() : d(0.0) {}
  object_double(std::string name, double val)
    : object_double(name, "", val) {}


  expr_value *eval()
  {
    return new expr_value_double{d};
  }
  
  double d;
};

class object_func : public obj {
public:
  ~object_func(){}
  object_func(ast_node *root, std::string name,
              std::string nspace, ast_node *para_list)
    : root(root) , para_list(para_list)
  { type = object_type::FUNC; this->name = name; this->nspace = nspace;}

  /* The ast node funcdec owns this pointer. */
  ast_node *root;
  ast_node *para_list;
  expr_value *eval() { throw std::runtime_error("func eval");}

  /* Evaluate the function. */
  expr_value *feval(expr_value_list *arg_value_list);
};

class expr_value_lval : public expr_value {
public:
  /* Does not own object. */
  obj *object;

  expr_value_lval(obj *obj)
    : object(obj) { type = value_type::LVAL; }

  expr_value *clone()
  {
    return new expr_value_lval{object};
  }
};

/* Base class for a node in the abstract syntax tree. */
class ast_node {
public:
  virtual ~ast_node() {};
  
  virtual expr_value *eval() = 0;

  ast_type type;  
};

class ast_node_double_literal : public ast_node {
public:
  ast_node_double_literal()
  { type = ast_type::DOUBLE_LITERAL; }
  ast_node_double_literal(double d) : d(d)
  { type = ast_type::DOUBLE_LITERAL; }
  ast_node_double_literal(std::string s)
  { type = ast_type::DOUBLE_LITERAL; d = stof(s); }
  ~ast_node_double_literal() {}

  double d;
  
  expr_value *eval()
  {
    auto evd = new expr_value_double{d};

    return evd;
  }

};

class ast_node_add : public ast_node {
public:
  ast_node_add() : ast_node_add(nullptr, nullptr) {}
  ast_node_add(ast_node *first, ast_node *sec) :
    first(first) , sec(sec) { type = ast_type::DOUBLE_ADD;}
  
  ~ast_node_add()
  {
    if (first) delete first;
    if (sec) delete sec;
  }

  expr_value *eval()
  {
    auto fv = first->eval();
    auto sv = sec->eval();

    if (fv->type == value_type::DOUBLE
        && sv->type == value_type::DOUBLE) {
          auto d_fv = dynamic_cast<expr_value_double*>(fv);
          auto d_sv = dynamic_cast<expr_value_double*>(sv);    

          double ans = d_fv->d + d_sv->d;

          delete sv;
          delete fv;
          
          return new expr_value_double{ans};
    } else {
      delete sv;
      delete fv;
      throw std::runtime_error("AST node add:  types not implemented");
    }

  }
  
  ast_node *first;
  ast_node *sec;
};

class ast_node_sub : public ast_node {
public:
  ast_node_sub() : ast_node_sub(nullptr, nullptr) {}
  ast_node_sub(ast_node *first, ast_node *sec) :
    first(first) , sec(sec) { type = ast_type::DOUBLE_SUB;}
  
  ~ast_node_sub()
  {
    if (first) delete first;
    if (sec) delete sec;
  }

  expr_value *eval()
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
          
          return new expr_value_double{ans};
    } else {
      delete fv;
      delete sv;
      throw std::runtime_error("AST node sub:  types not implemented");
    }

  }
  
  ast_node *first;
  ast_node *sec;
};

class ast_node_mul : public ast_node {
public:
  ast_node_mul() : ast_node_mul(nullptr, nullptr) {}
  ast_node_mul(ast_node *first, ast_node *sec) :
    first(first) , sec(sec) { type = ast_type::DOUBLE_MUL;}
  
  ~ast_node_mul()
  {
    if (first) delete first;
    if (sec) delete sec;
  }

  expr_value *eval()
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
          
          return new expr_value_double{ans};
    } else {
      delete fv;
      delete sv;
      throw std::runtime_error("AST node mul:  types not implemented");
    }

  }
  
  ast_node *first;
  ast_node *sec;
};

class ast_node_rdiv : public ast_node {
public:
  ast_node_rdiv() : ast_node_rdiv(nullptr, nullptr) {}
  ast_node_rdiv(ast_node *first, ast_node *sec) :
    first(first) , sec(sec) { type = ast_type::DOUBLE_RDIV;}
  
  ~ast_node_rdiv()
  {
    if (first) delete first;
    if (sec) delete sec;
  }

  expr_value *eval()
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

          return new expr_value_double{ans};
    } else {
      delete fv;
      delete sv;
      throw std::runtime_error("AST node rdiv:  types not implemented");
    }

  }
  
  ast_node *first;
  ast_node *sec;
};

class ast_node_pow : public ast_node {
public:
  ast_node_pow() : ast_node_pow(nullptr, nullptr) {}
  ast_node_pow(ast_node *first, ast_node *sec) :
    first(first) , sec(sec) { type = ast_type::POW;}
  
  ~ast_node_pow()
  {
    if (first) delete first;
    if (sec) delete sec;
  }

  expr_value *eval()
  {
    auto fv = first->eval();
    auto sv = sec->eval();

    if (fv->type == value_type::DOUBLE
        && sv->type == value_type::DOUBLE) {
          auto d_fv = dynamic_cast<expr_value_double*>(fv);
          auto d_sv = dynamic_cast<expr_value_double*>(sv);    

          double ans = pow(d_fv->d , d_sv->d);

          delete fv;
          delete sv;

          return new expr_value_double{ans};
    } else {
      delete fv;
      delete sv;
      throw std::runtime_error("AST node pow:  types not implemented");
    }

  }
  
  ast_node *first;
  ast_node *sec;
};


class ast_node_abs : public ast_node {
public:
  ast_node_abs(ast_node *first) :
    first(first) {type = ast_type::ABS; }
  ~ast_node_abs()
  { if (first) delete first; }

  ast_node *first;
  
  expr_value *eval()
  {
    auto fv = first->eval();

    if (fv->type == value_type::DOUBLE) {
          auto d_fv = dynamic_cast<expr_value_double*>(fv);
  
          double ans = d_fv->d < 0 ? -d_fv->d : d_fv->d;

          return new expr_value_double{ans};
    } else
      throw std::runtime_error("AST node unary minus types not implemented");
  }
};

class ast_node_uminus : public ast_node {
public:
  ast_node_uminus() : ast_node_uminus(nullptr) {}
  ast_node_uminus(ast_node *first) :
    first(first) { type = ast_type::DOUBLE_UMINUS;}
  
  ~ast_node_uminus()
  {
    if (first) delete first;
  }

  expr_value *eval()
  {
    auto fv = first->eval();

    if (fv->type == value_type::DOUBLE) {
          auto d_fv = dynamic_cast<expr_value_double*>(fv);
  
          double ans = -d_fv->d;

          return new expr_value_double{ans};
    } else
      throw std::runtime_error("AST node unary minus types not implemented");

  }
  
  ast_node *first;
};


class ast_node_var : public ast_node {
public:
  std::string name;
  std::string nspace;

  ~ast_node_var(){}
  ast_node_var(std::string name, std::string nspace = "")
   : name(name), nspace(nspace) { type = ast_type::VAR; }
  ast_node_var(): ast_node_var("",""){}
 
  
  
  expr_value *eval()
  {
    extern scope_stack scopes;
    auto p = scopes.find_object(name, nspace);
    if (!p)
      throw std::runtime_error("Object does not exist: >>" +
                               nspace + name + "<<");
    return p->eval();
  }

  expr_value *leval()
  {
    extern scope_stack scopes;
    auto p = scopes.find_object(name, nspace);
    return new expr_value_lval{p};
  }
};

class ast_node_assign : public ast_node {
public:
  ~ast_node_assign()
  {
    if (first) delete first;
    if (sec) delete sec;
  }
  ast_node_assign(ast_node *first, ast_node *sec)
    : first(first), sec(sec) { type = ast_type::ASSIGN;}

  expr_value *eval()
  {
    if (first->type != ast_type::VAR)
      throw std::runtime_error("Left hand of assigned not lvalue");
    auto lvar = dynamic_cast<ast_node_var*>(first);
    expr_value *lval_ = lvar->leval();
    if (lval_ == nullptr || lval_->type != value_type::LVAL)
      throw std::runtime_error("ast_node_assign bugg");
    auto lval = dynamic_cast<expr_value_lval*>(lval_);

    /* If the obj doesnt exist we create it. */
    if (!lval->object) {
      extern scope_stack scopes;
      
      auto obj = new object_double{lvar->name, lvar->nspace, 0.};
      scopes.get_top_scope().push_object(obj);
      lval->object = obj;
    } 

    auto rval = sec->eval();

    if (rval->type != value_type::DOUBLE)
      throw std::runtime_error("ast_node_assign Right type not implemented");
    if (lval->object->type != object_type::DOUBLE)
      throw std::runtime_error("ast_node_assign Left type not implemented");

    auto rvald = dynamic_cast<expr_value_double*>(rval);
    auto objd = dynamic_cast<object_double*>(lval->object);
    objd->d = rvald->d;
    
    delete rval;
    delete lval;

    return new expr_value_double(objd->d);
  }
  
  ast_node *first;
  ast_node *sec;
};

class ast_node_cmp : public ast_node {
public:
  ast_node_cmp() : ast_node_cmp(nullptr, nullptr) {}
  ast_node_cmp(ast_node *first, ast_node *sec) :
    first(first) , sec(sec) { type = ast_type::CMP;}
  
  ~ast_node_cmp()
  {
    if (first) delete first;
    if (sec) delete sec;
  }

  expr_value *eval()
  {
    auto fv = first->eval();
    auto sv = sec->eval();

    if (fv->type == value_type::DOUBLE
        && sv->type == value_type::DOUBLE) {
          auto d_fv = dynamic_cast<expr_value_double*>(fv);
          auto d_sv = dynamic_cast<expr_value_double*>(sv);    
          double ans;
          if (d_fv->d < d_sv->d)
            ans = -1;
          else if (d_fv->d > d_sv->d)
            ans = 1;
          else
            ans = 0;

          delete fv;
          delete sv;
          
          return new expr_value_double{ans};
    } else {
      delete fv;
      delete sv;
      throw std::runtime_error("AST node cmp:  types not implemented");
    }

  }
  
  ast_node *first;
  ast_node *sec;
};


class ast_node_temporary : public ast_node {
public:
  ast_node *first;
  expr_value *val = nullptr;

  ast_node_temporary(ast_node *first) : first(first) {}
  ~ast_node_temporary() { if (val) delete val; }

  expr_value *eval()
  {
    if (!val)
      val = first->eval();
    if (!val)
      throw std::runtime_error("temp bugg null val");
    return val->clone();
  }
  
};

class ast_node_explist : public ast_node {
public:
  ast_node_explist(ast_node *first)
  {
    v_nodes.push_back(first);
    type = ast_type::EXPLIST;
  }
  ~ast_node_explist()
  {
    for (auto e : v_nodes)
      delete e;
  }
  void append_node(ast_node *first)
  {
    v_nodes.push_back(first);
  }

  expr_value *eval()
  {
    expr_value *ans = 0;
    for (auto p : v_nodes)
      ans = p->eval();
    return ans;
  }
  
  std::vector<ast_node*> v_nodes;
};

class ast_node_doblock : public ast_node {
public:
  ast_node *first;

  ast_node_doblock(ast_node *first) : first(first) {}
  ~ast_node_doblock() { if (first) delete first; }

  expr_value *eval()
  {
    extern scope_stack scopes;
    
    scopes.push_new_scope();
    auto val = first->eval();
    scopes.pop_scope();
    return val;
  }
};

class ast_node_if : public ast_node {
public:
  ast_node *cond_e;
  ast_node *if_el;
  ast_node *else_el;
  ast_node_if(ast_node *cond_e, ast_node *if_el) :
    ast_node_if(cond_e, if_el, nullptr) {}
  ast_node_if(ast_node *cond_e, ast_node *if_el, ast_node *else_el):
    cond_e(cond_e), if_el(if_el), else_el(else_el) {}
  ~ast_node_if()
  {
    if (cond_e) delete cond_e;
    if (if_el) delete if_el;
    if (else_el) delete else_el;
  }

  expr_value *eval()
  {
    extern scope_stack scopes;
    
    auto ev_cond = cond_e->eval();
    if (ev_cond->type != value_type::DOUBLE)
      throw std::runtime_error("ast_node_if Cond type not implemented");
    auto ed = dynamic_cast<expr_value_double*>(ev_cond);

    expr_value *ans;
    
    if (ed->d) {
      scopes.push_new_scope();
      ans = if_el->eval();
      scopes.pop_scope();
    } else if (else_el) {
      scopes.push_new_scope();      
      ans = else_el->eval();
      scopes.pop_scope();
    } else
      ans = new expr_value_double{0.};

    delete ev_cond;

    return ans;
  }
};

class func_para {
public:
  ~func_para(){ if (val) delete val;}
  func_para(std::string name) : name(name) {}
  std::string name;
  expr_value *val = 0;
};

/* Class for a list of named definitions in a function definition. */
class ast_node_parlist : public ast_node {
public:
  ast_node_parlist() {}

  void append_parameter(func_para para)
  { v_func_paras.push_back(para); }

  /* Parameters, the defintion of the call signature. */
  std::vector<func_para> v_func_paras;
 
  expr_value *eval()
  { 
    throw std::runtime_error("Can't eval arglist.");
  }
};

class ast_node_arglist : public ast_node {
public:
  ast_node_arglist() {}
  ~ast_node_arglist()
  {
    for (auto p : v_ast_args)
      delete p;
  }
  void append_arg(ast_node *arg) { v_ast_args.push_back(arg);}

  expr_value *eval()
  {
    auto val_list = new expr_value_list;
    for (auto node : v_ast_args)
      val_list->v_val.push_back(node->eval());
    return val_list;
  }
  
  std::vector<ast_node*> v_ast_args;
};

class ast_node_funcdec : public ast_node {
public:
  ast_node_funcdec(ast_node *parlist, ast_node *code_block,
                   std::string name, std::string nspace)
    : parlist(parlist), code_block(code_block),
      name(name), nspace(nspace)
  {
   
  }
  /* fobj äger ast_noderna kanske är dumt? */
  ~ast_node_funcdec() { delete code_block; delete parlist; }

  expr_value *eval()
  {
    auto aa = dynamic_cast<ast_node_parlist*>(parlist);
    if (!aa)
      throw std::runtime_error("qweqweqweqweqweesasdfdsasdffd");
    extern scope_stack scopes;
    auto fobj = new object_func{code_block, name, nspace, parlist};
    scopes.get_top_scope().push_object(fobj);

    return new expr_value_double{0.};
  }
  
  ast_node *code_block;
  ast_node *parlist;
  std::string name;
  std::string nspace;
};

class ast_node_funccall : public ast_node {
public:
  ast_node_funccall(std::string nspace, std::string name, ast_node* arg_list) :
    nspace(nspace), arg_list(arg_list), name(name) {}
  std::string name;
  std::string nspace;
  ast_node *arg_list;

  expr_value *eval()
  {
     extern scope_stack scopes;

     auto obj = scopes.find_object(name, nspace);
     if (!obj)
       throw std::runtime_error("Could not find function " + nspace + " " + name);
     if (obj->type != object_type::FUNC)
       throw std::runtime_error("Symbol " + nspace + " " + name + " not a function.");

     auto fobj = dynamic_cast<object_func*>(obj);

     /* The expr_value_list from eval() owns the 
      * pointers to the corresponding expr_value:s. */
     auto val = dynamic_cast<expr_value_list*>(arg_list->eval());
     if (!val)
       throw std::runtime_error("feval kan inte göras med null obj");
     return fobj->feval(val);
  }
};

class ast_node_chainable : public ast_node {
public:
  ast_node *first;
  ast_node *sec;
};


class ast_node_andchain : public ast_node {
public:
  ast_node_chainable *first;
  ast_node *tail;
  ast_node_andchain *next = nullptr;

  ast_node_andchain(ast_node_chainable *first)
    : first(first), tail(first->sec) {}
  ~ast_node_andchain()
  {
    if (first) delete first;
    if (next) {
      next->first->first = 0;
      delete next;
    }
  }

  void append_next(ast_node_andchain *node)
  {
    auto p = &next;
    while(*p)
      p = &((*p)->next);
    *p = node;
    tail = node->first->sec;
    if (!tail)
      throw std::runtime_error("Bugg");
  }
  
  expr_value *eval()
  {
    bool ans = and_eval(true);
    return new expr_value_double{(double)ans};
  }

  bool and_eval(bool result)
  {
    auto e = first->eval();
    if (e->type != value_type::DOUBLE)
      throw std::runtime_error("andchain Type not implemented");
    auto ed = dynamic_cast<expr_value_double*>(e);

    bool b = !(ed->d == 0) && result;

    delete e;
    
    if (!next)
      return b;
    return next->and_eval(b);
  }
};


class ast_node_les : public ast_node_chainable {
public:
  ast_node_les() : ast_node_les(nullptr, nullptr) {}
  ast_node_les(ast_node *first, ast_node *sec)
  { this->first = first; this->sec = sec; type = ast_type::LES;}
  
  ~ast_node_les()
  {
    if (first) delete first;
    if (sec) delete sec;
  }

  expr_value *eval()
  {
    auto fv = first->eval();
    auto sv = sec->eval();

    if (fv->type == value_type::DOUBLE
        && sv->type == value_type::DOUBLE) {
          auto d_fv = dynamic_cast<expr_value_double*>(fv);
          auto d_sv = dynamic_cast<expr_value_double*>(sv);    
          double ans;
          if (d_fv->d < d_sv->d)
            ans = 1;
          else
            ans = 0;

          delete fv;
          delete sv;
          
          return new expr_value_double{ans};
    } else {
      delete fv;
      delete sv;
      throw std::runtime_error("AST node les:  types not implemented");
    }

  }
};

class ast_node_gre : public ast_node_chainable {
public:
  ast_node_gre() : ast_node_gre(nullptr, nullptr) {}
  ast_node_gre(ast_node *first, ast_node *sec)
    { this->first = first; this->sec = sec; type = ast_type::GRE;}
  
  ~ast_node_gre()
  {
    if (first) delete first;
    if (sec) delete sec;
  }

  expr_value *eval()
  {
    auto fv = first->eval();
    auto sv = sec->eval();

    if (fv->type == value_type::DOUBLE
        && sv->type == value_type::DOUBLE) {
          auto d_fv = dynamic_cast<expr_value_double*>(fv);
          auto d_sv = dynamic_cast<expr_value_double*>(sv);    
          double ans;
          if (d_fv->d > d_sv->d)
            ans = 1;
          else
            ans = 0;

          delete fv;
          delete sv;
          
          return new expr_value_double{ans};
    } else {
      delete fv;
      delete sv;
      throw std::runtime_error("AST node gre:  types not implemented");
    }

  }
};


class ast_node_equ : public ast_node_chainable {
public:
  ast_node_equ() : ast_node_equ(nullptr, nullptr) {}
  ast_node_equ(ast_node *first, ast_node *sec)
    { this->first = first; this->sec = sec; type = ast_type::EQU;}
  
  ~ast_node_equ()
  {
    if (first) delete first;
    if (sec) delete sec;
  }

  expr_value *eval()
  {
    auto fv = first->eval();
    auto sv = sec->eval();

    if (fv->type == value_type::DOUBLE
        && sv->type == value_type::DOUBLE) {
          auto d_fv = dynamic_cast<expr_value_double*>(fv);
          auto d_sv = dynamic_cast<expr_value_double*>(sv);    
          double ans;
          if (d_fv->d == d_sv->d)
            ans = 1;
          else
            ans = 0;

          delete fv;
          delete sv;
          
          return new expr_value_double{ans};
    } else {
      delete fv;
      delete sv;
      throw std::runtime_error("AST node equ:  types not implemented");
    }

  }
};

class ast_node_leq : public ast_node_chainable {
public:
  ast_node_leq() : ast_node_leq(nullptr, nullptr) {}
  ast_node_leq(ast_node *first, ast_node *sec)
    { this->first = first; this->sec = sec; type = ast_type::LEQ;}
  
  ~ast_node_leq()
  {
    if (first) delete first;
    if (sec) delete sec;
  }

  expr_value *eval()
  {
    auto fv = first->eval();
    auto sv = sec->eval();

    if (fv->type == value_type::DOUBLE
        && sv->type == value_type::DOUBLE) {
          auto d_fv = dynamic_cast<expr_value_double*>(fv);
          auto d_sv = dynamic_cast<expr_value_double*>(sv);    
          double ans;
          if (d_fv->d <= d_sv->d)
            ans = 1;
          else
            ans = 0;

          delete fv;
          delete sv;
          
          return new expr_value_double{ans};
    } else {
      delete fv;
      delete sv;
      throw std::runtime_error("AST node leq:  types not implemented");
    }

  }
};


class ast_node_geq : public ast_node_chainable {
public:
  ast_node_geq() : ast_node_geq(nullptr, nullptr) {}
  ast_node_geq(ast_node *first, ast_node *sec)
   { this->first = first; this->sec = sec; type = ast_type::GEQ;}
  
  ~ast_node_geq()
  {
    if (first) delete first;
    if (sec) delete sec;
  }

  expr_value *eval()
  {
    auto fv = first->eval();
    auto sv = sec->eval();

    if (fv->type == value_type::DOUBLE
        && sv->type == value_type::DOUBLE) {
          auto d_fv = dynamic_cast<expr_value_double*>(fv);
          auto d_sv = dynamic_cast<expr_value_double*>(sv);    
          double ans;
          if (d_fv->d >= d_sv->d)
            ans = 1;
          else
            ans = 0;

          delete fv;
          delete sv;
          
          return new expr_value_double{ans};
    } else {
      delete fv;
      delete sv;
      throw std::runtime_error("AST node geq:  types not implemented");
    }

  }
};

class ast_node_neq : public ast_node_chainable {
public:
  ast_node_neq() : ast_node_neq(nullptr, nullptr) {}
  ast_node_neq(ast_node *first, ast_node *sec)
   { this->first = first; this->sec = sec; type = ast_type::NEQ;}
  
  ~ast_node_neq()
  {
    if (first) delete first;
    if (sec) delete sec;
  }

  expr_value *eval()
  {
    auto fv = first->eval();
    auto sv = sec->eval();

    if (fv->type == value_type::DOUBLE
        && sv->type == value_type::DOUBLE) {
          auto d_fv = dynamic_cast<expr_value_double*>(fv);
          auto d_sv = dynamic_cast<expr_value_double*>(sv);    
          double ans;
          if (d_fv->d != d_sv->d)
            ans = 1;
          else
            ans = 0;

          delete fv;
          delete sv;
          
          return new expr_value_double{ans};
    } else {
      delete fv;
      delete sv;
      throw std::runtime_error("AST node neq:  types not implemented");
    }

  }
};
