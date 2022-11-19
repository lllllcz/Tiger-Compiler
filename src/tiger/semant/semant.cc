#include "tiger/absyn/absyn.h"
#include "tiger/semant/semant.h"

namespace absyn {

#define DEFAULT_VALUE type::IntTy::Instance()
#define DEFAULT_TYPE type::VoidTy::Instance()

void AbsynTree::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                           err::ErrorMsg *errormsg) const {
  /* Put your lab4 code here */
  this->root_->SemAnalyze(venv, tenv, 0, errormsg);
}

type::Ty *SimpleVar::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  /* Put your lab4 code here */
  env::EnvEntry *entry = venv->Look(sym_);
  if (entry && typeid(*entry) == typeid(env::VarEntry)) {
    return ((env::VarEntry*) entry)->ty_->ActualTy();
  } else {
    errormsg->Error(pos_, "undefined variable %s", sym_->Name().c_str());
    return DEFAULT_VALUE;
  };
}

type::Ty *FieldVar::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  /* Put your lab4 code here */
  type::Ty *ty = var_->SemAnalyze(venv, tenv, labelcount, errormsg);

  if (typeid(*ty) != typeid(type::RecordTy)) {
    errormsg->Error(pos_, "not a record type");
    return DEFAULT_VALUE;
  };

  type::FieldList* fieldlist = ((type::RecordTy*)ty)->fields_;

  const auto &list = fieldlist->GetList();

  type::Ty *t = nullptr;
  for (auto it = list.begin(); it != list.end(); ++it) {
    if (sym_ == (*it)->name_) {
        return (*it)->ty_->ActualTy();
    }
  }
  errormsg->Error(pos_, "field %s doesn't exist", sym_->Name().data());
  return DEFAULT_VALUE;
}

type::Ty *SubscriptVar::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   int labelcount,
                                   err::ErrorMsg *errormsg) const {
  /* Put your lab4 code here */
  type::Ty *var_ty = var_->SemAnalyze(venv, tenv, labelcount, errormsg);
  type::Ty *exp_ty = subscript_->SemAnalyze(venv, tenv, labelcount, errormsg);

  if (typeid(*exp_ty) != typeid(type::IntTy)) {
    errormsg->Error(pos_, "array index must be integer");
    return DEFAULT_VALUE;
  };
  if (typeid(*var_ty) != typeid(type::ArrayTy)) {
    errormsg->Error(pos_, "array type required");
    return DEFAULT_VALUE;
  };

  return ((type::ArrayTy*) var_ty)->ty_->ActualTy();
}

type::Ty *VarExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* Put your lab4 code here */
  // FIXME: need switch?
  return var_->SemAnalyze(venv, tenv, labelcount, errormsg);
}

type::Ty *NilExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* Put your lab4 code here */
  return type::NilTy::Instance();
}

type::Ty *IntExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* Put your lab4 code here */
  return type::IntTy::Instance();
}

type::Ty *StringExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  /* Put your lab4 code here */
  return type::StringTy::Instance();
}

type::Ty *CallExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                              int labelcount, err::ErrorMsg *errormsg) const {
  /* Put your lab4 code here */
  env::EnvEntry *entry = venv->Look(func_);

  if (!entry || typeid(*entry) != typeid(env::FunEntry)) {
    errormsg->Error(pos_, "undefined function %s", func_->Name().c_str());
    return DEFAULT_VALUE;
  };

  type::TyList *formals = ((env::FunEntry*) entry)->formals_;
  const auto &formallists = formals->GetList();
  const auto &args = this->args_->GetList();

  auto f_it = formallists.begin();
  auto a_it = args.begin();

  for (; f_it != formallists.end() && a_it != args.end(); ++f_it, ++a_it) {
    type::Ty *ty = (*a_it)->SemAnalyze(venv, tenv, labelcount, errormsg);

    if ((*f_it)->ActualTy() != ty->ActualTy()) {
      errormsg->Error((*a_it)->pos_, "para type mismatch");
    }
  }

  if (a_it != args.end()) {
      errormsg->Error((*a_it)->pos_, "too many params in function %s", func_->Name().data());
  } else if (f_it != formallists.end()) {
      errormsg->Error(pos_, "too few params in function %s", func_->Name().data());
  }
  return ((env::FunEntry*) entry)->result_->ActualTy();
}

