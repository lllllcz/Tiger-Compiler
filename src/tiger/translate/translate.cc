#include "tiger/translate/translate.h"

#include <tiger/absyn/absyn.h>

#include "tiger/env/env.h"
#include "tiger/errormsg/errormsg.h"
#include "tiger/frame/x64frame.h"
#include "tiger/frame/temp.h"
#include "tiger/frame/frame.h"

extern frame::Frags *frags;
extern frame::RegManager *reg_manager;

namespace tr {

Access *Access::AllocLocal(Level *level, bool escape) {
  /* Put your lab5 code here */
  return new Access(level, level->frame_->allocLocal(escape)); 
}

class Cx {
public:
  PatchList trues_;
  PatchList falses_;
  tree::Stm *stm_;

  Cx(PatchList trues, PatchList falses, tree::Stm *stm)
      : trues_(trues), falses_(falses), stm_(stm) {}
};

class Exp {
public:
  [[nodiscard]] virtual tree::Exp *UnEx() = 0;
  [[nodiscard]] virtual tree::Stm *UnNx() = 0;
  [[nodiscard]] virtual Cx UnCx(err::ErrorMsg *errormsg) = 0;
};

class ExpAndTy {
public:
  tr::Exp *exp_;
  type::Ty *ty_;

  ExpAndTy(tr::Exp *exp, type::Ty *ty) : exp_(exp), ty_(ty) {}
};

class ExExp : public Exp {
public:
  tree::Exp *exp_;

  explicit ExExp(tree::Exp *exp) : exp_(exp) {}

  [[nodiscard]] tree::Exp *UnEx() override { 
    /* Put your lab5 code here */
    return exp_;
  }
  [[nodiscard]] tree::Stm *UnNx() override {
    /* Put your lab5 code here */
    return new tree::ExpStm(exp_);
  }
  [[nodiscard]] Cx UnCx(err::ErrorMsg *errormsg) override {
    /* Put your lab5 code here */
    tree::CjumpStm* stm = new tree::CjumpStm(tree::RelOp::NE_OP, exp_, new tree::ConstExp(0), NULL, NULL);
    PatchList true_list(&(stm->true_label_));
    PatchList false_list(&(stm->false_label_));
    return Cx(true_list, false_list, stm);
  }
};

class NxExp : public Exp {
public:
  tree::Stm *stm_;

  explicit NxExp(tree::Stm *stm) : stm_(stm) {}

  [[nodiscard]] tree::Exp *UnEx() override {
    /* Put your lab5 code here */
    return new tree::EseqExp(stm_, new tree::ConstExp(0));
  }
  [[nodiscard]] tree::Stm *UnNx() override { 
    /* Put your lab5 code here */
    return stm_;
  }
  [[nodiscard]] Cx UnCx(err::ErrorMsg *errormsg) override {
    /* Put your lab5 code here */
    // stm can't be converted to cx
    assert(false);
  }
};

class CxExp : public Exp {
public:
  Cx cx_;

  CxExp(PatchList trues, PatchList falses, tree::Stm *stm)
      : cx_(trues, falses, stm) {}
  
