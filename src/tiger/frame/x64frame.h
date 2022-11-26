//
// Created by wzl on 2021/10/12.
//

#ifndef TIGER_COMPILER_X64FRAME_H
#define TIGER_COMPILER_X64FRAME_H

#include "tiger/frame/frame.h"

namespace frame {
class X64RegManager : public RegManager {
  /* Put your lab5 code here */
private:
  temp::Temp *rax, *rbx, *rcx, *rdx, *rdi, *rsi, *rbp, *rsp,
            *r8, *r9, *r10, *r11, *r12, *r13, *r14, *r15;
public:
  X64RegManager();

  /**
   * Get general-purpose registers except RSI
   * NOTE: returned temp list should be in the order of calling convention
   * @return general-purpose registers
   */
  [[nodiscard]] temp::TempList *Registers() override;

  /**
   * Get registers which can be used to hold arguments
   * NOTE: returned temp list must be in the order of calling convention
   * @return argument registers
   */
  [[nodiscard]] temp::TempList *ArgRegs() override;

  /**
   * Get caller-saved registers
   * NOTE: returned registers must be in the order of calling convention
   * @return caller-saved registers
   */
  [[nodiscard]] temp::TempList *CallerSaves() override;

  /**
   * Get callee-saved registers
   * NOTE: returned registers must be in the order of calling convention
   * @return callee-saved registers
   */
  [[nodiscard]] temp::TempList *CalleeSaves() override;

  /**
   * Get return-sink registers
   * @return return-sink registers
   */
  [[nodiscard]] temp::TempList *ReturnSink() override;

  /**
   * Get word size
   */
  [[nodiscard]] int WordSize() override;

  [[nodiscard]] temp::Temp *FramePointer() override;

  [[nodiscard]] temp::Temp *StackPointer() override;

  [[nodiscard]] temp::Temp *ReturnValue() override;

  temp::Temp* GetNthArgReg(int i) {
    switch (i)
    {
    case 1: return rdi;
    case 2: return rsi;
    case 3: return rdx;
    case 4: return rcx;
    case 5: return r8;
    case 6: return r9;
    };
    return nullptr;
  };

  temp::Temp* RAX() { return rax; };
  temp::Temp* RDI() { return rdi; };
  temp::Temp* RSI() { return rsi; };
  temp::Temp* RDX() { return rdx; };
  temp::Temp* RCX() { return rcx; };
  temp::Temp* R8() { return r8; };
  temp::Temp* R9() { return r9; };
  temp::Temp* R10() { return r10; };
  temp::Temp* R11() { return r11; };
  temp::Temp* RBX() { return rbx; };
  temp::Temp* RBP() { return rbp; };
  temp::Temp* R12() { return r12; };
  temp::Temp* R13() { return r13; };
  temp::Temp* R14() { return r14; };
  temp::Temp* R15() { return r15; };
  temp::Temp* RSP() { return rsp; };

};

} // namespace frame
#endif // TIGER_COMPILER_X64FRAME_H
