#ifndef TIGER_FRAME_FRAME_H_
#define TIGER_FRAME_FRAME_H_

#include <list>
#include <memory>
#include <string>

#include "tiger/frame/temp.h"
#include "tiger/translate/tree.h"

namespace frame {

class RegManager {
public:
  RegManager() : temp_map_(temp::Map::Empty()) {}

  temp::Temp *GetRegister(int regno) { return regs_[regno]; }

  /**
   * Get general-purpose registers except RSI
   * NOTE: returned temp list should be in the order of calling convention
   * @return general-purpose registers
   */
  [[nodiscard]] virtual temp::TempList *Registers() = 0;

  /**
   * Get registers which can be used to hold arguments
   * NOTE: returned temp list must be in the order of calling convention
   * @return argument registers
   */
  [[nodiscard]] virtual temp::TempList *ArgRegs() = 0;

  /**
   * Get caller-saved registers
   * NOTE: returned registers must be in the order of calling convention
   * @return caller-saved registers
   */
  [[nodiscard]] virtual temp::TempList *CallerSaves() = 0;

  /**
   * Get callee-saved registers
   * NOTE: returned registers must be in the order of calling convention
   * @return callee-saved registers
   */
  [[nodiscard]] virtual temp::TempList *CalleeSaves() = 0;

  /**
   * Get return-sink registers
   * @return return-sink registers
   */
  [[nodiscard]] virtual temp::TempList *ReturnSink() = 0;

  /**
   * Get word size
   */
  [[nodiscard]] virtual int WordSize() = 0;

  [[nodiscard]] virtual temp::Temp *FramePointer() = 0;

  [[nodiscard]] virtual temp::Temp *StackPointer() = 0;

  [[nodiscard]] virtual temp::Temp *ReturnValue() = 0;

  temp::Map *temp_map_;
protected:
  std::vector<temp::Temp *> regs_;
};

class Access {
public:
  /* Put your lab5 code here */
  
  virtual ~Access() = default;
  
  virtual tree::Exp *toExp(tree::Exp *fp = nullptr) const = 0;
  
};

class Frame {
  /* Put your lab5 code here */

public:
  temp::Label *label_name_;
  std::list<Access *> formals_;

  int offset = 0;

  Frame(temp::Label *name) : label_name_(name) {}

  virtual Access *allocLocal(bool escape) = 0;

};

/**
 * Fragments
 */

class Frag {
public:
  virtual ~Frag() = default;
};

class StringFrag : public Frag {
public:
  temp::Label *label_;
  std::string str_;

  StringFrag(temp::Label *label, std::string str)
      : label_(label), str_(std::move(str)) {}
};

class ProcFrag : public Frag {
public:
  tree::Stm *body_;
  Frame *frame_;

  ProcFrag(tree::Stm *body, Frame *frame) : body_(body), frame_(frame) {}
};

class Frags {
public:
  Frags() = default;
  void PushBack(Frag *frag) { frags_.push_back(frag); }
  const std::list<Frag*> &GetList() { return frags_; }

private:
  std::list<Frag*> frags_;
};

/* TODO: Put your lab5 code here */
class FrameFactory {
public:
  static Frame *NewFrame(temp::Label *label, const std::list<bool> &formals);
  static tree::Stm *ProcEntryExit1(Frame *f, tree::Stm *stm);
  static tree::Exp *ExternalCall(const std::string name, tree::ExpList *args);
};

} // namespace frame

#endif