type::Ty *OpExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                            int labelcount, err::ErrorMsg *errormsg) const {
  /* Put your lab4 code here */
  type::Ty *left_ty = left_->SemAnalyze(venv, tenv, labelcount, errormsg);
  type::Ty *right_ty = right_->SemAnalyze(venv, tenv, labelcount, errormsg);

  if (!left_ty->IsSameType(right_ty)) {
    errormsg->Error(pos_, "same type required");
  };
  
  if (oper_ == Oper::PLUS_OP || oper_ == Oper::MINUS_OP ||
      oper_ == Oper::TIMES_OP || oper_ == Oper::DIVIDE_OP)
  {
    if (typeid(*left_ty) != typeid(type::IntTy)) errormsg->Error(left_->pos_, "integer required");
    if (typeid(*right_ty) != typeid(type::IntTy)) errormsg->Error(right_->pos_, "integer required");
  }
  return DEFAULT_VALUE;
}

type::Ty *RecordExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  /* Put your lab4 code here */
  type::Ty* ty = tenv->Look(typ_);

  if (!ty) {
    errormsg->Error(pos_, "undefined type %s", typ_->Name().c_str());
    return DEFAULT_VALUE;
  };
  ty = ty->ActualTy();
  if (typeid(*ty) != typeid(type::RecordTy)) {
    errormsg->Error(pos_, "not record type %s", typ_->Name().c_str());
    return DEFAULT_VALUE;
  };

  const auto &args = this->fields_->GetList();
  const auto &recordlist = ((type::RecordTy*) ty)->fields_->GetList();
  // FIXME: no error report here
  type::Field *field = nullptr;

  for (auto a_it = args.begin(); a_it != args.end(); ++a_it) {
    field = nullptr;

    for (auto r_it = recordlist.begin(); r_it != recordlist.end(); ++r_it) {
      if ((*r_it)->name_ == (*a_it)->name_) {
        field = *r_it;
        break;
      }
    }
    if (!field) {break;}

    type::Ty *ty = (*a_it)->exp_->SemAnalyze(venv, tenv, labelcount, errormsg);
  }

  return ty;
}

type::Ty *SeqExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* Put your lab4 code here */
  const auto &exps = seq_->GetList();

  type::Ty *ty = nullptr;
  for (auto it = exps.begin(); it != exps.end(); ++it) {
    ty = (*it)->SemAnalyze(venv, tenv, labelcount, errormsg);
  }

  return ty;
}

type::Ty *AssignExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  /* Put your lab4 code here */
  if (typeid(*var_) == typeid(SimpleVar)) {
    env::EnvEntry* entry = venv->Look(((SimpleVar*) var_)->sym_);
    if (entry->readonly_) {
      errormsg->Error(pos_, "loop variable can't be assigned");
      return DEFAULT_VALUE;
    };
  };

  type::Ty* var_ty = var_->SemAnalyze(venv, tenv, labelcount, errormsg);
  type::Ty* exp_ty = exp_->SemAnalyze(venv, tenv, labelcount, errormsg);

  if (!var_ty->IsSameType(exp_ty))
    errormsg->Error(pos_, "unmatched assign exp");

  return var_ty;
}

type::Ty *IfExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                            int labelcount, err::ErrorMsg *errormsg) const {
  /* Put your lab4 code here */
  type::Ty *test_ty = this->test_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if (typeid(*test_ty) != typeid(type::IntTy)) {
    errormsg->Error(pos_, "integer required");
  }

  type::Ty *then_ty = then_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if (elsee_) {
    // else is not empty
    type::Ty *else_ty = elsee_->SemAnalyze(venv, tenv, labelcount, errormsg);
    if (!then_ty->IsSameType(else_ty)) {
        errormsg->Error(then_->pos_, "then exp and else exp type mismatch");
    }
    return then_ty->ActualTy();
  } else {
    if (typeid(*then_ty) != typeid(type::VoidTy)) {
        errormsg->Error(then_->pos_, "if-then exp's body must produce no value");
    }
  }
  return DEFAULT_TYPE;
}

