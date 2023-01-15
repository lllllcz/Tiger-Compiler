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

  /* init */
  for (auto node : cfg_nodes) {
    in_->Enter(node, new temp::TempList());
    out_->Enter(node, new temp::TempList());
  }

  while (true) {
    bool isChanged = false;

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
    if (!isChanged) break;
  }

}

void LiveGraphFactory::InterfGraph() {
  /* TODO: Put your lab6 code here */

  live_graph_.interf_graph = new IGraph();
  live_graph_.moves = new MoveList();

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
  for (auto iter1 : registers) {
    for (auto iter2 : registers) {
      live_graph_.interf_graph->AddEdge(temp_node_map_->Look(iter1), temp_node_map_->Look(iter2));
      live_graph_.interf_graph->AddEdge(temp_node_map_->Look(iter2), temp_node_map_->Look(iter1));
    }
  }

  for (auto node : cfg_nodes) {
    auto instr = node->NodeInfo();

    if (typeid(*instr) != typeid(assem::MoveInstr)) {
      /* not move */
      for (auto def_reg : instr->Def()->GetList()) {
        for (auto out_reg : out_->Look(node)->GetList()) {
          if (out_reg == reg_manager->StackPointer()) continue;

          live_graph_.interf_graph->AddEdge(temp_node_map_->Look(def_reg), temp_node_map_->Look(out_reg));
          live_graph_.interf_graph->AddEdge(temp_node_map_->Look(out_reg), temp_node_map_->Look(def_reg));
        }

      }
    }
    else {
      /* move */
      for (auto def_reg : instr->Def()->GetList()) {
        auto use_regs = instr->Use();
        
        for (auto out_reg : out_->Look(node)->GetList()) {
          if (use_regs->Contain(out_reg))
            continue;
          live_graph_.interf_graph->AddEdge(temp_node_map_->Look(def_reg), temp_node_map_->Look(out_reg));
          live_graph_.interf_graph->AddEdge(temp_node_map_->Look(out_reg), temp_node_map_->Look(def_reg));
        }

        /* add move related regs */
        for (auto use_reg : instr->Use()->GetList()) {
          if (!live_graph_.moves->Contain(temp_node_map_->Look(use_reg), temp_node_map_->Look(def_reg)))
            live_graph_.moves->Append(temp_node_map_->Look(use_reg), temp_node_map_->Look(def_reg));
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
