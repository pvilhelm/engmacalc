#pragma once

#include <stdexcept>
#include <string>
#include <vector>

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
};

enum class value_type {
                            DOUBLE = 1,
                            RVAL,
                            LVAL,
};

enum class object_type {
                        DOUBLE = 1,
};

class scope;

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
    for (auto p : vec_objs)
      delete p;
  }
  void push_object(obj *obj)
  {
    if (find_object(obj->name, obj->nspace))
        throw std::runtime_error("push_object: Pushing existing object:"
                                 + obj->nspace + obj->name);
    vec_objs.push_back(obj);
  }
  void pop_object() { vec_objs.pop_back(); }
  obj*find_object(std::string name, std::string nspace)
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
  value_type type;
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

class expr_value_lval : public expr_value {
public:
  /* Does not own object. */
  obj *object;

  expr_value_lval(obj *obj)
    : object(obj) { type = value_type::LVAL; }
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
          auto d_fv = static_cast<expr_value_double*>(fv);
          auto d_sv = static_cast<expr_value_double*>(sv);    

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
          auto d_fv = static_cast<expr_value_double*>(fv);
          auto d_sv = static_cast<expr_value_double*>(sv);    

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
          auto d_fv = static_cast<expr_value_double*>(fv);
          auto d_sv = static_cast<expr_value_double*>(sv);    

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
          auto d_fv = static_cast<expr_value_double*>(fv);
          auto d_sv = static_cast<expr_value_double*>(sv);    

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
          auto d_fv = static_cast<expr_value_double*>(fv);
  
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
    auto lvar = static_cast<ast_node_var*>(first);
    expr_value *lval_ = lvar->leval();
    if (lval_ == nullptr || lval_->type != value_type::LVAL)
      throw std::runtime_error("ast_node_assign bugg");
    auto lval = static_cast<expr_value_lval*>(lval_);

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

    auto rvald = static_cast<expr_value_double*>(rval);
    auto objd = static_cast<object_double*>(lval->object);
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
          auto d_fv = static_cast<expr_value_double*>(fv);
          auto d_sv = static_cast<expr_value_double*>(sv);    
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

class ast_node_chaincmp : public ast_node {
public:
  ast_node *first;
  ast_node *sec;
};

class ast_node_les : public ast_node_chaincmp {
public:
  ast_node_les() : ast_node_les(nullptr, nullptr) {}
  ast_node_les(ast_node *first, ast_node *sec) :
    first(first) , sec(sec) { type = ast_type::LES;}
  
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
          auto d_fv = static_cast<expr_value_double*>(fv);
          auto d_sv = static_cast<expr_value_double*>(sv);    
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
  
  ast_node *first;
  ast_node *sec;
};

class ast_node_gre : public ast_node_chaincmp {
public:
  ast_node_gre() : ast_node_gre(nullptr, nullptr) {}
  ast_node_gre(ast_node *first, ast_node *sec) :
    first(first) , sec(sec) { type = ast_type::GRE;}
  
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
          auto d_fv = static_cast<expr_value_double*>(fv);
          auto d_sv = static_cast<expr_value_double*>(sv);    
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
  
  ast_node *first;
  ast_node *sec;
};


class ast_node_equ : public ast_node_chaincmp {
public:
  ast_node_equ() : ast_node_equ(nullptr, nullptr) {}
  ast_node_equ(ast_node *first, ast_node *sec) :
    first(first) , sec(sec) { type = ast_type::EQU;}
  
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
          auto d_fv = static_cast<expr_value_double*>(fv);
          auto d_sv = static_cast<expr_value_double*>(sv);    
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
  
  ast_node *first;
  ast_node *sec;
};

class ast_node_leq : public ast_node_chaincmp {
public:
  ast_node_leq() : ast_node_leq(nullptr, nullptr) {}
  ast_node_leq(ast_node *first, ast_node *sec) :
    first(first) , sec(sec) { type = ast_type::LEQ;}
  
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
          auto d_fv = static_cast<expr_value_double*>(fv);
          auto d_sv = static_cast<expr_value_double*>(sv);    
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
  
  ast_node *first;
  ast_node *sec;
};


class ast_node_geq : public ast_node_chaincmp {
public:
  ast_node_geq() : ast_node_geq(nullptr, nullptr) {}
  ast_node_geq(ast_node *first, ast_node *sec) :
    first(first) , sec(sec) { type = ast_type::GEQ;}
  
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
          auto d_fv = static_cast<expr_value_double*>(fv);
          auto d_sv = static_cast<expr_value_double*>(sv);    
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
  
  ast_node *first;
  ast_node *sec;
};

class ast_node_neq : public ast_node_chaincmp {
public:
  ast_node_neq() : ast_node_neq(nullptr, nullptr) {}
  ast_node_neq(ast_node *first, ast_node *sec) :
    first(first) , sec(sec) { type = ast_type::NEQ;}
  
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
          auto d_fv = static_cast<expr_value_double*>(fv);
          auto d_sv = static_cast<expr_value_double*>(sv);    
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
  
  ast_node *first;
  ast_node *sec;
};
