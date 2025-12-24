#include "../refactored_include/digraph.h"

#include <stdexcept>

namespace Graph {

static inline void check_node_(NodeId node, std::size_t n, const char* what) {
    if (node < 0 || static_cast<std::size_t>(node) >= n) {
        throw std::out_of_range(what);
    }
}

const std::vector<OutEdge>& Digraph::out_edges(NodeId node) const {
    check_node_(node, adjacency_list_.size(), "Invalid node id in out_edges");
    return adjacency_list_[node];
}

std::size_t Digraph::out_degree(NodeId node) const {
    return out_edges(node).size();
}

const std::string& Digraph::title(NodeId node) const {
    check_node_(node, titles_.size(), "Invalid node id in title()");
    return titles_[node];
}

} // namespace Graph