type::Ty *WhileExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  /* Put your lab4 code here */
  type::Ty *test_ty = test_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if (typeid(*test_ty) != typeid(type::IntTy)) {
      errormsg->Error(test_->pos_, "while exp's test must produce int value");
  }
  
  if (body_) {
    // body is not empty
    type::Ty *body_ty =  body_->SemAnalyze(venv, tenv, labelcount + 1, errormsg);
    if (typeid(*body_ty) != typeid(type::VoidTy)) {
      errormsg->Error(body_->pos_, "while body must produce no value");
    }
  }

  return DEFAULT_VALUE;
}

type::Ty *ForExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* Put your lab4 code here */
  
  type::Ty *hi_ty = hi_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if (typeid(*hi_ty) != typeid(type::IntTy)) {
    errormsg->Error(hi_->pos_, "for exp's range type is not integer");
  }

  if (body_) {
    venv->BeginScope();
    venv->Enter(var_, new env::VarEntry(type::IntTy::Instance(), true));
    type::Ty *body_ty = body_->SemAnalyze(venv, tenv, labelcount + 1, errormsg);
    venv->EndScope();

    if (typeid(*body_ty) != typeid(type::VoidTy)) {
      errormsg->Error(body_->pos_, "for exp's body must produce no value");
    }
  }

  return DEFAULT_TYPE;
}

type::Ty *BreakExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  /* Put your lab4 code here */
  if (labelcount == 0) {
    errormsg->Error(pos_, "break is not inside any loop");
  }
  return DEFAULT_TYPE;
}

type::Ty *LetExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* Put your lab4 code here */
  venv->BeginScope();
  tenv->BeginScope();

  const auto &decslist = decs_->GetList();
  for (auto it = decslist.begin(); it != decslist.end(); it++) 
    (*it)->SemAnalyze(venv, tenv, labelcount, errormsg);

  if (!body_) {
    // body is empty
    return DEFAULT_VALUE;
  }

  type::Ty* ty = body_->SemAnalyze(venv, tenv, labelcount, errormsg);
  
  venv->EndScope();
  tenv->EndScope();
  return ty;
}

type::Ty *ArrayExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  /* Put your lab4 code here */
  type::Ty* ty = tenv->Look(typ_);
  
  // if (!ty) {
  //   errormsg->Error(pos_, "undefined type %s", typ_->Name().c_str());
  //   return DEFAULT_VALUE;
  // };

  ty = ty->ActualTy();
  if (typeid(*ty) != typeid(type::ArrayTy)) {
    errormsg->Error(pos_, "not array type");
    return DEFAULT_VALUE;
  };

  // type::Ty * size_ty = size_->SemAnalyze(venv, tenv, labelcount, errormsg);
  // if (typeid(size_ty) != typeid(type::IntTy)) {
  //   errormsg->Error(pos_, "undefined type %s", typ_->Name().c_str());
  // };

  type::ArrayTy* array_ty = (type::ArrayTy*) ty;
  if (!init_->SemAnalyze(venv, tenv, labelcount, errormsg)->IsSameType(array_ty->ty_)) {
    errormsg->Error(pos_, "type mismatch");
  };

  return array_ty->ActualTy();
}

type::Ty *VoidExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                              int labelcount, err::ErrorMsg *errormsg) const {
  /* Put your lab4 code here */
  return type::VoidTy::Instance();
}

void FunctionDec::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* Put your lab4 code here */
  const auto &list = this->functions_->GetList();

  for (auto it = list.begin(); it != list.end(); ++it) {
    FunDec *fun = *it;
    if (venv->Look(fun->name_)) {
      errormsg->Error(pos_, "two functions have the same name");
    }
    type::TyList *formalTyList = fun->params_->MakeFormalTyList(tenv, errormsg);
    if (fun->result_) {
      venv->Enter(fun->name_, new env::FunEntry(formalTyList, tenv->Look(fun->result_)));
    } else {
      venv->Enter(fun->name_, new env::FunEntry(formalTyList, type::VoidTy::Instance()));
    }
  }

  for (auto it = list.begin(); it != list.end(); ++it) {
    FunDec *fun = *it;
    venv->BeginScope();

    const auto &fieldList = fun->params_->GetList();
    const auto &tyList = fun->params_->MakeFormalTyList(tenv, errormsg)->GetList();

    auto it1 = tyList.begin();
    auto it2 = fieldList.begin();
    for (; it2 != fieldList.end(); ++it1, ++it2) {
      venv->Enter((*it2)->name_, new env::VarEntry(*it1));
    }

    type::Ty *ty = fun->body_->SemAnalyze(venv, tenv, labelcount, errormsg);
    env::EnvEntry *entry = venv->Look(fun->name_);
    if (typeid(*entry) != typeid(env::FunEntry) || ty != ((env::FunEntry *)entry)->result_->ActualTy()) {
      errormsg->Error(pos_, "procedure returns value");
    }
    venv->EndScope();
  }
}

