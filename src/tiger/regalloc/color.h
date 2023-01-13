#ifndef TIGER_COMPILER_COLOR_H
#define TIGER_COMPILER_COLOR_H

#include "tiger/codegen/assem.h"
#include "tiger/frame/temp.h"
#include "tiger/liveness/liveness.h"
#include "tiger/util/graph.h"

#include <map>
#include <set>
#include <utility>

namespace col {
struct Result {
  Result() : coloring(nullptr), spills(nullptr) {}
  Result(temp::Map *coloring, live::INodeListPtr spills)
      : coloring(coloring), spills(spills) {}
  temp::Map *coloring;
  live::INodeListPtr spills;
};

class Color {
  /* TODO: Put your lab6 code here */

public:
  Color(live::LiveGraph liveness);
  ~Color() = default;

  void Paint();

  col::Result result_;

private:
  const int REG_NUM_K = 15;

  live::LiveGraph live_graph_;

  inline bool precolor(live::INodePtr n) const;

private:
  std::map<temp::Temp *, std::string> precolored;
  live::INodeList simplifyWorklist;
  live::INodeList freezeWorklist;
  live::INodeList spillWorklist;
  live::INodeList spillNodes;
  live::INodeList coalescedNodes;
  live::INodeList coloredNodes;
  live::INodeList selectStack;
  
  live::MoveList coalescedMoves;
  live::MoveList constrainedMoves;
  live::MoveList frozenMoves;
  live::MoveList worklistMoves;
  live::MoveList activeMoves;

  std::set<std::pair<live::INodePtr , live::INodePtr >> adjSet;
  std::map<live::INodePtr, live::INodeListPtr> adjList;
  std::map<live::INodePtr, int> degree;
  std::map<live::INodePtr, live::MoveList> moveList;
  std::map<live::INodePtr, live::INodePtr> alias;

private:
  void Build();
  
  void AddEdge(live::INodePtr u, live::INodePtr v);
  
  void MakeWorklist();
  
  live::INodeList Adjacent(live::INodePtr node) ;
  
  live::MoveList NodeMoves(live::INodePtr node) ;
  
  bool MoveRelated(live::INodePtr node) ;

  void Simplify();
  
  void DecrementDegree(live::INodePtr node);
  
  void EnableMoves(const live::INodeList &nodes);

  void Coalesce();

  void AddWorkList(live::INodePtr node);

  bool OK(live::INodePtr t, live::INodePtr r);

  bool Conservative(const live::INodeList &nodes);

  live::INodePtr GetAlias(live::INodePtr node) ;

  void Combine(live::INodePtr u, live::INodePtr v);

  void Freeze();

  void FreezeMoves(live::INodePtr u);

  void SelectSpill();

  void AssignColors();

};
} // namespace col

#endif // TIGER_COMPILER_COLOR_H
