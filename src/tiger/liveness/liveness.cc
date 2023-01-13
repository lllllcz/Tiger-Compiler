#include "tiger/liveness/liveness.h"

extern frame::RegManager *reg_manager;

namespace live {

bool MoveList::Contain(INodePtr src, INodePtr dst) {
  return std::any_of(move_list_.cbegin(), move_list_.cend(),
                     [src, dst](std::pair<INodePtr, INodePtr> move) {
                       return move.first == src && move.second == dst;
                     });
}

void MoveList::Delete(INodePtr src, INodePtr dst) {
  assert(src && dst);
  auto move_it = move_list_.begin();
  for (; move_it != move_list_.end(); move_it++) {
    if (move_it->first == src && move_it->second == dst) {
      break;
    }
  }
  move_list_.erase(move_it);
}

MoveList *MoveList::Union(MoveList *list) {
  auto *res = new MoveList();
  for (auto move : move_list_) {
    res->move_list_.push_back(move);
  }
  for (auto move : list->GetList()) {
    if (!res->Contain(move.first, move.second))
      res->move_list_.push_back(move);
  }
  return res;
}

MoveList *MoveList::Intersect(MoveList *list) {
  auto *res = new MoveList();
  for (auto move : list->GetList()) {
    if (Contain(move.first, move.second))
      res->move_list_.push_back(move);
  }
  return res;
}

} // namespace live

namespace live {

void LiveGraphFactory::LiveMap() {
  /* TODO: Put your lab6 code here */

  const auto &cfg_nodes = flowgraph_->Nodes()->GetList();
  for (auto node : cfg_nodes) {
    in_->Enter(node, new temp::TempList());
    out_->Enter(node, new temp::TempList());
  }

  bool isChanged = true;
  while (isChanged) {
    isChanged = false;

    for (auto node : cfg_nodes) {

      auto init_in_nodes = in_->Look(node);
      auto init_out_nodes = out_->Look(node);

      if (out_->Look(node) != nullptr) {
        auto use_nodes = node->NodeInfo()->Use();
        auto def_nodes = node->NodeInfo()->Def();
        // update in[] of node : use[] + out[] - def[]
        in_->Set(node, use_nodes->Union(out_->Look(node)->Diff(def_nodes)));
      }

      for (auto succ : node->Succ()->GetList()) {
        auto in_nodes = in_->Look(succ); // in[] of succ
        auto out_nodes = out_->Look(node); // out[] of node
        // update out[] of node
        out_->Set(node, out_nodes->Union(in_nodes));
      }
    
      if (isChanged) continue;

      if (!in_->Look(node)->Equal(init_in_nodes) || !out_->Look(node)->Equal(init_out_nodes)) {
        isChanged = true;
      }

    }
  }

}

void LiveGraphFactory::InterfGraph() {
  /* TODO: Put your lab6 code here */

  live_graph_.interf_graph = new IGraph();
  live_graph_.moves = new MoveList();

  const auto rsp = reg_manager->StackPointer();
  const auto &registers = reg_manager->Registers()->GetList();

  /* add machine register nodes */
  for (auto reg : registers) {
    temp_node_map_->Enter(reg, live_graph_.interf_graph->NewNode(reg));
  }

  /* add nodes in use[] and def[] */
  const auto &cfg_nodes = flowgraph_->Nodes()->GetList();
  for (auto node : cfg_nodes) {
    auto node_info = node->NodeInfo();
    auto union_list = node_info->Use()->Union(node_info->Def());
    for (auto temp : union_list->GetList()) {
      if (!temp_node_map_->Look(temp))
        temp_node_map_->Enter(temp, live_graph_.interf_graph->NewNode(temp));
    }
  }

  /* add edge for machine registers */
  for (auto iter1 = registers.begin(); iter1 != registers.end(); ++iter1) {
    for (auto iter2 = std::next(iter1); iter2 != registers.end(); ++iter2) {
      auto node1 = temp_node_map_->Look(*iter1);
      auto node2 = temp_node_map_->Look(*iter2);

      live_graph_.interf_graph->AddEdge(node1, node2);
      live_graph_.interf_graph->AddEdge(node2, node1);
    }
  }

  for (auto node : cfg_nodes) {
    auto instr = node->NodeInfo();

    if (typeid(*instr) != typeid(assem::MoveInstr)) {
      /* not move */
      for (auto def_reg : instr->Def()->GetList()) {
        if (def_reg == rsp) continue;

        auto left_value_node = temp_node_map_->Look(def_reg);

        for (auto out_reg : out_->Look(node)->GetList()) {
          if (out_reg == rsp) continue;

          auto out_active_node = temp_node_map_->Look(out_reg);

          live_graph_.interf_graph->AddEdge(left_value_node, out_active_node);
          live_graph_.interf_graph->AddEdge(out_active_node, left_value_node);
        }

      }
    }
    else {
      /* move */
      for (auto def_reg : instr->Def()->GetList()) {
        auto left_value_node = temp_node_map_->Look(def_reg);
        auto use_regs = instr->Use();
        
        for (auto out_reg : out_->Look(node)->GetList()) {
          if (use_regs->Contain(out_reg))
            continue;
          auto out_active_node = temp_node_map_->Look(out_reg);
          live_graph_.interf_graph->AddEdge(left_value_node, out_active_node);
          live_graph_.interf_graph->AddEdge(out_active_node, left_value_node);
        }

        for (auto use_reg : instr->Use()->GetList()) {
          auto use_node = temp_node_map_->Look(use_reg);
          if (!live_graph_.moves->Contain(use_node, left_value_node))
            live_graph_.moves->Append(use_node, left_value_node);
        }

      }
    }
    
  }

  for (auto node : cfg_nodes) {
    auto node_info = node->NodeInfo();
    auto union_list = node_info->Def()->Union(node_info->Use());
    for (auto def_or_use : union_list->GetList()) {
      if (live_graph_.priority.find(def_or_use) == live_graph_.priority.end())
        live_graph_.priority[def_or_use] = 0;
      live_graph_.priority[def_or_use] += 1;
    };
  };

}

void LiveGraphFactory::Liveness() {
  LiveMap();
  InterfGraph();
}

} // namespace live
