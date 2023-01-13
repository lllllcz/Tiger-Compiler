#include "tiger/liveness/flowgraph.h"

namespace fg {

void FlowGraphFactory::AssemFlowGraph() {
  /* TODO: Put your lab6 code here */

  FNodePtr prev = nullptr;
  bool flag = true; // assign true if prev is jmp
  for (auto instr : instr_list_->GetList()) {
    FNodePtr node = flowgraph_->NewNode(instr);

    if (flag && prev != nullptr) {
      flowgraph_->AddEdge(prev, node);
    }

    if (typeid(*instr) == typeid(assem::LabelInstr)) {
      label_map_->Enter(((assem::LabelInstr *)instr)->label_, node);
    }

    prev = node;

    assem::Instr *prev_info = prev->NodeInfo();
    flag = !(
      typeid(*prev_info) == typeid(assem::OperInstr)
      && ((assem::OperInstr *)instr)->assem_.find("jmp") == 0
    );
  }

  for (auto node : flowgraph_->Nodes()->GetList()) {
    assem::Instr *instr = node->NodeInfo();
    if (typeid(*instr) != typeid(assem::OperInstr)) continue;

    auto instr_jumps = ((assem::OperInstr *)instr)->jumps_;
    if (instr_jumps == nullptr || instr_jumps->labels_ == nullptr) continue;

    const auto &labels = *instr_jumps->labels_;
    for (temp::Label *label : labels) {
      FNodePtr target_node = label_map_->Look(label);
      if (target_node) 
        flowgraph_->AddEdge(node, target_node);
    }

  }

}

} // namespace fg

namespace assem {

temp::TempList *LabelInstr::Def() const {
  /* TODO: Put your lab6 code here */
  return new temp::TempList();
}

temp::TempList *MoveInstr::Def() const {
  /* TODO: Put your lab6 code here */
  return dst_ ? dst_ : new temp::TempList();
}

temp::TempList *OperInstr::Def() const {
  /* TODO: Put your lab6 code here */
  return dst_ ? dst_ : new temp::TempList();
}

temp::TempList *LabelInstr::Use() const {
  /* TODO: Put your lab6 code here */
  return new temp::TempList();
}

temp::TempList *MoveInstr::Use() const {
  /* TODO: Put your lab6 code here */
  return src_ ? src_ : new temp::TempList();
}

temp::TempList *OperInstr::Use() const {
  /* TODO: Put your lab6 code here */
  return src_ ? src_ : new temp::TempList();
}
} // namespace assem
