#include "tiger/regalloc/regalloc.h"

#include "tiger/output/logger.h"

extern frame::RegManager *reg_manager;

namespace ra {
/* TODO: Put your lab6 code here */

void RegAllocator::RegAlloc() {
  col::Result col_result;
  auto output_instr_list = new assem::InstrList();
  while (true) {

    /* analysis liveness */

    fg::FlowGraphFactory flow_graph_fac(instr_list_);
    // build control flow graph
    flow_graph_fac.AssemFlowGraph();
    fg::FGraphPtr control_flow_graph = flow_graph_fac.GetFlowGraph();

    live::LiveGraphFactory live_graph_fac(control_flow_graph);
    // build interf/liveness graph
    live_graph_fac.Liveness();
    live::LiveGraph liveness = live_graph_fac.GetLiveGraph();
    
    /* paint */

    col::Color painter(liveness);
    painter.Paint();
    col_result = painter.result_;

    if (col_result.spills == nullptr || col_result.spills->GetList().empty()) {
      // no spill, over

      result_ = std::make_unique<ra::Result>(col_result.coloring, instr_list_);

      break;
    }
    else {
      // spill, rewrite
      RewriteProgram(col_result.spills);
    }

  }
  
}

std::unique_ptr<ra::Result> RegAllocator::TransferResult() {
  return std::move(result_);
};

void RegAllocator::RewriteProgram(const live::INodeList *spilledNodes)
{
  // check all spilled node
  for (auto node : spilledNodes->GetList()) {
    auto new_instr_list = new assem::InstrList();
    auto temp = node->NodeInfo();
    frame_->frame_offset -= reg_manager->WordSize();

    for (auto instr : instr_list_->GetList()) {

      if (instr->Use()->Contain(temp)) {

        /* need one instr to LOAD before it */

        auto new_temp = temp::TempFactory::NewTemp();

        std::string new_instr_str = "movq (" + frame_->label_name_->Name() + "_framesize-" + std::to_string(-frame_->frame_offset) + ")(`s0), `d0";
        auto new_instr = new assem::OperInstr(new_instr_str, new temp::TempList({new_temp}), new temp::TempList({reg_manager->StackPointer()}), nullptr);
        new_instr_list->Append(new_instr);

        // update the temp in the Use
        instr->Use()->replaceTemp(temp, new_temp);
      }

      new_instr_list->Append(instr);

      if (instr->Def()->Contain(temp)) {

        /* need one instr to STORE after it */

        auto new_temp = temp::TempFactory::NewTemp();

        std::string new_instr_str = "movq `s0, (" + frame_->label_name_->Name() + "_framesize-" + std::to_string(-frame_->frame_offset) + ")(`d0)";
        auto new_instr = new assem::OperInstr(new_instr_str, new temp::TempList({reg_manager->StackPointer()}), new temp::TempList({new_temp}), nullptr);
        new_instr_list->Append(new_instr);

        // update the temp in the Def
        instr->Def()->replaceTemp(temp, new_temp);
      }
    }

    delete instr_list_;
    instr_list_ = new_instr_list;
  }
}

} // namespace ra