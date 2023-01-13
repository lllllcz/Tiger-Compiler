#ifndef TIGER_REGALLOC_REGALLOC_H_
#define TIGER_REGALLOC_REGALLOC_H_

#include "tiger/codegen/assem.h"
#include "tiger/codegen/codegen.h"
#include "tiger/frame/frame.h"
#include "tiger/frame/temp.h"
#include "tiger/liveness/liveness.h"
#include "tiger/regalloc/color.h"
#include "tiger/util/graph.h"

namespace ra {

class Result {
public:
  temp::Map *coloring_;
  assem::InstrList *il_;

  Result() : coloring_(nullptr), il_(nullptr) {}
  Result(temp::Map *coloring, assem::InstrList *il)
      : coloring_(coloring), il_(il) {}
  Result(const Result &result) = delete;
  Result(Result &&result) = delete;
  Result &operator=(const Result &result) = delete;
  Result &operator=(Result &&result) = delete;
  ~Result() = default;
};

class RegAllocator {
  /* TODO: Put your lab6 code here */
public:
  frame::Frame *frame_;
  assem::InstrList *instr_list_;
  std::unique_ptr<ra::Result> result_;

public:
  RegAllocator(frame::Frame *frame, std::unique_ptr<cg::AssemInstr> traces)
   : frame_(frame), instr_list_(traces->GetInstrList()), result_(std::make_unique<ra::Result>()) {}
  
  void RegAlloc();

  std::unique_ptr<ra::Result> TransferResult();

  void RewriteProgram(const live::INodeList *spilledNodes);

};

} // namespace ra

#endif