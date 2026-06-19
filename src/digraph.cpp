#include "digraph.h"

#include <stdexcept>

namespace Graph {
namespace {

void check_node(NodeId node, std::size_t n, const char* what) {
    if (node < 0 || static_cast<std::size_t>(node) >= n) {
        throw std::out_of_range(what);
    }
}

} // namespace

const std::vector<OutEdge>& Digraph::out_edges(NodeId node) const {
    check_node(node, adjacency_list_.size(), "Invalid node id in out_edges");
    return adjacency_list_[node];
}

std::size_t Digraph::out_degree(NodeId node) const {
    return out_edges(node).size();
}

const std::string& Digraph::title(NodeId node) const {
    check_node(node, titles_.size(), "Invalid node id in title()");
    return titles_[node];
}

} // namespace Graph
