#include "tiger/translate/translate.h"

#include <tiger/absyn/absyn.h>
#include <vector>

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

    tree::CjumpStm* stm = new tree::CjumpStm(tree::RelOp::NE_OP, exp_, new tree::ConstExp(0), nullptr, nullptr);
    PatchList true_list({&(stm->true_label_)});
    PatchList false_list({&(stm->false_label_)});
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
    temp::Label *t = temp::LabelFactory::NewLabel(), *f = temp::LabelFactory::NewLabel();;
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

    temp::Label *label = temp::LabelFactory::NewLabel();
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
  /* TODO: Put your lab5 code here */

  temp::Label* main_label_ = temp::LabelFactory::NamedLabel("tigermain");
  frame::Frame* main_frame_ = frame::NewFrame(main_label_, {});
  main_level_ = std::make_unique<tr::Level>(main_frame_, nullptr);

  FillBaseTEnv();
  FillBaseVEnv();

  absyn_tree_->Translate(venv_.get(), tenv_.get(), main_level_.get(), main_label_, errormsg_.get());
  return;
}

tree::Exp* StaticLink(tr::Level* target, tr::Level* current) {
  
  tree::Exp* staticlink = new tree::TempExp(reg_manager->FramePointer());
  while (current != target) {
    staticlink = new tree::MemExp(new tree::BinopExp(tree::PLUS_OP, staticlink, new tree::ConstExp(-current->frame_->frame_offset)));    
    current = current->parent_;
  }
  staticlink = new tree::MemExp(new tree::BinopExp(tree::PLUS_OP, staticlink, new tree::ConstExp(current->frame_->frame_offset)));

  return staticlink;
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

  env::VarEntry *var_entry = (env::VarEntry *)venv->Look(sym_); 

  tree::Exp *fp = StaticLink(var_entry->access_->level_, level);
  tr::Exp *exp = new tr::ExExp(var_entry->access_->access_->toExp(fp));

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
  
  return new tr::ExpAndTy(nullptr, type::IntTy::Instance());
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
  return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(val_)), type::IntTy::Instance());
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
  if (!entry || typeid(*entry) != typeid(env::FunEntry)) {
    errormsg->Error(pos_, "undefined function %s", func_->Name().c_str());
    return new tr::ExpAndTy(nullptr, type::IntTy::Instance());
  }

  env::FunEntry *fun_entry = (env::FunEntry*) entry;
  tree::ExpList *argexp_list = new tree::ExpList();

  type::Ty *ty = (fun_entry->result_ != nullptr) ? (fun_entry->result_->ActualTy()) : (type::VoidTy::Instance());

  for (auto arg : args_->GetList()) {
    
    argexp_list->Append(arg->Translate(venv, tenv , level, label, errormsg)->exp_->UnEx());
  }
  

  tr::Exp *exp = nullptr;
  if (fun_entry->level_->parent_ == nullptr)
    exp = new tr::ExExp(frame::ExternalCall(temp::LabelFactory::LabelString(func_), argexp_list));
  else {
    // argexp_list->Insert(tr::StaticLink(fun_entry->level_->parent_, level)); // FIXME: do i need this line?
    exp = new tr::ExExp(new tree::CallExp(new tree::NameExp(func_), argexp_list));
  };

  return new tr::ExpAndTy(exp, ty);

}

