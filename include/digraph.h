#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace Graph {

using NodeId = int;

struct OutEdge {
    NodeId to;
    int weight = 1;
};

class DigraphBuilder;

/**
 * @brief A directed graph represented using an adjacency list.
 *
 * Notes:
 * - Node IDs are dense: 0..num_nodes-1 (remapped from file)
 * - Graph is immutable after construction (built via DigraphBuilder)
 */
class Digraph {
public:
    Digraph() = default;

    std::size_t num_nodes() const { return adjacency_list_.size(); }
    std::size_t num_edges() const { return edge_count_; }

    const std::vector<OutEdge>& out_edges(NodeId node) const;
    std::size_t out_degree(NodeId node) const;

    const std::string& title(NodeId node) const;
    const std::vector<std::string>& titles() const { return titles_; }

private:
    friend class DigraphBuilder;

    std::vector<std::vector<OutEdge>> adjacency_list_;
    std::vector<std::string> titles_;
    std::size_t edge_count_ = 0;
};

} // namespace Graph
