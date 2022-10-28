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

  bool success = true;

  auto it1 = formallists.begin();
  auto it2 = args.begin();
  absyn::Exp *exp;

  for (; it1 != formallists.end() && it2 != args.end(); ++it1, ++it2) {
    exp = *it2;
    type::Ty *ty = exp->SemAnalyze(venv, tenv, labelcount, errormsg);

    if ((*it1)->ActualTy() != ty->ActualTy()) {
      errormsg->Error(exp->pos_, "para type mismatch");
      success = false;
    }
  }

  if (it2 != args.end()) {
      errormsg->Error(exp->pos_, "too many params in function %s", func_->Name().data());
  } else if (it1 != formallists.end()) {
      errormsg->Error(pos_, "too few params in function %s", func_->Name().data());
  }
  return ((env::FunEntry*) entry)->result_->ActualTy();
}

type::Ty *OpExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                            int labelcount, err::ErrorMsg *errormsg) const {
  /* Put your lab4 code here */
  type::Ty *left_ty = left_->SemAnalyze(venv, tenv, labelcount, errormsg);
  type::Ty *right_ty = right_->SemAnalyze(venv, tenv, labelcount, errormsg);

  switch (oper_) {
    case Oper::PLUS_OP:
    case Oper::MINUS_OP:
    case Oper::TIMES_OP:
    case Oper::DIVIDE_OP: {
      if (typeid(*left_ty) != typeid(type::IntTy)) errormsg->Error(left_->pos_, "integer required");
      if (typeid(*right_ty) != typeid(type::IntTy)) errormsg->Error(right_->pos_, "integer required");
      break;
    }
    default: {
      if (!left_ty->IsSameType(right_ty)) {
        errormsg->Error(pos_, "same type required");
      };
      break;
    }
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
  // FIXME: no error report
  Exp *exp = nullptr;
  type::Field *field = nullptr;

  for (auto it1 = args.begin(); it1 != args.end(); ++it1) {
    exp = (*it1)->exp_;
    field = nullptr;

    for (auto it2 = recordlist.begin(); it2 != recordlist.end(); ++it2) {
      if ((*it2)->name_ == (*it1)->name_) {
        field = *it2;
        break;
      }
    }
    if (!field) {break;}

    type::Ty *ty = exp->SemAnalyze(venv, tenv, labelcount, errormsg);
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
  if (!elsee_) {
    if (typeid(*then_ty) != typeid(type::VoidTy)) {
        errormsg->Error(then_->pos_, "if-then exp's body must produce no value");
    }
  } else {
    type::Ty *else_ty = elsee_->SemAnalyze(venv, tenv, labelcount, errormsg);
    if (!then_ty->IsSameType(else_ty)) {
        errormsg->Error(then_->pos_, "then exp and else exp type mismatch");
    }
    return then_ty->ActualTy();
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
    type::Ty *body_ty = nullptr;
    body_ty = body_->SemAnalyze(venv, tenv, labelcount + 1, errormsg);
    if (typeid(*body_ty) != typeid(type::VoidTy)) {
      errormsg->Error(body_->pos_, "while body must produce no value");
    }
  }

  return DEFAULT_VALUE;
}

type::Ty *ForExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* Put your lab4 code here */
  type::Ty *l_ty = lo_->SemAnalyze(venv, tenv, labelcount, errormsg);
  type::Ty *h_ty = hi_->SemAnalyze(venv, tenv, labelcount, errormsg);

  type::Ty *body_ty = nullptr;

  if (typeid(*l_ty) != typeid(type::IntTy)) {
    errormsg->Error(lo_->pos_, "for exp's range type is not integer");
  }
  if (typeid(*h_ty) != typeid(type::IntTy)) {
    errormsg->Error(hi_->pos_, "for exp's range type is not integer");
  }

  if (body_) {
    venv->BeginScope();
    venv->Enter(var_, new env::VarEntry(type::IntTy::Instance(), true));
    body_ty = body_->SemAnalyze(venv, tenv, labelcount + 1, errormsg);
    venv->EndScope();

    if (typeid(*body_ty) != typeid(type::VoidTy)) {
      errormsg->Error(body_->pos_, "for exp's body must produce no value");
    }
  }

  return type::VoidTy::Instance();
}

type::Ty *BreakExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  /* Put your lab4 code here */
  if (labelcount == 0) {
    errormsg->Error(pos_, "break is not inside any loop");
  }
  return type::VoidTy::Instance();
}