tr::ExpAndTy *OpExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                               tr::Level *level, temp::Label *label,
                               err::ErrorMsg *errormsg) const {
  /* Put your lab5 code here */

  tr::ExpAndTy* right_tr = right_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy* left_tr = left_->Translate(venv, tenv, level, label, errormsg);

  tr::Exp *exp = nullptr;
  tree::CjumpStm *stm = nullptr;

  if (oper_ == absyn::OR_OP) {
    temp::Label *z = temp::LabelFactory::NewLabel();
    auto left_cx = left_tr->exp_->UnCx(errormsg);
    auto right_cx = right_tr->exp_->UnCx(errormsg);
    tree::Stm *seqstm = new tree::SeqStm(left_cx.stm_, new tree::SeqStm(new tree::LabelStm(z), right_cx.stm_));
    
    if (left_cx.stm_->kind_ == tree::Stm::CJMUP && right_cx.stm_->kind_ == tree::Stm::CJMUP) {
      static_cast<tree::CjumpStm *>(left_cx.stm_)->false_label_ = z;

      tr::PatchList true_list = tr::PatchList({
        &(static_cast<tree::CjumpStm *>(left_cx.stm_)->true_label_),
        &(static_cast<tree::CjumpStm *>(right_cx.stm_)->true_label_),
      });
      tr::PatchList false_list = tr::PatchList({
        &(static_cast<tree::CjumpStm *>(right_cx.stm_)->false_label_)
      });
      exp = new tr::CxExp(true_list, false_list, seqstm);
      return new tr::ExpAndTy(exp, type::IntTy::Instance());
    }

    if (left_cx.stm_->kind_ == tree::Stm::SEQ && right_cx.stm_->kind_ == tree::Stm::CJMUP) {
      left_cx.falses_.DoPatch(z);
      tr::PatchList true_list = tr::PatchList::JoinPatch(left_cx.trues_, right_cx.trues_);
      tr::PatchList false_list = right_cx.falses_;
      exp = new tr::CxExp(true_list, false_list, seqstm);
      return new tr::ExpAndTy(exp, type::IntTy::Instance());
    } 
  }

  if (oper_ == absyn::AND_OP) {
    temp::Label *z = temp::LabelFactory::NewLabel();
    auto left_cx = left_tr->exp_->UnCx(errormsg);
    auto right_cx = right_tr->exp_->UnCx(errormsg);
    tree::Stm *seqstm = new tree::SeqStm(left_cx.stm_, new tree::SeqStm(new tree::LabelStm(z), right_cx.stm_));
 
    if (typeid(*left_cx.stm_) == typeid(tree::CjumpStm) && typeid(*right_cx.stm_) == typeid(tree::CjumpStm)) {
      static_cast<tree::CjumpStm *>(left_cx.stm_)->true_label_ = z;

      tr::PatchList true_list = tr::PatchList({
        &(static_cast<tree::CjumpStm *>(right_cx.stm_)->true_label_)
      });
      tr::PatchList false_list = tr::PatchList({
        &(static_cast<tree::CjumpStm *>(left_cx.stm_)->false_label_),
        &(static_cast<tree::CjumpStm *>(right_cx.stm_)->false_label_),
      });
      
      exp = new tr::CxExp(true_list, false_list, seqstm);
      return new tr::ExpAndTy(exp, type::IntTy::Instance());
    }

    if (typeid(*left_cx.stm_) == typeid(tree::SeqStm) && typeid(*right_cx.stm_) == typeid(tree::CjumpStm)) {
      left_cx.trues_.DoPatch(z);
      tr::PatchList true_list = right_cx.trues_;
      tr::PatchList false_list = tr::PatchList::JoinPatch(left_cx.falses_, right_cx.falses_);
      exp = new tr::CxExp(true_list, false_list, seqstm);
      return new tr::ExpAndTy(exp, type::IntTy::Instance());
    } 
  }

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
    // compare string
    if (left_tr->ty_->IsSameType(type::StringTy::Instance()) && right_tr->ty_->IsSameType(left_tr->ty_))
      stm = new tree::CjumpStm(
        tree::EQ_OP, 
        frame::ExternalCall("string_equal", new tree::ExpList({left_tr->exp_->UnEx(), right_tr->exp_->UnEx()})), 
        new tree::ConstExp(1), 
        nullptr, 
        nullptr
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
    tr::PatchList true_list({&(stm->true_label_)});
    tr::PatchList false_list({&(stm->false_label_)});
    exp = new tr::CxExp(true_list, false_list, stm);
  }
  return new tr::ExpAndTy(exp, type::IntTy::Instance());
}