  [[nodiscard]] tree::Exp *UnEx() override {
    /* Put your lab5 code here */
    temp::Temp *reg = temp::TempFactory::NewTemp();
    temp::Label* t = temp::LabelFactory::NewLabel(), *f = temp::LabelFactory::NewLabel();;
    cx_.trues_.DoPatch(t);
    cx_.falses_.DoPatch(f);
    return 
      new tree::EseqExp(new tree::MoveStm(new tree::TempExp(reg), new tree::ConstExp(1)),
        new tree::EseqExp(cx_.stm_, 
          new tree::EseqExp(new tree::LabelStm(f),
            new tree::EseqExp(new tree::MoveStm(new tree::TempExp(reg), new tree::ConstExp(0)),
              new tree::EseqExp(new tree::LabelStm(t), 
                new tree::TempExp(reg)
              )
            )
          )
        )
      );
  }
  [[nodiscard]] tree::Stm *UnNx() override {
    /* Put your lab5 code here */
    temp::Label* label = temp::LabelFactory::NewLabel();
    cx_.trues_.DoPatch(label);
    cx_.falses_.DoPatch(label);
    return new tree::SeqStm(cx_.stm_, new tree::LabelStm(label));
  }
  [[nodiscard]] Cx UnCx(err::ErrorMsg *errormsg) override { 
    /* Put your lab5 code here */
    return cx_;
  }
};

void ProgTr::Translate() {
  /* Put your lab5 code here */
  temp::Label *main_label = temp::LabelFactory::NamedLabel("__tigermain__");
  frame::Frame *main_frame = frame::FrameFactory::NewFrame(main_label, {});
  absyn_tree_->Translate(venv_.get(), tenv_.get(), new tr::Level(main_frame, nullptr), main_label, errormsg_.get());
}

tree::Exp* StaticLink(tr::Level* target, tr::Level* current) {
  tree::Exp* staticklink = new tree::TempExp(reg_manager->FramePointer());
  while (current != target) {
    frame::Access *accsee = current->frame_->formals_.front();
    staticklink = accsee->toExp(staticklink);
    current = current->parent_;
  }
  return staticklink;
}

} // namespace tr

namespace absyn {

tr::ExpAndTy *AbsynTree::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* Put your lab5 code here */
  return root_->Translate(venv, tenv, level, label, errormsg);
}

tr::ExpAndTy *SimpleVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* Put your lab5 code here */

  // search for the variable in the environment
  env::EnvEntry *entry = venv->Look(sym_);
  env::VarEntry *var_entry = static_cast<env::VarEntry *>(entry);

  tree::Exp *fp = StaticLink(var_entry->access_->level_, level);
  auto exp = new tr::ExExp(var_entry->access_->access_->toExp(fp));

  return new tr::ExpAndTy(exp, var_entry->ty_->ActualTy());

}

tr::ExpAndTy *FieldVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,
                                  err::ErrorMsg *errormsg) const {
  /* Put your lab5 code here */
  tr::ExpAndTy* var_tr = var_->Translate(venv, tenv, level, label, errormsg);
  type::Ty* var_ty = var_tr->ty_->ActualTy();
  
  type::FieldList * field_list = ((type::RecordTy *) var_ty)->fields_;
  int offset = 0;
  for (auto field : field_list->GetList()) {
    if (field->name_ == sym_) {
      tr::Exp* exp = new tr::ExExp(
        new tree::MemExp(new tree::BinopExp(
          tree::BinOp::PLUS_OP, 
          var_tr->exp_->UnEx(), 
          new tree::ConstExp(offset * reg_manager->WordSize())
        ))
      );
      return new tr::ExpAndTy(exp, field->ty_->ActualTy());
    }
    offset += 1;
  }

  // should not reach here
  return new tr::ExpAndTy(NULL, type::IntTy::Instance());
}

tr::ExpAndTy *SubscriptVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                      tr::Level *level, temp::Label *label,
                                      err::ErrorMsg *errormsg) const {
  /* Put your lab5 code here */
  tr::ExpAndTy* var_tr = var_->Translate(venv, tenv, level, label, errormsg);

  tr::ExpAndTy* subscript_tr = subscript_->Translate(venv, tenv, level, label, errormsg);
  
  tr::Exp* exp = new tr::ExExp(new tree::MemExp(new tree::BinopExp(
    tree::BinOp::PLUS_OP, 
    var_tr->exp_->UnEx(), 
    new tree::BinopExp(tree::BinOp::MUL_OP, subscript_tr->exp_->UnEx(), new tree::ConstExp(reg_manager->WordSize()))
  )));
  type::Ty* ty = ((type::ArrayTy *) var_tr->ty_)->ty_->ActualTy();
  return new tr::ExpAndTy(exp, ty);
}