void VarDec::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv, int labelcount,
                        err::ErrorMsg *errormsg) const {
  /* Put your lab4 code here */
  if (venv->Look(this->var_)) {
    errormsg->Error(pos_, "two varible have the same name");
    return;
  }

  type::Ty * init_ty = this->init_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if (typ_) {
    // type is not empty
    type::Ty * ty = tenv->Look(typ_);
    
    if (!ty->IsSameType(init_ty)) {
      errormsg->Error(pos_, "type mismatch");
    }
    venv->Enter(this->var_, new env::VarEntry(ty));
  
  } else {
    if (init_ty->IsSameType(type::NilTy::Instance()) && typeid(*(init_ty->ActualTy())) != typeid(type::RecordTy)) {
      errormsg->Error(pos_, "init should not be nil without type specified");
    } else {
      venv->Enter(this->var_, new env::VarEntry(init_ty));
    }
  }
}

void TypeDec::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv, int labelcount,
                         err::ErrorMsg *errormsg) const {
  /* Put your lab4 code here */
  const auto &list = types_->GetList();

  for (auto it = list.begin(); it != list.end(); ++it) {
    absyn::NameAndTy *nt = *it;
    if (tenv->Look(nt->name_)) {
      errormsg->Error(pos_, "two types have the same name");
    } else {
      tenv->Enter(nt->name_, new type::NameTy(nt->name_, nullptr));
    }
  }

  for (auto it = list.begin(); it != list.end(); ++it) {
    type::Ty *ty = tenv->Look((*it)->name_);
    ((type::NameTy *)ty)->ty_ = (*it)->ty_->SemAnalyze(tenv, errormsg);
  }

  for (auto it = list.begin(); it != list.end(); it++) {
    type::Ty *ty = tenv->Look((*it)->name_);
    auto t = ((type::NameTy *)ty)->ty_;
    while (typeid(*t) == typeid(type::NameTy)) {
      type::NameTy * namety = (type::NameTy *)t;
      if (namety->sym_ == (*it)->name_) {
        errormsg->Error(pos_, "illegal type cycle");
        return;
      }
      t = namety->ty_;
    }
  }
}

type::Ty *NameTy::SemAnalyze(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  /* Put your lab4 code here */
  type::Ty* ty = tenv->Look(name_);
  if (!ty) {
    errormsg->Error(pos_, "undefined type %s", name_->Name().c_str());
    return DEFAULT_TYPE;
  };
  return new type::NameTy(name_, ty);
}

type::Ty *RecordTy::SemAnalyze(env::TEnvPtr tenv,
                               err::ErrorMsg *errormsg) const {
  /* Put your lab4 code here */
  return new type::RecordTy(record_->MakeFieldList(tenv, errormsg));
}

type::Ty *ArrayTy::SemAnalyze(env::TEnvPtr tenv,
                              err::ErrorMsg *errormsg) const {
  /* Put your lab4 code here */
  type::Ty* ty = tenv->Look(array_);
  if (!ty) {
    errormsg->Error(pos_, "undefined type %s", array_->Name().c_str());
    return DEFAULT_TYPE;
  };
  return new type::ArrayTy(ty);
}

} // namespace absyn

namespace sem {

void ProgSem::SemAnalyze() {
  FillBaseVEnv();
  FillBaseTEnv();
  absyn_tree_->SemAnalyze(venv_.get(), tenv_.get(), errormsg_.get());
}

} // namespace tr
