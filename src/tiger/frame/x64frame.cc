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
    Access* local = nullptr;
    if (escape) {
      local = new InFrameAccess(frame_offset);
      frame_offset -= reg_manager->WordSize();
    } else {
      local = new InRegAccess(temp::TempFactory::NewTemp());
    }
    return local;
  };
};

/* Put your lab5 code here */

X64RegManager::X64RegManager() : RegManager() {
    rax = temp::TempFactory::NewTemp();
    rbx = temp::TempFactory::NewTemp();
    rcx = temp::TempFactory::NewTemp();
    rdx = temp::TempFactory::NewTemp();
    rdi = temp::TempFactory::NewTemp();
    rsi = temp::TempFactory::NewTemp();
    rbp = temp::TempFactory::NewTemp();
    rsp = temp::TempFactory::NewTemp();
    r8 = temp::TempFactory::NewTemp();
    r9 = temp::TempFactory::NewTemp();
    r10 = temp::TempFactory::NewTemp();
    r11 = temp::TempFactory::NewTemp();
    r12 = temp::TempFactory::NewTemp();
    r13 = temp::TempFactory::NewTemp();
    r14 = temp::TempFactory::NewTemp();
    r15 = temp::TempFactory::NewTemp();

    temp_map_->Enter(rax, new std::string("%rax"));
    temp_map_->Enter(rbx, new std::string("%rbx"));
    temp_map_->Enter(rcx, new std::string("%rcx"));
    temp_map_->Enter(rdx, new std::string("%rdx"));
    temp_map_->Enter(rdi, new std::string("%rdi"));
    temp_map_->Enter(rsi, new std::string("%rsi"));
    temp_map_->Enter(rbp, new std::string("%rbp"));
    temp_map_->Enter(rsp, new std::string("%rsp"));
    temp_map_->Enter(r8, new std::string("%r8"));
    temp_map_->Enter(r9, new std::string("%r9"));
    temp_map_->Enter(r10, new std::string("%r10"));
    temp_map_->Enter(r11, new std::string("%r11"));
    temp_map_->Enter(r12, new std::string("%r12"));
    temp_map_->Enter(r13, new std::string("%r13"));
    temp_map_->Enter(r14, new std::string("%r14"));
    temp_map_->Enter(r15, new std::string("%r15"));

  };

temp::TempList* X64RegManager::Registers() {
  static temp::TempList* templist = nullptr;
  
  if (templist) return templist;
  
  templist = new temp::TempList();

  templist->Append(rax);
  templist->Append(rdi);
  templist->Append(rsi);
  templist->Append(rdx);
  templist->Append(rcx);
  templist->Append(r8);
  templist->Append(r9);
  templist->Append(r10);
  templist->Append(r11);
  templist->Append(rbx);
  templist->Append(rbp);
  templist->Append(r12);
  templist->Append(r13);
  templist->Append(r14);
  templist->Append(r15);
  return templist;
};

temp::TempList* X64RegManager::ArgRegs() {
  static temp::TempList* templist = nullptr;

  if (templist) return templist;

  templist = new temp::TempList();
  templist->Append(rdi);
  templist->Append(rsi);
  templist->Append(rdx);
  templist->Append(rcx);
  templist->Append(r8);
  templist->Append(r9);
  return templist;
};

temp::TempList* X64RegManager::CallerSaves() {
  static temp::TempList* templist = nullptr;

  if (templist) return templist;

  templist = new temp::TempList();
  templist->Append(rax);
  templist->Append(rdi);
  templist->Append(rsi);
  templist->Append(rdx);
  templist->Append(rcx);
  templist->Append(r8);
  templist->Append(r9);
  templist->Append(r10);
  templist->Append(r11);
  return templist;
};
temp::TempList* X64RegManager::CalleeSaves() {
  static temp::TempList* templist = nullptr;

  if (templist) return templist;

  templist = new temp::TempList();
  templist->Append(rbx);
  templist->Append(rbp);
  templist->Append(r12);
  templist->Append(r13);
  templist->Append(r14);
  templist->Append(r15);
  return templist; 
};

temp::TempList* X64RegManager::ReturnSink() {
  assert(0);
  return nullptr;
};

int X64RegManager::WordSize() {
  return 8;
};

temp::Temp* X64RegManager::FramePointer() {
  return rbp;
};

temp::Temp* X64RegManager::StackPointer() {
  return rsp;
};

temp::Temp* X64RegManager::ReturnValue() {
  return rax;
};

Frame *NewFrame(temp::Label *label, const std::list<bool> &formals) {
  return new X64Frame(label, formals);
}

tree::Exp* ExternalCall(const std::string name, tree::ExpList* args) {
  return new tree::CallExp(new tree::NameExp(temp::LabelFactory::NamedLabel(name)), args);
};

tree::Stm* ProcEntryExit1(Frame* frame, tree::Stm* stm) {
  int num = 1;
  
  tree::Stm* viewshift = new tree::ExpStm(new tree::ConstExp(0));
  
  auto formal_list = frame->formals_;
  
  for (auto formal : formal_list) {
    if (reg_manager->GetNthArgReg(num))
      viewshift = new tree::SeqStm(viewshift, new tree::MoveStm(formal->toExp(new tree::TempExp(reg_manager->FramePointer())), new tree::TempExp(reg_manager->GetNthArgReg(num))));
    else {
      frame->frame_offset -= reg_manager->WordSize();
      viewshift = new tree::SeqStm(new tree::MoveStm(formal->toExp(new tree::TempExp(reg_manager->FramePointer())), new tree::MemExp(new tree::BinopExp(tree::BinOp::PLUS_OP, new tree::TempExp(reg_manager->RBP()), new tree::ConstExp((6 - num) * reg_manager->WordSize())))), viewshift);
    }
    num++;
  };
  return new tree::SeqStm(viewshift, stm);
};

assem::InstrList* ProcEntryExit2(assem::InstrList* body) {
  static temp::TempList* retlist = nullptr;
  if (!retlist)
    retlist = new temp::TempList(reg_manager->ReturnValue());
  assem::OperInstr* ele = new assem::OperInstr("", nullptr, retlist, new assem::Targets(nullptr));
  body->Append(ele);
  return body;
};

assem::Proc* ProcEntryExit3(frame::Frame* frame, assem::InstrList* body) {
  static char instr[256];

  std::string prolog;
  sprintf(instr, ".set %s_framesize, %d\n", frame->label_name_->Name().c_str(), -frame->frame_offset);
  prolog = std::string(instr);
  sprintf(instr, "%s:\n", frame->label_name_->Name(). c_str());
  prolog.append(std::string(instr));
  sprintf(instr, "subq $%d, %%rsp\n", -frame->frame_offset);
  prolog.append(std::string(instr));

  sprintf(instr, "addq $%d, %%rsp\n", -frame->frame_offset);
  std::string epilog = std::string(instr);
  epilog.append(std::string("retq\n"));
  return new assem::Proc(prolog, body, epilog);
};

} // namespace frame
