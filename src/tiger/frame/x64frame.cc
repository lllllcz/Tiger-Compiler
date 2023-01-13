#include "tiger/frame/x64frame.h"

extern frame::RegManager *reg_manager;

namespace frame {
/* Put your lab5 code here */
class InFrameAccess : public Access {
public:
  int offset;

  explicit InFrameAccess(int offset) : offset(offset) {}
  /* Put your lab5 code here */
  
  tree::Exp *toExp(tree::Exp *fp = nullptr) const override {
    return new tree::MemExp(new tree::BinopExp(tree::BinOp::PLUS_OP, fp, new tree::ConstExp(offset)));
  }

};


class InRegAccess : public Access {
public:
  temp::Temp *reg;

  explicit InRegAccess(temp::Temp *reg) : reg(reg) {}
  /* Put your lab5 code here */
  
  tree::Exp *toExp(tree::Exp *fp = nullptr) const override {
    return new tree::TempExp(reg);
  }

};

class X64Frame : public Frame {
  /* Put your lab5 code here */
public:
  X64Frame(temp::Label* name, std::list<bool> escapes) : Frame(name, escapes) {
    this->frame_offset = -reg_manager->WordSize();
    for (auto escape : escapes) {
      formals_.push_back(allocLocal(escape));
    };
  };
  Access* allocLocal(bool escape) override {
    frame::Access *access;
    if (escape) {
      frame_offset -= reg_manager->WordSize();
      access = new InFrameAccess(frame_offset);
    } else {
      access = new InRegAccess(temp::TempFactory::NewTemp());
    }
    return access;
  };
};

/* Put your lab5 code here */

X64RegManager::X64RegManager() : RegManager() {
    std::vector<std::string *> names;

    names.push_back(new std::string("%rax"));
    names.push_back(new std::string("%rbx"));
    names.push_back(new std::string("%rcx"));
    names.push_back(new std::string("%rdx"));
    names.push_back(new std::string("%rsi"));
    names.push_back(new std::string("%rdi"));
    names.push_back(new std::string("%rbp"));
    names.push_back(new std::string("%rsp"));
    names.push_back(new std::string("%r8"));
    names.push_back(new std::string("%r9"));
    names.push_back(new std::string("%r10"));
    names.push_back(new std::string("%r11"));
    names.push_back(new std::string("%r12"));
    names.push_back(new std::string("%r13"));
    names.push_back(new std::string("%r14"));
    names.push_back(new std::string("%r15"));

    for (auto &name : names) {
      auto reg = temp::TempFactory::NewTemp();
      regs_.push_back(reg);
      temp_map_->Enter(reg, name);
    }
  };

temp::TempList* X64RegManager::Registers() {
  temp::TempList* templist = new temp::TempList();
  for (int i = 0; i < 16; i++) {
    if (i == 7) continue;
    templist->Append(regs_[i]);
  }
  return templist;
};

temp::TempList* X64RegManager::ArgRegs() {
  temp::TempList* templist = new temp::TempList();
  templist->Append(regs_[5]); //rdi
  templist->Append(regs_[4]); //rsi
  templist->Append(regs_[3]); //rdx
  templist->Append(regs_[2]); //rcx
  templist->Append(regs_[8]); //r8
  templist->Append(regs_[9]); //r9
  return templist;
};

temp::TempList* X64RegManager::CallerSaves() {
  temp::TempList* templist = new temp::TempList();
  templist->Append(regs_[0]); //rax
  templist->Append(regs_[2]); //rcx
  templist->Append(regs_[3]); //rdx
  templist->Append(regs_[4]); //rsi
  templist->Append(regs_[5]); //rdi
  templist->Append(regs_[8]); //r8
  templist->Append(regs_[9]); //r9
  templist->Append(regs_[10]); //r10
  templist->Append(regs_[11]); //r11
  return templist;
};
temp::TempList* X64RegManager::CalleeSaves() {
  temp::TempList* templist = new temp::TempList();
  templist->Append(regs_[1]); //rbx
  templist->Append(regs_[6]); //rbp
  templist->Append(regs_[12]); //r12
  templist->Append(regs_[13]); //r13
  templist->Append(regs_[14]); //r14
  templist->Append(regs_[15]); //r15
  return templist; 
};

temp::TempList* X64RegManager::ReturnSink() {
  temp::TempList *temp_list = CalleeSaves();
  temp_list->Append(StackPointer());
  temp_list->Append(ReturnValue());
  return temp_list;
};

int X64RegManager::WordSize() {
  return 8;
};

temp::Temp* X64RegManager::FramePointer() {
  return regs_[6];
};

temp::Temp* X64RegManager::StackPointer() {
  return regs_[7];
};

temp::Temp* X64RegManager::ReturnValue() {
  return regs_[0];
};

temp::TempList *X64RegManager::CalculateRegs() {
  auto temp_list = new temp::TempList();
  temp_list->Append(regs_[3]); //%rdx
  temp_list->Append(regs_[0]); //%rax
  return temp_list;
}

temp::Temp *X64RegManager::GetX64rax() {
  return regs_[0];
}


Frame *NewFrame(temp::Label *label, const std::list<bool> &formals) {
  return new X64Frame(label, formals);
}

tree::Exp* ExternalCall(const std::string name, tree::ExpList* args) {
  return new tree::CallExp(new tree::NameExp(temp::LabelFactory::NamedLabel(name)), args);
};

tree::Stm* ProcEntryExit1(Frame* frame, tree::Stm* body) {
  tree::Stm *view_shift = nullptr, *stm = nullptr;
  tree::Exp *src, *dst;

  bool reg_over = false;
  int i = 0;
  const auto &reg_list = reg_manager->ArgRegs()->GetList();
  auto iter = reg_list.begin();

  for (frame::Access *access : frame->formals_) {
    dst = access->toExp(new tree::TempExp(reg_manager->FramePointer()));
    if (reg_over) {
      i += 1;
      src = new tree::MemExp(new tree::BinopExp(
        tree::BinOp::PLUS_OP,
        new tree::TempExp(reg_manager->FramePointer()), 
        new tree::ConstExp(i * reg_manager->WordSize())
      ));
    }
    else {
      src = new tree::TempExp(*iter);
      iter++;
      reg_over = (iter == reg_list.end());
    }
    stm = new tree::MoveStm(dst, src);
    view_shift = view_shift ? new tree::SeqStm(view_shift, stm) : stm;
  }
  return new tree::SeqStm(view_shift, body);
};

assem::InstrList* ProcEntryExit2(assem::InstrList* body) {
  body->Append(new assem::OperInstr("", new temp::TempList(), reg_manager->ReturnSink(), nullptr));
  return body;
};

assem::Proc* ProcEntryExit3(frame::Frame* frame, assem::InstrList* body) {
  int frame_args = std::max(frame->maxArgs - 6, 0) * reg_manager->WordSize();
  std::string prolog =
    ".set " + frame->label_name_->Name() + "_framesize, " + std::to_string(-(frame->frame_offset)) + "\n"
    + frame->label_name_->Name() + ":\n"
    + "subq $" + std::to_string(frame_args - frame->frame_offset) + ", %rsp\n"
    ;
  std::string epilog = 
    "addq $" + std::to_string(frame_args - frame->frame_offset) + ", %rsp\n" 
    + "retq\n"
    ;
  return new assem::Proc(prolog, body, epilog);
};

} // namespace frame