tr::ExpAndTy *RecordExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,      
                                   err::ErrorMsg *errormsg) const {
  /* Put your lab5 code here */

  type::Ty* ty = tenv->Look(typ_)->ActualTy();

  if (!ty) {
    // errormsg->Error(pos_, "undefined type %s", typ_->Name().c_str());
    return new tr::ExpAndTy(nullptr, type::IntTy::Instance());
  };

  tree::ExpList* list = new tree::ExpList();

  for (auto efield : fields_->GetList()) {
    tr::ExpAndTy* tmpexp = efield->exp_->Translate(venv, tenv, level, label, errormsg);
    
    list->Append(tmpexp->exp_->UnEx());
  }

  temp::Temp* reg = temp::TempFactory::NewTemp();
  auto args = new tree::ExpList();
  args->Append(new tree::ConstExp(reg_manager->WordSize() * fields_->GetList().size()));
  tree::Stm* stm = new tree::MoveStm(
    new tree::TempExp(reg), 
    frame::ExternalCall("alloc_record", args)
  );

  int count = 0;
  
  for (auto ele : list->GetList()) {
    stm = new tree::SeqStm(
      stm, 
      new tree::MoveStm(
        new tree::MemExp(new tree::BinopExp(
          tree::BinOp::PLUS_OP, 
          new tree::TempExp(reg), 
          new tree::ConstExp(count * reg_manager->WordSize()))), 
        ele
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
  if (!seq_ || seq_->GetList().empty()) return new tr::ExpAndTy(nullptr, type::VoidTy::Instance());

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
  
  tr::Exp *exp = new tr::NxExp(new tree::MoveStm(var_tr->exp_->UnEx(), exp_tr->exp_->UnEx()));
  return new tr::ExpAndTy(exp, type::VoidTy::Instance());
}

tr::ExpAndTy *IfExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                               tr::Level *level, temp::Label *label,
                               err::ErrorMsg *errormsg) const {
  /* Put your lab5 code here */ 

  tr::Exp *exp = nullptr;

  tr::ExpAndTy * test_tr = test_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy * then_tr = then_->Translate(venv, tenv, level, label, errormsg);

  tr::Cx test_cx = test_tr->exp_->UnCx(errormsg);
  temp::Temp* r = temp::TempFactory::NewTemp();
  temp::Label* true_label = temp::LabelFactory::NewLabel();
  temp::Label* false_label = temp::LabelFactory::NewLabel();

  test_cx.trues_.DoPatch(true_label);
  test_cx.falses_.DoPatch(false_label);

  if (elsee_) {
    tr::ExpAndTy * else_tr = elsee_->Translate(venv, tenv, level, label, errormsg);

    temp::Label* done_label = temp::LabelFactory::NewLabel();
    auto done_label_list = new std::vector<temp::Label *>({done_label});
    
    exp = new tr::ExExp(new tree::EseqExp(
      test_cx.stm_,
      new tree::EseqExp(
        new tree::LabelStm(true_label),
        new tree::EseqExp(
          new tree::MoveStm(new tree::TempExp(r), then_tr->exp_->UnEx()),
          new tree::EseqExp(
            new tree::JumpStm(new tree::NameExp(done_label), done_label_list),
            new tree::EseqExp(
              new tree::LabelStm(false_label),
              new tree::EseqExp(
                new tree::MoveStm(new tree::TempExp(r), else_tr->exp_->UnEx()),
                new tree::EseqExp(
                  new tree::JumpStm(new tree::NameExp(done_label), done_label_list),
                  new tree::EseqExp(
                    new tree::LabelStm(done_label), 
                    new tree::TempExp(r)
                  )
                )
              )
            )
          )
        )
      )
    ));
  }
  else {
    exp = new tr::NxExp(new tree::SeqStm(
      test_cx.stm_, 
      new tree::SeqStm(
        new tree::LabelStm(true_label), 
        new tree::SeqStm(then_tr->exp_->UnNx(), 
        new tree::LabelStm(false_label))
      )
    ));
  }

  return new tr::ExpAndTy(exp, then_tr->ty_);
}

tr::ExpAndTy *WhileExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,            
                                  err::ErrorMsg *errormsg) const {
  /* Put your lab5 code here */ 

  tr::Exp *exp = nullptr;

  temp::Label* test_label = temp::LabelFactory::NewLabel();
  temp::Label* body_label = temp::LabelFactory::NewLabel();
  temp::Label* done_label = temp::LabelFactory::NewLabel();

  tr::ExpAndTy * test_tr = test_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy * body_tr = body_->Translate(venv, tenv, level, done_label, errormsg);

  tr::Cx test_cx = test_tr->exp_->UnCx(errormsg);

  test_cx.trues_.DoPatch(body_label);
  test_cx.falses_.DoPatch(done_label);

  auto test_label_list = new std::vector<temp::Label *>(1, test_label);

  exp = new tr::NxExp(new tree::SeqStm(
    new tree::LabelStm(test_label),
    new tree::SeqStm(
      test_cx.stm_,
      new tree::SeqStm(
        new tree::LabelStm(body_label),
        new tree::SeqStm(
          body_tr->exp_->UnNx(),
          new tree::SeqStm(
            new tree::JumpStm(new tree::NameExp(test_label), test_label_list),
            new tree::LabelStm(done_label)
          )
        )
      )
    )
  ));
  return new tr::ExpAndTy(exp, type::VoidTy::Instance());
}

tr::ExpAndTy *ForExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* Put your lab5 code here */ 

  tr::Exp *exp = nullptr;

  tr::ExpAndTy * hi_tr = hi_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy * lo_tr = lo_->Translate(venv, tenv, level, label, errormsg);

  temp::Label *body_label = temp::LabelFactory::NewLabel();
  temp::Label *test_label = temp::LabelFactory::NewLabel();
  temp::Label *done_label = temp::LabelFactory::NewLabel();

  venv->BeginScope();

  tr::Access * access = tr::Access::AllocLocal(level, escape_);
  venv->Enter(var_, new env::VarEntry(access, lo_tr->ty_, true));
  
  tr::ExpAndTy * body_tr = body_->Translate(venv, tenv, level, done_label, errormsg);
  
  venv->EndScope();
  
  tree::Exp *i = access->access_->toExp(new tree::TempExp(reg_manager->FramePointer()));
  auto test_label_list = new std::vector<temp::Label *>({test_label});
  exp = new tr::NxExp(new tree::SeqStm(
    new tree::MoveStm(i, lo_tr->exp_->UnEx()),
    new tree::SeqStm(
      new tree::LabelStm(test_label),
      new tree::SeqStm(
        new tree::CjumpStm(tree::RelOp::LE_OP, i, hi_tr->exp_->UnEx(), body_label, done_label),
        new tree::SeqStm(
          new tree::LabelStm(body_label),
          new tree::SeqStm(
            body_tr->exp_->UnNx(),
            new tree::SeqStm(
              new tree::MoveStm(i, new tree::BinopExp(tree::BinOp::PLUS_OP, i, new tree::ConstExp(1))),
              new tree::SeqStm(
                new tree::JumpStm(new tree::NameExp(test_label), test_label_list),
                new tree::LabelStm(done_label)
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

  tree::Stm* stm = new tree::JumpStm(new tree::NameExp(label), new std::vector<temp::Label *>({label}));
  tr::Exp* nx = new tr::NxExp(stm);
  return new tr::ExpAndTy(nx, type::VoidTy::Instance());
}

tr::ExpAndTy *LetExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  
  static bool first = true;
  bool isMain = false;
  if (first) {
    isMain = true;
    first = false;
  };

  tree::Stm* stm = nullptr;
  tree::Exp* res = nullptr;
  type::Ty* ty = type::VoidTy::Instance();

  venv->BeginScope();
  tenv->BeginScope();
  
  if (decs_ != nullptr) {
    std::list<Dec *> list = decs_->GetList();
    auto iter = list.begin();
    stm = (*iter)->Translate(venv, tenv, level, label, errormsg)->UnNx();
    iter++;
    for (;iter != list.end(); iter++) {
      stm = new tree::SeqStm(stm, (*iter)->Translate(venv, tenv, level, label, errormsg)->UnNx());
    };
  };

  tr::ExpAndTy* body_tr = body_->Translate(venv, tenv, level, label, errormsg);

  venv->EndScope();
  tenv->EndScope();
  
  if (stm != nullptr) res = new tree::EseqExp(stm, body_tr->exp_->UnEx());
  else res = body_tr->exp_->UnEx();

  if (isMain) {
    frags->PushBack(new frame::ProcFrag(new tree::ExpStm(res), level->frame_));
    isMain = false;
  };
  return new tr::ExpAndTy(new tr::ExExp(res), body_tr->ty_->ActualTy());

  // static bool isMain = true;
  
  // venv->BeginScope();
  // tenv->BeginScope();

  // std::list<tr::Exp *> dec_tr_list;
  
  // for (auto dec : decs_->GetList()) {
  //     dec_tr_list.push_back(dec->Translate(venv, tenv, level, label, errormsg));
  // }
  
  // tr::ExpAndTy * body_tr = body_->Translate(venv, tenv, level, label, errormsg);

  // tr::Exp *exp = nullptr;
  // for (auto dec_tr : dec_tr_list) {
  //   if (exp = nullptr) {
  //     exp = dec_tr;
  //   } else if (dec_tr) {
  //     exp = new tr::ExExp(new tree::EseqExp(exp->UnNx(), dec_tr->UnEx()));
  //   } else {
  //     exp = new tr::ExExp(new tree::EseqExp(exp->UnNx(), new tree::ConstExp(0)));
  //   }
  // }

  // venv->EndScope();
  // tenv->EndScope();

  // tree::Exp *te = nullptr;
  // if (exp == nullptr) {
  //   exp = body_tr->exp_;
  //   te = exp->UnEx();
  // } else {
  //   te = new tree::EseqExp(exp->UnNx(), body_tr->exp_->UnEx());
  //   exp = new tr::ExExp(te);
  // }
  // if (isMain) {
  //   frags->PushBack(new frame::ProcFrag(new tree::ExpStm(te), level->frame_));
  //   isMain = false;
  // };

  // return new tr::ExpAndTy(exp, body_tr->ty_);
}

tr::ExpAndTy *ArrayExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,                    
                                  err::ErrorMsg *errormsg) const {
  /* Put your lab5 code here */ 

  type::Ty* ty = tenv->Look(typ_)->ActualTy();
  if (!ty) {
    // errormsg->Error(pos_, "undefined type %s", typ_->Name().c_str());
    return new tr::ExpAndTy(nullptr, type::IntTy::Instance());
  };

  tr::ExpAndTy* size_tr = size_->Translate(venv, tenv, level, label, errormsg);

  tr::ExpAndTy* init_tr = init_->Translate(venv, tenv, level, label, errormsg);
 
  tree::ExpList *exp_list = new tree::ExpList();
  exp_list->Append(size_tr->exp_->UnEx());
  exp_list->Append(init_tr->exp_->UnEx());
  tr::Exp* exp = new tr::ExExp(frame::ExternalCall("init_array", exp_list));
  return new tr::ExpAndTy(exp, ty);
}

tr::ExpAndTy *VoidExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                 tr::Level *level, temp::Label *label,
                                 err::ErrorMsg *errormsg) const {
  /* Put your lab5 code here */
  return new tr::ExpAndTy(nullptr, type::VoidTy::Instance());
}

tr::Exp *FunctionDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* Put your lab5 code here */

  const auto func_list = functions_->GetList();

  // add all declarations to the environment
  for (auto func : func_list) {
    
    type::TyList *ty_list = func->params_->MakeFormalTyList(tenv, errormsg);

    std::list<bool> escapes;
    for (absyn::Field *field : func->params_->GetList()) {
      escapes.push_back(field->escape_);
    }

    tr::Level* new_level = new tr::Level(frame::NewFrame(func->name_, escapes), level);
    
    if (!func->result_) {
      // no return value
      venv->Enter(func->name_, new env::FunEntry(new_level, func->name_, ty_list, type::VoidTy::Instance()));
    }
    else {
      type::Ty* result = tenv->Look(func->result_);
      venv->Enter(func->name_, new env::FunEntry(new_level, func->name_, ty_list, result));
    }
  }

  // translate each function
  for (auto func : func_list) {
    venv->BeginScope();
    env::FunEntry* func_entry = (env::FunEntry *)venv->Look(func->name_);

    auto type_iter = func_entry->formals_->GetList().begin();
    const auto access_list = func_entry->level_->frame_->formals_;
    auto access_iter = access_list.begin();

    for (auto& param : func->params_->GetList()) {
      venv->Enter(param->name_, new env::VarEntry(new tr::Access(func_entry->level_, *access_iter), *type_iter));
      type_iter++;
      access_iter++;
    }

    tr::ExpAndTy * body_tr = func->body_->Translate(venv, tenv, func_entry->level_, label, errormsg);
    venv->EndScope();

    frags->PushBack(new frame::ProcFrag(
      frame::ProcEntryExit1(
        func_entry->level_->frame_, 
        new tree::MoveStm(new tree::TempExp(reg_manager->ReturnValue()), body_tr->exp_->UnEx())
        ), 
      func_entry->level_->frame_
      ));
  }
  
  return new tr::ExExp(new tree::ConstExp(0));
}

tr::Exp *VarDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                           tr::Level *level, temp::Label *label,
                           err::ErrorMsg *errormsg) const {
  /* Put your lab5 code here */
  
  tr::ExpAndTy *init_tr = init_->Translate(venv, tenv, level, label, errormsg);

  tr::Access *access = tr::Access::AllocLocal(level, this->escape_);
  type::Ty* ty = (typ_) ? (tenv->Look(typ_)) : (init_tr->ty_);

  venv->Enter(this->var_, new env::VarEntry(access, ty->ActualTy()));

  tr::Exp *var_exp = new tr::ExExp(access->access_->toExp(tr::StaticLink(level, access->level_)));

  return new tr::NxExp(new tree::MoveStm(var_exp->UnEx(), init_tr->exp_->UnEx()));
}

tr::Exp *TypeDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                            tr::Level *level, temp::Label *label,
                            err::ErrorMsg *errormsg) const {
  /* Put your lab5 code here */

  const auto ty_list = types_->GetList();
  
  // for (auto name_and_type : ty_list) {
  //   for (auto check_name : ty_list) {
  //     if (check_name->name_->Name() == name_and_type->name_->Name()) {
  //       errormsg->Error(pos_, "two types have the same name");
  //     }
  //   }
  //   tenv->Enter(name_and_type->name_, new type::NameTy(name_and_type->name_, nullptr));
  // }

  auto tylist_iter = ty_list.begin();
  while (tylist_iter != ty_list.end()) {
    auto tylist_next_iter = tylist_iter;
    tylist_next_iter++;
    while (tylist_next_iter != ty_list.end()) {
      if ((*tylist_iter)->name_ == (*tylist_next_iter)->name_)
        errormsg->Error(pos_, "two types have the same name");
      tylist_next_iter++;
    };
    tenv->Enter((*tylist_iter)->name_, new type::NameTy((*tylist_iter)->name_, nullptr));
    tylist_iter++;
  };

  for (auto name_and_type : ty_list) {
    type::NameTy *name_ty = (type::NameTy *)(tenv->Look(name_and_type->name_));
    name_ty->ty_ = name_and_type->ty_->Translate(tenv, errormsg);
  }

  for (auto name_and_type : ty_list) {
    type::Ty *ty = tenv->Look(name_and_type->name_);
    std::vector<sym::Symbol *> trace;
    while (typeid(*ty) == typeid(type::NameTy)) {
      type::NameTy *name_ty = static_cast<type::NameTy *>(ty);
      for (sym::Symbol *symbol : trace) {
        if (symbol->Name() == name_ty->sym_->Name()) {
          errormsg->Error(pos_, "illegal type cycle");
        }
      }
      trace.push_back(name_ty->sym_);
      ty = name_ty->ty_;
    }
  }

  return new tr::ExExp(new tree::ConstExp(0));
}

type::Ty *NameTy::Translate(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  /* Put your lab5 code here */

  type::Ty* ty = tenv->Look(name_);
  if (!ty) {
    // errormsg->Error(pos_, "undefined type %s", name_->Name().c_str());
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
    // errormsg->Error(pos_, "undefined type %s", array_->Name().c_str());
    return type::IntTy::Instance();
  }
  return new type::ArrayTy(ty);
}

} // namespace absyn
