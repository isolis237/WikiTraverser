#pragma once

#include "digraph.h"

#include <string>
#include <vector>

namespace Graph {

constexpr NodeId kInvalidNodeId = -1;

NodeId FindNodeIdByTitle(const Digraph& graph, const std::string& title);
NodeId HighestOutDegreeNode(const Digraph& graph);
std::vector<NodeId> ShortestPathBfs(const Digraph& graph, NodeId start, NodeId end);

} // namespace Graph