tr::ExpAndTy *VarExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* Put your lab5 code here */
  return var_->Translate(venv, tenv, level, label, errormsg);
}

tr::ExpAndTy *NilExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* Put your lab5 code here */
  return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)), type::NilTy::Instance());
}

tr::ExpAndTy *IntExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* Put your lab5 code here */
  return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(1)), type::IntTy::Instance());
}

tr::ExpAndTy *StringExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* Put your lab5 code here */
  temp::Label* string_label = temp::LabelFactory::NewLabel();
  frags->PushBack(new frame::StringFrag(string_label, str_));
  return new tr::ExpAndTy(new tr::ExExp(new tree::NameExp(string_label)), type::StringTy::Instance());
}

tr::ExpAndTy *CallExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                 tr::Level *level, temp::Label *label,
                                 err::ErrorMsg *errormsg) const {
  /* Put your lab5 code here */
  env::EnvEntry *entry = venv->Look(func_);
  if (!entry || typeid(entry) != typeid(env::FunEntry)) {
    errormsg->Error(pos_, "undefined function %s", func_->Name().c_str());
    return new tr::ExpAndTy(nullptr, type::IntTy::Instance());
  }

  env::FunEntry *fun_entry = (env::FunEntry*) entry;
  tree::ExpList *argexp_list = new tree::ExpList();

  for (auto arg : args_->GetList()) {
    argexp_list->Append(arg->Translate(venv, tenv , level, label, errormsg)->exp_->UnEx());
  }

  tr::Exp *exp = nullptr;
  if (!fun_entry->level_->parent_)
    exp = new tr::ExExp(frame::FrameFactory::ExternalCall(func_->Name(), argexp_list));
  else {
    argexp_list->Append(tr::StaticLink(fun_entry->level_->parent_, level));

    exp = new tr::ExExp(new tree::CallExp(new tree::NameExp(func_), argexp_list));
  };
  return new tr::ExpAndTy(exp, fun_entry->result_);

}

tr::ExpAndTy *OpExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                               tr::Level *level, temp::Label *label,
                               err::ErrorMsg *errormsg) const {
  /* Put your lab5 code here */
  tr::ExpAndTy* right_tr = right_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy* left_tr = left_->Translate(venv, tenv, level, label, errormsg);

  tr::Exp *exp = nullptr;
  tree::CjumpStm *stm = nullptr;

  switch (oper_) {

  case absyn::PLUS_OP:
    exp = new tr::ExExp(new tree::BinopExp(tree::BinOp::PLUS_OP, left_tr->exp_->UnEx(), right_tr->exp_->UnEx()));
    break;
  case absyn::MINUS_OP:
    exp = new tr::ExExp(new tree::BinopExp(tree::BinOp::MINUS_OP, left_tr->exp_->UnEx(), right_tr->exp_->UnEx()));
    break;
  case absyn::TIMES_OP:
    exp = new tr::ExExp(new tree::BinopExp(tree::BinOp::MUL_OP, left_tr->exp_->UnEx(), right_tr->exp_->UnEx()));
    break;
  case absyn::DIVIDE_OP:
    exp = new tr::ExExp(new tree::BinopExp(tree::BinOp::DIV_OP, left_tr->exp_->UnEx(), right_tr->exp_->UnEx()));
    break;

  case absyn::EQ_OP:
    // FIXME: compare string
    if (left_tr->ty_->IsSameType(type::StringTy::Instance()))
      stm = new tree::CjumpStm(
        tree::EQ_OP, 
        frame::FrameFactory::ExternalCall("string_equal", new tree::ExpList({left_tr->exp_->UnEx(), right_tr->exp_->UnEx()})), 
        new tree::ConstExp(1), 
        NULL, 
        NULL
      );
    else
      stm = new tree::CjumpStm(tree::RelOp::EQ_OP, left_tr->exp_->UnEx(), right_tr->exp_->UnEx(), nullptr, nullptr);
    break;
  case absyn::NEQ_OP:
    stm = new tree::CjumpStm(tree::RelOp::NE_OP, left_tr->exp_->UnEx(), right_tr->exp_->UnEx(), nullptr, nullptr);
    break;
  case absyn::LT_OP:
    stm = new tree::CjumpStm(tree::RelOp::LT_OP, left_tr->exp_->UnEx(), right_tr->exp_->UnEx(), nullptr, nullptr);
    break;
  case absyn::LE_OP:
    stm = new tree::CjumpStm(tree::RelOp::LE_OP, left_tr->exp_->UnEx(), right_tr->exp_->UnEx(), nullptr, nullptr);
    break;
  case absyn::GT_OP:
    stm = new tree::CjumpStm(tree::RelOp::GT_OP, left_tr->exp_->UnEx(), right_tr->exp_->UnEx(), nullptr, nullptr);
    break;
  case absyn::GE_OP:
    stm = new tree::CjumpStm(tree::RelOp::GE_OP, left_tr->exp_->UnEx(), right_tr->exp_->UnEx(), nullptr, nullptr);
    break;
  default:
    break;
  }

  if (exp == nullptr) {
    tr::PatchList true_list(&(stm->true_label_));
    tr::PatchList false_list(&(stm->false_label_));
    exp = new tr::CxExp(true_list, false_list, stm);
  }
  return new tr::ExpAndTy(exp, type::IntTy::Instance());
}

