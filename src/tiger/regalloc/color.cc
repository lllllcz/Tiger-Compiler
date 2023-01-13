#include "tiger/regalloc/color.h"

extern frame::RegManager *reg_manager;

namespace col {
/* TODO: Put your lab6 code here */

Color::Color(live::LiveGraph liveness)
 : live_graph_(liveness)
{
  for (auto t : reg_manager->Registers()->GetList()) {
    auto name = reg_manager->temp_map_->Look(t);
    precolored.insert(std::make_pair(t, *name));
  }
  result_.coloring = temp::Map::Empty();
  result_.spills = new live::INodeList();
}

inline bool Color::precolor(live::INodePtr n) const {
  return precolored.find(n->NodeInfo()) != precolored.end();
}

void Color::Paint() {
  for (auto node : live_graph_.interf_graph->Nodes()->GetList()) {
    adjList[node] = new live::INodeList();
    degree[node] = 0;
  };
  Build();
  MakeWorklist();

  do {
    if (!simplifyWorklist.GetList().empty()) {
      Simplify();
    } else if (!worklistMoves.GetList().empty()) {
      Coalesce();
    } else if (!freezeWorklist.GetList().empty()) {
      Freeze();
    } else if (!spillWorklist.GetList().empty()) {
      SelectSpill();
    }
  } while (
    !simplifyWorklist.GetList().empty() || !worklistMoves.GetList().empty() ||
    !freezeWorklist.GetList().empty() || !spillWorklist.GetList().empty()
  );

  AssignColors();

}

void Color::Build() {
  for (auto move : live_graph_.moves->GetList()) {
    auto src = move.first;
    auto dest = move.second;
    moveList[src].Append(src, dest);
    moveList[dest].Append(src, dest);
  }

  worklistMoves = *live_graph_.moves;

  const auto &interf_node_list = live_graph_.interf_graph->Nodes()->GetList();
  for (auto node : interf_node_list)
    degree[node] = 0;

  for (auto node : interf_node_list) {
    for (auto node_tmp : node->Adj()->GetList()) {
      if (node != node_tmp) AddEdge(node, node_tmp);
    }
  }

  for (auto &pair : precolored) {
    result_.coloring->Enter(pair.first, new std::string(pair.second));
  }

}

void Color::AddEdge(live::INodePtr u, live::INodePtr v) {
  if (adjSet.find(std::make_pair(u, v)) != adjSet.end()) return;

  adjSet.insert(std::make_pair(u, v));
  adjSet.insert(std::make_pair(v, u));

  if (!precolor(u)) {
    if (!adjList[u]->Contain(v)) adjList[u]->Append(v);
    // adjList[u].Append(v);
    degree[u]++;
  }
  if (!precolor(v)) {
    if (!adjList[v]->Contain(u)) adjList[v]->Append(u);
    // adjList[v].Append(u);
    degree[v]++;
  }
  
}

void Color::MakeWorklist() {
  for (auto node : live_graph_.interf_graph->Nodes()->GetList()) {
    if (precolor(node)) continue;

    if (degree[node] >= REG_NUM_K)
      spillWorklist.Append(node); // high degree
    else if (MoveRelated(node))
      freezeWorklist.Append(node); // low degree, move related
    else
      simplifyWorklist.Append(node); // low degree, not move related
  }
}

/* Find all neighbors */
live::INodeList Color::Adjacent(live::INodePtr node)  {
  auto it = adjList.find(node);
  if (it == adjList.end()) {
    adjList[node] = new live::INodeList();
    return live::INodeList();
  }
  auto list = selectStack.Union(&coalescedNodes);
  return *it->second->Diff(list);
}

live::MoveList Color::NodeMoves(live::INodePtr node) {
  auto it = moveList.find(node);
  if (it == moveList.end()) {
    return live::MoveList();
  }
  live::MoveList list = *activeMoves.Union(&worklistMoves);
  return *(it->second).Intersect(&list);
}

bool Color::MoveRelated(live::INodePtr node)  {
  return !NodeMoves(node).GetList().empty();
}

void Color::Simplify() {
  auto node = simplifyWorklist.GetList().front(); // simplify the first
  simplifyWorklist.DeleteNode(node);

  selectStack.Append(node);

  // dec degree
  auto adj_list = Adjacent(node).GetList();
  for (auto node : adj_list) {
    DecrementDegree(node);
  }
}

void Color::DecrementDegree(live::INodePtr node) {
  if (precolor(node)) return;

  int d = degree[node];
  degree[node] = d - 1;

  // because its degree decrease
  // if this node is move related, it may be able to coalesce
  if (d == REG_NUM_K) {
    live::INodeListPtr n_list = new live::INodeList({node});
    live::INodeListPtr a_list = Adjacent(node).Union(n_list);
    EnableMoves(*a_list);
    spillWorklist.DeleteNode(node);
    if (MoveRelated(node)) {
      freezeWorklist.Append(node);
    } else {
      simplifyWorklist.Append(node);
    }
  }
}

void Color::EnableMoves(const live::INodeList &nodes) {
  for (auto node : nodes.GetList()) {
    auto node_moves = NodeMoves(node);
    for (auto move_pair : node_moves.GetList()) {
      if (!activeMoves.Contain(move_pair.first, move_pair.second)) continue;
      // transfer move
      activeMoves.Delete(move_pair.first, move_pair.second);
      worklistMoves.Append(move_pair.first, move_pair.second);
    }
  }
}

void Color::Coalesce() {
  if (worklistMoves.GetList().empty())
    return;

  auto move = worklistMoves.GetList().front();
  auto src = move.first;
  auto dst = move.second;

  live::INodePtr u, v;
  u = GetAlias(dst);
  v = GetAlias(src);
  if (!precolor(dst)) {
    live::INodePtr tmp;
    tmp = u;
    u = v;
    v = tmp;
  }
  worklistMoves.Delete(src, dst);

  if (u == v) {
    coalescedMoves.Append(src, dst);
    AddWorkList(u);
  }
  else if (precolor(v) || adjSet.find(std::make_pair(u, v)) != adjSet.end()) {
    constrainedMoves.Append(src, dst);
    AddWorkList(u);
    AddWorkList(v);
  }
  else {
    bool flag = true;
    live::INodeList list = Adjacent(v);
    const auto &node_list = list.GetList();
    for (auto node : node_list) {
      if (!OK(node, u)) {
        flag = false;
        break;
      }
    }
    
    live::INodeList avlist = Adjacent(v);
    live::INodeList aulist = Adjacent(u);
    live::INodeList auvl = *aulist.Union(&avlist);

    if ((precolor(u) && flag ) || (!precolor(u) && Conservative(auvl))) {
      /* George & Briggs */
      coalescedMoves.Append(src, dst);
      Combine(u, v);
      AddWorkList(u);
    } else {
      activeMoves.Append(src, dst);
    }

  }

}

void Color::AddWorkList(live::INodePtr node) {
  // low degree, not move related
  if (!precolor(node) && !MoveRelated(node) && degree[node] < REG_NUM_K) {
    freezeWorklist.DeleteNode(node);
    simplifyWorklist.Append(node);
  }
}

bool Color::OK(live::INodePtr t, live::INodePtr r) {
  int d = degree[t];
  bool c = (adjSet.find(std::make_pair(t, r)) != adjSet.end());
  return precolor(t) || (d < REG_NUM_K) || c;
}

bool Color::Conservative(const live::INodeList &nodes) {
  int k = 0;
  for (auto node : nodes.GetList()) {
    if (precolor(node) || degree[node] >= REG_NUM_K) {
      k++;
    }
  }
  return k < REG_NUM_K;
}

live::INodePtr Color::GetAlias(live::INodePtr node)  {
  live::INodePtr ret = node;
  if (coalescedNodes.Contain(node)) {
    ret = GetAlias(alias[node]);
  }
  return ret;
}

void Color::Combine(live::INodePtr u, live::INodePtr v) {
  if (freezeWorklist.Contain(v)) {
    freezeWorklist.DeleteNode(v);
  } else {
    spillWorklist.DeleteNode(v);
  }
  coalescedNodes.Append(v);

  alias[v] = u;

  // combine
  moveList[u] = *moveList[u].Union(&moveList[v]);

  live::INodeList v_list;
  v_list.Append(v);
  EnableMoves(v_list);

  auto list = Adjacent(v);
  for (auto adj_node : list.GetList()) {
    if (adj_node != u) AddEdge(adj_node, u);
    DecrementDegree(adj_node);
  }

  if (degree[u] >= REG_NUM_K && freezeWorklist.Contain(u)) {
    freezeWorklist.DeleteNode(u);
    spillWorklist.Append(u);
  }
}

void Color::Freeze() {
  if (freezeWorklist.GetList().empty()) {
    return;
  };
  auto node = freezeWorklist.GetList().front();
  freezeWorklist.DeleteNode(node);
  simplifyWorklist.Append(node);
  FreezeMoves(node);
}

void Color::FreezeMoves(live::INodePtr u) {
  auto node_moves = NodeMoves(u);
  for (auto move : node_moves.GetList()) {
    auto src = move.first;
    auto dst = move.second;

    live::INodePtr v;
    if (GetAlias(dst) == GetAlias(u)) {
      v = GetAlias(src);
    } else {
      v = GetAlias(dst);
    }
    activeMoves.Delete(src, dst);
    frozenMoves.Append(src, dst);

    if (NodeMoves(v).GetList().empty() && degree[v] < REG_NUM_K && !precolor(v)) {
      freezeWorklist.DeleteNode(v);
      simplifyWorklist.Append(v);
    }
  }
}

void Color::SelectSpill() {
  live::INodePtr m = nullptr;

  double chosen_priority = 1e20;
  for (auto tmp : spillWorklist.GetList()) {
    if (!spillNodes.Contain(tmp) && !precolor(tmp)) {
      double cal = live_graph_.priority[tmp->NodeInfo()] / double (degree[tmp]);
      if (cal < chosen_priority) {
        m = tmp;
        chosen_priority = cal;
      }
    }
  };
  
  // not found, select the first
  if (!m) {
    m = spillWorklist.GetList().front();
  }

  spillWorklist.DeleteNode(m);
  simplifyWorklist.Append(m);
  FreezeMoves(m);
}

void Color::AssignColors() {
  while (!selectStack.GetList().empty()) {
    auto node = selectStack.GetList().back();
    selectStack.DeleteNode(node);

    // available colors
    std::set<std::string> colors; // %rax...
    for (auto &it : precolored) {
      colors.insert(it.second);
    }

    // remove the color of interf node
    for (auto node : adjList[node]->GetList()) {
      auto alias_node = GetAlias(node);
      if (coloredNodes.Contain(alias_node) || precolor(alias_node)) {
        std::string str = *(result_.coloring->Look(alias_node->NodeInfo()));
        auto iter = colors.find(str);
        if (iter != colors.end())
          colors.erase(iter);
      }
    }
    if (colors.empty()) {
      // no available color, spill it
      result_.spills->Append(node);
    } else {
      // assign color
      coloredNodes.Append(node);
      result_.coloring->Enter(node->NodeInfo(), new std::string(*colors.begin()));
    }
  }

  for (auto node : coalescedNodes.GetList()) {
    std::string col_str = *(result_.coloring->Look(GetAlias(node)->NodeInfo()));
    result_.coloring->Enter(node->NodeInfo(), new std::string(col_str));
  }
}

} // namespace col
