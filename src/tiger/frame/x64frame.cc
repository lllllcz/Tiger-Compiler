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
  explicit X64Frame(temp::Label* name, std::list<bool> escapes) : Frame(name) {
    offset = -8;
    for (auto escape : escapes) {
      formals_.push_back(allocLocal(escape));
    }
  };

  frame::Access *allocLocal(bool escape) override {
    if (escape) {
      offset -= reg_manager->WordSize();
      return new InFrameAccess(offset);
    } else {
      return new InRegAccess(temp::TempFactory::NewTemp());
    }
  }

};
/* Put your lab5 code here */

X64RegManager::X64RegManager () {
  std::vector<std::string> reg_names = {
    "rax",
    "rbx",
    "rcx",
    "rdx",
    "rsi",
    "rdi",
    "rbp",
    "rsp",
    "r8",
    "r9",
    "r10",
    "r11",
    "r12",
    "r13",
    "r14",
    "r15"
  };

  for (auto name: reg_names) {
    temp::Temp * reg = temp::TempFactory::NewTemp();
    regs_.push_back(reg);
    temp_map_->Enter(reg, &name);
  }

}

temp::TempList *X64RegManager::Registers() {

  temp::TempList * temp_list = new temp::TempList();

  for (int i = 0; i < 16; i++) {

    // skip %rsp
    if (i == 7) continue;

    temp_list->Append(GetRegister(i));
  }

  return temp_list;
}

temp::TempList *X64RegManager::ArgRegs() {

  temp::TempList *temp_list = new temp::TempList();

  temp_list->Append(GetRegister(5)); //%rdi
  temp_list->Append(GetRegister(4)); //%rsi
  temp_list->Append(GetRegister(3)); //%rdx
  temp_list->Append(GetRegister(2)); //%rcx
  temp_list->Append(GetRegister(8)); //%r8
  temp_list->Append(GetRegister(9)); //%r9

  return temp_list;
}

temp::TempList *X64RegManager::CallerSaves() {

  temp::TempList * temp_list = new temp::TempList();
  
  temp_list->Append(GetRegister(0));  //%rax
  temp_list->Append(GetRegister(2));  //%rcx
  temp_list->Append(GetRegister(3));  //%rdx
  temp_list->Append(GetRegister(4));  //%rsi
  temp_list->Append(GetRegister(5));  //%rdi
  temp_list->Append(GetRegister(8));  //%r8
  temp_list->Append(GetRegister(9));  //%r9
  temp_list->Append(GetRegister(10)); //%r10
  temp_list->Append(GetRegister(11)); //%r11
  
  return temp_list;
}

temp::TempList *X64RegManager::CalleeSaves() {

  temp::TempList * temp_list = new temp::TempList();
  
  temp_list->Append(GetRegister(1));  //%rbx
  temp_list->Append(GetRegister(6));  //%rbp
  temp_list->Append(GetRegister(12)); //%r12
  temp_list->Append(GetRegister(13)); //%r13
  temp_list->Append(GetRegister(14)); //%r14
  temp_list->Append(GetRegister(15)); //%r15
  
  return temp_list;
}

temp::TempList *X64RegManager::ReturnSink() {
  return nullptr;
}

int X64RegManager::WordSize() {
  return 8;
}

temp::Temp *X64RegManager::FramePointer() {
  return GetRegister(6); //%rbp
}

temp::Temp *X64RegManager::StackPointer() {
  return GetRegister(7); //%rsp
}

temp::Temp *X64RegManager::ReturnValue() {
  return GetRegister(0); //%rax
}

Frame *FrameFactory::NewFrame(temp::Label *label, const std::list<bool> &formals) {
  return new X64Frame(label, formals);
}

tree::Exp* FrameFactory::ExternalCall(const std::string name, tree::ExpList* args) {
  return new tree::CallExp(new tree::NameExp(temp::LabelFactory::NamedLabel(name)), args);
};

tree::Stm* FrameFactory::ProcEntryExit1(Frame* frame, tree::Stm* stm) {
  int num = 0;
  tree::Stm* viewshift = new tree::ExpStm(new tree::ConstExp(0));
  for (Access *access : frame->formals_) {
    if (reg_manager->GetRegister(num));
      viewshift = new tree::SeqStm(
        viewshift, 
        new tree::MoveStm(
          access->toExp(new tree::TempExp(reg_manager->FramePointer())), 
          new tree::TempExp(reg_manager->GetRegister(num))
        )
      );
    num += 1;
  };
  return new tree::SeqStm(viewshift, stm);
};

} // namespace frame