tr::ExpAndTy *RecordExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,      
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  type::Ty* ty = tenv->Look(typ_)->ActualTy();
  tree::ExpList* list = new tree::ExpList();

  auto efields = fields_->GetList();

  int count = 0;
  for (auto efield : efields) {
    count += 1;

    tr::ExpAndTy* tmpexp = efield->exp_->Translate(venv, tenv, level, label, errormsg);
    
    list->Append(tmpexp->exp_->UnEx());

  }

  temp::Temp* reg = temp::TempFactory::NewTemp();
  auto *args = new tree::ExpList();
  args->Append(new tree::ConstExp(reg_manager->WordSize() * count));
  tree::Stm* stm = new tree::MoveStm(
    new tree::TempExp(reg), 
    frame::FrameFactory::ExternalCall("alloc_record", args));

  count = 0;
  
  for (auto ele : list->GetList()) {
    stm = new tree::SeqStm(
      stm, 
      new tree::MoveStm(
        new tree::MemExp(new tree::BinopExp(
          tree::BinOp::PLUS_OP, 
          new tree::TempExp(reg), 
          new tree::ConstExp(count * reg_manager->WordSize()))), 
        NULL
      )
    );
    count += 1;
  }
  tr::ExExp* exp = new tr::ExExp(new tree::EseqExp(stm, new tree::TempExp(reg)));

  return new tr::ExpAndTy(exp, ty);
}

tr::ExpAndTy *SeqExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* Put your lab5 code here */
  if (!seq_) return new tr::ExpAndTy(nullptr, type::VoidTy::Instance());

  tr::Exp *exp = new tr::ExExp(new tree::ConstExp(0));
  tr::ExpAndTy *et = nullptr;
  for (auto seqexp : seq_->GetList()) {
    et = seqexp->Translate(venv, tenv, level, label, errormsg);
    if (et->exp_)
      exp = new tr::ExExp(new tree::EseqExp(exp->UnNx(), et->exp_->UnEx()));
    else
      exp = new tr::ExExp(new tree::EseqExp(exp->UnNx(), new tree::ConstExp(0)));
  }

  return new tr::ExpAndTy(exp, et->ty_);
}

tr::ExpAndTy *AssignExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,                       
                                   err::ErrorMsg *errormsg) const {
  /* Put your lab5 code here */
  
  tr::ExpAndTy * var_tr = var_->Translate(venv, tenv, level, label, errormsg);
  
  tr::ExpAndTy * exp_tr = exp_->Translate(venv, tenv, level, label, errormsg);

  return new tr::ExpAndTy(new tr::NxExp(new tree::MoveStm(var_tr->exp_->UnEx(), exp_tr->exp_->UnEx())), type::VoidTy::Instance());
}

