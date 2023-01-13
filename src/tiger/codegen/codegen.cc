#include "tiger/codegen/codegen.h"

#include <cassert>
#include <sstream>

extern frame::RegManager *reg_manager;

namespace {

constexpr int maxlen = 1024;


} // namespace

namespace cg {

void CodeGen::Codegen() {
  /* TODO: Put your lab5 code here */
  fs_ = frame_->label_name_->Name();

  assem::InstrList* instr_list = new assem::InstrList();

  std::string instr = "movq `s0, `d0";
  auto saved_regs = new std::list<temp::Temp *>();
  auto callee_save_regs = reg_manager->CalleeSaves()->GetList();

  for (auto reg : callee_save_regs) {
    auto saved = temp::TempFactory::NewTemp();
    saved_regs->push_back(saved);
    instr_list->Append(new assem::MoveInstr(instr, new temp::TempList(saved), new temp::TempList(reg)));
  }
  
  std::list<tree::Stm *> trace_list = traces_->GetStmList()->GetList();
  for (auto trace : trace_list)
  {
    assert(trace != NULL);
    trace->Munch(*instr_list, fs_);
  }

  for (auto reg : callee_save_regs) {
    auto saved = saved_regs->front();
    instr_list->Append(new assem::MoveInstr(instr, new temp::TempList(reg), new temp::TempList(saved)));
    saved_regs->pop_front();
  }
  assem_instr_ = std::make_unique<cg::AssemInstr>(frame::ProcEntryExit2(instr_list));
  return;
};

void AssemInstr::Print(FILE *out, temp::Map *map) const {
  for (auto instr : instr_list_->GetList())
    instr->Print(out, map);
  fprintf(out, "\n");
}
} // namespace cg

namespace tree {
/* TODO: Put your lab5 code here */

void SeqStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* Put your lab5 code here */
  left_->Munch(instr_list, fs);
  right_->Munch(instr_list, fs);
}

void LabelStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* Put your lab5 code here */
  instr_list.Append(new assem::LabelInstr(temp::LabelFactory::LabelString(label_), label_));
}

void JumpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* Put your lab5 code here */
  auto jump_instr = new assem::OperInstr(
      "jmp `j0", 
      nullptr, 
      nullptr, 
      new assem::Targets(jumps_)
    );
  instr_list.Append(jump_instr);
}

void CjumpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* Put your lab5 code here */

  temp::Temp *left_m = left_->Munch(instr_list, fs);
  temp::Temp *right_m = right_->Munch(instr_list, fs);
  std::string instr;
  switch (op_)
  {
  case tree::RelOp::EQ_OP: instr = "je"; break;
  case tree::RelOp::NE_OP: instr = "jne"; break;
  case tree::RelOp::LT_OP: instr = "jl"; break;
  case tree::RelOp::LE_OP: instr = "jle"; break;
  case tree::RelOp::GE_OP: instr = "jge"; break;
  case tree::RelOp::GT_OP: instr = "jg"; break;
  default:
    break;
  }

  auto label_list = new std::vector<temp::Label *>({true_label_, false_label_});

  instr_list.Append(
    new assem::OperInstr(
      "cmpq `s0, `s1", 
      nullptr, 
      new temp::TempList({right_m, left_m}), 
      nullptr
    )
  );
  instr_list.Append(
    new assem::OperInstr(
      instr + " `j0", 
      nullptr, 
      nullptr, 
      new assem::Targets(label_list)
    )
  );

}

void MoveStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* Put your lab5 code here */
  if (typeid(*dst_) == typeid(tree::TempExp)) {
    tree::TempExp *dst_temp_ = (tree::TempExp *)dst_;
    temp::Temp *src_m = src_->Munch(instr_list, fs);
    instr_list.Append(
      new assem::MoveInstr(
        "movq `s0, `d0",
        new temp::TempList(dst_temp_->temp_),
        new temp::TempList(src_m)
      )
    );
    return;
  }

  if (typeid(*dst_) == typeid(tree::MemExp)) {
    tree::MemExp *dst_mem_ = (tree::MemExp *)dst_;
    temp::Temp *src_m = src_->Munch(instr_list, fs);
    temp::Temp *dst_m = dst_mem_->exp_->Munch(instr_list, fs);
    instr_list.Append(
      new assem::OperInstr(
        "movq `s0, (`s1)",
        nullptr,
        new temp::TempList({src_m, dst_m}),
        nullptr
      )
    );
    return;
  }

}

void ExpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* Put your lab5 code here */
  exp_->Munch(instr_list, fs);
}

temp::Temp *BinopExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* Put your lab5 code here */

  temp::Temp *left_m = left_->Munch(instr_list, fs);
  temp::Temp *right_m = right_->Munch(instr_list, fs);

  temp::Temp *dst_reg = temp::TempFactory::NewTemp();

  instr_list.Append(
    new assem::MoveInstr(
      "movq `s0, `d0",
      new temp::TempList(dst_reg),
      new temp::TempList(left_m)
    )
  );

  switch (op_)
  {
  case tree::PLUS_OP: {
    instr_list.Append(
      new assem::OperInstr(
        "addq `s0, `d0",
        new temp::TempList(dst_reg),
        new temp::TempList({right_m, dst_reg}),
        nullptr
      )
    );
    break;
  }
  case tree::MINUS_OP: {
    instr_list.Append(
      new assem::OperInstr(
        "subq `s0, `d0",
        new temp::TempList(dst_reg),
        new temp::TempList({right_m, dst_reg}),
        nullptr
      )
    );
    break;
  }
  case tree::MUL_OP: {
    instr_list.Append(
      new assem::MoveInstr(
        "movq `s0, `d0",
        new temp::TempList({reg_manager->GetX64rax()}),
        new temp::TempList({dst_reg})
      )
    );
    instr_list.Append(
      new assem::OperInstr(
        "imulq `s0",
        reg_manager->CalculateRegs(),
        new temp::TempList({right_m}), 
        nullptr
      )
    );
    instr_list.Append(
      new assem::MoveInstr(
        "movq `s0, `d0", 
        new temp::TempList({dst_reg}),
        new temp::TempList({reg_manager->GetX64rax()})
      )
    );
    break;
  }
  case tree::DIV_OP: {
    instr_list.Append(
      new assem::MoveInstr(
        "movq `s0, `d0",
        new temp::TempList({reg_manager->GetX64rax()}),
        new temp::TempList({dst_reg})
      )
    );
    instr_list.Append(
      new assem::OperInstr(
        "cqto",
        reg_manager->CalculateRegs(),
        new temp::TempList({reg_manager->GetX64rax()}),
        nullptr
      )
    );
    instr_list.Append(
      new assem::OperInstr(
        "idivq `s0",
        reg_manager->CalculateRegs(), 
        new temp::TempList({right_m}), 
        nullptr
      )
    );
    instr_list.Append(
      new assem::MoveInstr(
        "movq `s0, `d0",
        new temp::TempList({dst_reg}), 
        new temp::TempList({reg_manager->GetX64rax()})
      )
    );
    break;
  }
  case tree::AND_OP: {
    // instr_list.Append(
    //   new assem::OperInstr(
    //     "andq `s0, `d0",
    //     new temp::TempList(dst_reg),
    //     new temp::TempList(right_m),
    //     nullptr
    //   )
    // );
    break;
  }
  case tree::OR_OP: {
    // instr_list.Append(
    //   new assem::OperInstr(
    //     "orq `s0, `d0",
    //     new temp::TempList(dst_reg),
    //     new temp::TempList(right_m),
    //     nullptr
    //   )
    // );
    break;
  }
  default:
    break;
  }
  
  return dst_reg;
}