type::Ty *LetExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* Put your lab4 code here */
  venv->BeginScope();
  tenv->BeginScope();

  const auto &decslist = decs_->GetList();
  for (auto iter = decslist.begin(); iter != decslist.end(); iter++) 
    (*iter)->SemAnalyze(venv, tenv, labelcount, errormsg);

  if (!body_) {
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
  if (!ty) {
    errormsg->Error(pos_, "undefined type %s", typ_->Name().c_str());
    return DEFAULT_VALUE;
  };
  ty = ty->ActualTy();
  if (typeid(*ty) != typeid(type::ArrayTy)) {
    errormsg->Error(pos_, "not array type");
    return DEFAULT_VALUE;
  };
  type::Ty * size_ty = size_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if (typeid(size_ty) != typeid(type::IntTy)) {
    errormsg->Error(pos_, "undefined type %s", typ_->Name().c_str());
  };
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
      type::Ty *resultTy = tenv->Look(fun->result_);
      venv->Enter(fun->name_, new env::FunEntry(formalTyList, resultTy));
    } else {
      venv->Enter(fun->name_, new env::FunEntry(formalTyList, type::VoidTy::Instance()));
    }
  }

  for (auto it = list.begin(); it != list.end(); ++it) {
    FunDec *fun = *it;
    type::TyList *formalTyList = fun->params_->MakeFormalTyList(tenv, errormsg);
    venv->BeginScope();

    const auto &fieldList = fun->params_->GetList();
    const auto &tyList = formalTyList->GetList();

    auto it1 = tyList.begin();
    auto it2 = fieldList.begin();
    for (; it2 != fieldList.end(); ++it1, ++it2) {
      venv->Enter((*it2)->name_, new env::VarEntry(*it1));
    }

    type::Ty *ty = fun->body_->SemAnalyze(venv, tenv, labelcount, errormsg);
    env::EnvEntry *entry = venv->Look(fun->name_);
    if (typeid(*entry) != typeid(env::FunEntry) || ty != static_cast<env::FunEntry *>(entry)->result_->ActualTy()) {
      errormsg->Error(pos_, "procedure returns value");
    }
    venv->EndScope();
  }
}

void VarDec::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv, int labelcount,
                        err::ErrorMsg *errormsg) const {
  /* Put your lab4 code here */
  if (venv->Look(this->var_)) {
    return;
  }

  auto init_ty = this->init_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if (typ_) {
    auto ty = tenv->Look(typ_);
    if (!ty) {
      errormsg->Error(pos_, "undefined type of %s", this->typ_->Name().data());
    } else {
      if (!ty->IsSameType(init_ty)) {
        errormsg->Error(pos_, "type mismatch");
      }
      venv->Enter(this->var_, new env::VarEntry(ty));
    }
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
    absyn::NameAndTy *nt = *it;
    type::Ty *ty = tenv->Look(nt->name_);
    type::NameTy *name_ty = static_cast<type::NameTy *>(ty);

    name_ty->ty_ = nt->ty_->SemAnalyze(tenv, errormsg);
  }

  bool loop = false;
  for (auto it = list.begin(); it != list.end(); it++) {
    absyn::NameAndTy *nt = *it;
    type::Ty *ty = tenv->Look(nt->name_);
    auto t = static_cast<type::NameTy *>(ty)->ty_;
    while (typeid(*t) == typeid(type::NameTy)) {
      auto namety = static_cast<type::NameTy *>(t);
      if (namety->sym_ == nt->name_) {
          loop = true;
          break;
      }
      t = namety->ty_;
    }
    if (loop) {
      break;
    }
  }

  if (loop) {
    errormsg->Error(pos_, "illegal type cycle");
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
} // namespace sem