tr::ExpAndTy *IfExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                               tr::Level *level, temp::Label *label,
                               err::ErrorMsg *errormsg) const {
  /* Put your lab5 code here */
  tr::Exp *exp = nullptr;
  tr::ExpAndTy * test_et = test_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy * then_et = then_->Translate(venv, tenv, level, label, errormsg);

  tree::Stm * test_cx_stm = test_et->exp_->UnCx(errormsg).stm_;
  temp::Temp* r = temp::TempFactory::NewTemp();
  temp::Label* true_label = temp::LabelFactory::NewLabel();
  temp::Label* false_label = temp::LabelFactory::NewLabel();

  if (elsee_) {
    tr::ExpAndTy * else_et = elsee_->Translate(venv, tenv, level, label, errormsg);

    temp::Label* done_label = temp::LabelFactory::NewLabel();
    auto *done_label_list = new std::vector<temp::Label *>();
    done_label_list->push_back(done_label);
    
    exp = new tr::ExExp(new tree::EseqExp(test_cx_stm, new tree::EseqExp(
      new tree::LabelStm(true_label),
      new tree::EseqExp(
        new tree::MoveStm(new tree::TempExp(r), then_et->exp_->UnEx()),
        new tree::EseqExp(
          new tree::JumpStm(new tree::NameExp(done_label), done_label_list),
          new tree::EseqExp(
            new tree::LabelStm(false_label),
            new tree::EseqExp(
              new tree::MoveStm(new tree::TempExp(r), else_et->exp_->UnEx()),
              new tree::EseqExp(new tree::LabelStm(done_label), new tree::TempExp(r))
            )
          )
        )
      )
    )));
  }
  else
    exp = new tr::NxExp(new tree::SeqStm(
      test_cx_stm, 
      new tree::SeqStm(
        new tree::LabelStm(true_label), 
        new tree::SeqStm(then_et->exp_->UnNx(), new tree::LabelStm(false_label))
      )
    ));

  return new tr::ExpAndTy(exp, then_et->ty_);
}

tr::ExpAndTy *WhileExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,            
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::Exp *exp = nullptr;
  tr::ExpAndTy * test_et = test_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy * body_et = body_->Translate(venv, tenv, level, label, errormsg);

  temp::Label* test_label = temp::LabelFactory::NewLabel();
  temp::Label* body_label = temp::LabelFactory::NewLabel();
  temp::Label* done_label = temp::LabelFactory::NewLabel();
  tree::Stm * test_cx_stm = test_et->exp_->UnCx(errormsg).stm_;

  auto *test_label_list = new std::vector<temp::Label *>();
  test_label_list->push_back(test_label);

  // FIXME: do patch?

  exp = new tr::NxExp(new tree::SeqStm(
    new tree::LabelStm(test_label),
    new tree::SeqStm(
      test_cx_stm,
      new tree::SeqStm(
        new tree::LabelStm(body_label),
        new tree::SeqStm(
          body_et->exp_->UnNx(),
          new tree::SeqStm(
            new tree::JumpStm(new tree::NameExp(test_label), test_label_list),
            new tree::LabelStm(done_label)
          )
        )
      )
    )
  ));

  return new tr::ExpAndTy(exp, body_et->ty_);
}