temp::Temp *MemExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* Put your lab5 code here */

  temp::Temp *dst_reg = temp::TempFactory::NewTemp();
  temp::Temp *src_mem = exp_->Munch(instr_list, fs);
  instr_list.Append(
    new assem::OperInstr(
      "movq (`s0), `d0", 
      new temp::TempList(dst_reg), 
      new temp::TempList(src_mem), 
      nullptr
    )
  );
  return dst_reg;
}

temp::Temp *TempExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* Put your lab5 code here */
  if (temp_ != reg_manager->FramePointer()) {
    return temp_;
  }

  temp::Temp *dst_reg = temp::TempFactory::NewTemp();
  // std::string instr = "leaq " + std::string(fs.data()) + "(`s0), `d0";
  std::stringstream stream;
  stream << "leaq " << fs << "_framesize(`s0), `d0";
  std::string instr = stream.str();
  instr_list.Append(
    new assem::OperInstr(
      instr,
      new temp::TempList(dst_reg),
      new temp::TempList(reg_manager->StackPointer()),
      nullptr
    )
  );
  return dst_reg;
}

temp::Temp *EseqExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* Put your lab5 code here */
  stm_->Munch(instr_list, fs);
  return exp_->Munch(instr_list, fs);
}

temp::Temp *NameExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* Put your lab5 code here */

  temp::Temp *dst_reg = temp::TempFactory::NewTemp();
  std::string instr = "leaq " + temp::LabelFactory::LabelString(this->name_) + "(%rip), `d0";
  instr_list.Append(
    new assem::OperInstr(
      instr, 
      new temp::TempList(dst_reg), 
      nullptr, 
      nullptr
    )
  );
  return dst_reg;
}

temp::Temp *ConstExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* Put your lab5 code here */

  temp::Temp *dst_reg = temp::TempFactory::NewTemp();
  std::string instr = "movq $" + std::to_string(consti_) + ", `d0";
  instr_list.Append(
    new assem::OperInstr(
      instr, 
      new temp::TempList(dst_reg), 
      nullptr, 
      nullptr
    )
  );
  return dst_reg;
}

temp::Temp *CallExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* Put your lab5 code here */

  temp::Temp *dst_reg = temp::TempFactory::NewTemp();

  auto func_name = (tree::NameExp *) fun_;
  std::string instr = "callq " + temp::LabelFactory::LabelString(func_name->name_);
  
  args_->MunchArgs(instr_list, fs);

  instr_list.Append(
    new assem::OperInstr(
      instr,
      reg_manager->CallerSaves(),
      reg_manager->ArgRegs(),
      nullptr
    )
  );
  instr_list.Append(
    new assem::MoveInstr(
      "movq `s0, `d0",
      new temp::TempList(dst_reg),
      new temp::TempList(reg_manager->ReturnValue())
    )
  );

  return dst_reg;
}

temp::TempList *ExpList::MunchArgs(assem::InstrList &instr_list, std::string_view fs) {
  /* Put your lab5 code here */

  temp::TempList *reg_list = new temp::TempList();

  auto iter = reg_manager->ArgRegs()->GetList().begin();
  int i = 0;
  for (auto exp : exp_list_) {
    temp::Temp *arg = exp->Munch(instr_list, fs);
    if (i < 6) {
      instr_list.Append(
        new assem::MoveInstr(
          "movq `s0, `d0",
          new temp::TempList(*iter),
          new temp::TempList(arg)
        )
      );
      reg_list->Append(*iter);
      iter++;
    }
    else {
      std::string instr = "movq `s0, " + std::to_string((i-6) * reg_manager->WordSize()) + "(`s1)";
      instr_list.Append(
        new assem::OperInstr(
          instr,
          nullptr,
          new temp::TempList({arg, reg_manager->StackPointer()}),
          nullptr
        )
      );
    }
    i += 1;
  }

  return reg_list;
}

} // namespace tree
