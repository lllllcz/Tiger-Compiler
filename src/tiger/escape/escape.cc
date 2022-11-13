#include "tiger/escape/escape.h"
#include "tiger/absyn/absyn.h"

namespace esc {
void EscFinder::FindEscape() { absyn_tree_->Traverse(env_.get()); }
} // namespace esc

namespace absyn {

void AbsynTree::Traverse(esc::EscEnvPtr env) {
  /* Put your lab5 code here */
  root_->Traverse(env, 0);
}

void SimpleVar::Traverse(esc::EscEnvPtr env, int depth) {
  /* Put your lab5 code here */
  esc::EscapeEntry* t = env->Look(sym_);
  if (depth > t->depth_)
    *(t->escape_) = true;
}

void FieldVar::Traverse(esc::EscEnvPtr env, int depth) {
  /* Put your lab5 code here */
  var_->Traverse(env, depth);
}

void SubscriptVar::Traverse(esc::EscEnvPtr env, int depth) {
  /* Put your lab5 code here */
  var_->Traverse(env, depth);
  subscript_->Traverse(env, depth);
}

void VarExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* Put your lab5 code here */
  var_->Traverse(env, depth);
}

void NilExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* Put your lab5 code here */
}

void IntExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* Put your lab5 code here */
}

void StringExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* Put your lab5 code here */
}

void CallExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* Put your lab5 code here */
  auto list = args_->GetList();
  for (auto ele : list)
    ele->Traverse(env, depth);
}

void OpExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* Put your lab5 code here */
  left_->Traverse(env, depth);
  right_->Traverse(env, depth);
}

void RecordExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* Put your lab5 code here */
  auto list = fields_->GetList();
  for (auto ele : list)
    ele->exp_->Traverse(env, depth);
}

void SeqExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* Put your lab5 code here */
  auto list = seq_->GetList();
  for (auto ele : list)
    ele->Traverse(env, depth);
}

void AssignExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* Put your lab5 code here */
  var_->Traverse(env, depth);
  exp_->Traverse(env, depth);
}

void IfExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* Put your lab5 code here */
  test_->Traverse(env, depth);
  then_->Traverse(env, depth);
  if (elsee_) elsee_->Traverse(env, depth);
}

void WhileExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* Put your lab5 code here */
  test_->Traverse(env, depth);
  if (body_) body_->Traverse(env, depth);
}

void ForExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* Put your lab5 code here */
  lo_->Traverse(env, depth);
  hi_->Traverse(env, depth);
  if (body_) {
    escape_ = false;
    env->Enter(var_, new esc::EscapeEntry(depth, &(escape_)));
    body_->Traverse(env, depth);
  }
}

void BreakExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* Put your lab5 code here */
}

void LetExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* Put your lab5 code here */
  auto list = decs_->GetList();
  for (auto ele : list)
    ele->Traverse(env, depth);
  if (body_) body_->Traverse(env, depth);
}

void ArrayExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* Put your lab5 code here */
  size_->Traverse(env, depth);
  init_->Traverse(env, depth);
}

void VoidExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* Put your lab5 code here */
}

void FunctionDec::Traverse(esc::EscEnvPtr env, int depth) {
  /* Put your lab5 code here */
  auto fuclist = functions_->GetList();
  for (auto fuc : fuclist) {
    env->BeginScope();

    auto paramlist = fuc->params_->GetList();
    for (auto param : paramlist) {
      param->escape_ = false;
      env->Enter(param->name_, new esc::EscapeEntry(depth + 1, &param->escape_));
    };
    fuc->body_->Traverse(env, depth + 1);

    env->EndScope();
  };
}

void VarDec::Traverse(esc::EscEnvPtr env, int depth) {
  /* Put your lab5 code here */
  init_->Traverse(env, depth);
  escape_ = false;
  env->Enter(var_, new esc::EscapeEntry(depth, &escape_));
}

void TypeDec::Traverse(esc::EscEnvPtr env, int depth) {
  /* Put your lab5 code here */
}

} // namespace absyn