tr::ExpAndTy *ForExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::Exp *exp = nullptr;
  tr::ExpAndTy * hi_et = hi_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy * lo_et = lo_->Translate(venv, tenv, level, label, errormsg);

  venv->BeginScope();
  tr::Access * access = tr::Access::AllocLocal(level, escape_);
  venv->Enter(var_, new env::VarEntry(access, lo_et->ty_)); // FIXME: read only?
  tr::ExpAndTy * body_et = body_->Translate(venv, tenv, level, label, errormsg); // FIXME: label?
  venv->EndScope();
  
  temp::Label *body_label = temp::LabelFactory::NewLabel();
  temp::Label *test_label = temp::LabelFactory::NewLabel();

  tree::Exp *i = access->access_->toExp(new tree::TempExp(reg_manager->FramePointer()));
  exp = new tr::NxExp(new tree::SeqStm(
    new tree::MoveStm(i, lo_et->exp_->UnEx()),
    new tree::SeqStm(
      new tree::LabelStm(test_label),
      new tree::SeqStm(
        new tree::CjumpStm(tree::RelOp::LE_OP, i, hi_et->exp_->UnEx(), body_label, label),
        new tree::SeqStm(
          new tree::LabelStm(body_label),
          new tree::SeqStm(
            body_et->exp_->UnNx(),
            new tree::SeqStm(
              new tree::MoveStm(i, new tree::BinopExp(tree::BinOp::PLUS_OP, i, new tree::ConstExp(1))),
              new tree::SeqStm(
                new tree::JumpStm(new tree::NameExp(test_label), new std::vector<temp::Label *>(1, test_label)),
                new tree::LabelStm(label)
              )
            )
          )
        )
      )
    )
  ));

  return new tr::ExpAndTy(exp, type::VoidTy::Instance());
}

tr::ExpAndTy *BreakExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,
                                  err::ErrorMsg *errormsg) const {
  /* Put your lab5 code here */
  tree::Stm* stm = new tree::JumpStm(new tree::NameExp(label), new std::vector<temp::Label *>(1, label));
  tr::Exp* nx = new tr::NxExp(stm);
  return new tr::ExpAndTy(nx, type::VoidTy::Instance());
}

tr::ExpAndTy *LetExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
}

tr::ExpAndTy *ArrayExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,                    
                                  err::ErrorMsg *errormsg) const {
  /* Put your lab5 code here */
  type::Ty* ty = tenv->Look(typ_)->ActualTy();

  tr::ExpAndTy* size_et = size_->Translate(venv, tenv, level, label, errormsg);

  tr::ExpAndTy* check_et = init_->Translate(venv, tenv, level, label, errormsg);
 
  tr::Exp* exp = new tr::ExExp(frame::FrameFactory::ExternalCall("init_array", new tree::ExpList({size_et->exp_->UnEx(), size_et->exp_->UnEx()})));
  return new tr::ExpAndTy(exp, ty);
}

tr::ExpAndTy *VoidExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                 tr::Level *level, temp::Label *label,
                                 err::ErrorMsg *errormsg) const {
  /* Put your lab5 code here */
  return new tr::ExpAndTy(NULL, type::VoidTy::Instance());
}

tr::Exp *FunctionDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
}

tr::Exp *VarDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                           tr::Level *level, temp::Label *label,
                           err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
}

tr::Exp *TypeDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                            tr::Level *level, temp::Label *label,
                            err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
}

type::Ty *NameTy::Translate(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  /* Put your lab5 code here */
  type::Ty* ty = tenv->Look(name_);
  if (!ty) {
    //errormsg->Error(pos_, "undefined type %s", name_->Name().c_str());
    return type::VoidTy::Instance();
  };
  return new type::NameTy(name_, ty);
}

type::Ty *RecordTy::Translate(env::TEnvPtr tenv,
                              err::ErrorMsg *errormsg) const {
  /* Put your lab5 code here */
  return new type::RecordTy(record_->MakeFieldList(tenv, errormsg));
}

type::Ty *ArrayTy::Translate(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  /* Put your lab5 code here */
  type::Ty *ty = tenv->Look(array_);
  if (!ty) {
    errormsg->Error(pos_, "undefined type %s", array_->Name().c_str());
    return type::IntTy::Instance();
  }
  return new type::ArrayTy(ty);
}

} // namespace absyn
