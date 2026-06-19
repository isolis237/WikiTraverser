#include "graph_algorithms.h"

#include "title_utils.h"

#include <algorithm>
#include <queue>

namespace Graph {

NodeId FindNodeIdByTitle(const Digraph& graph, const std::string& title) {
    const std::string needle = NormalizeTitleKey(title);
    if (needle.empty()) return kInvalidNodeId;

    for (std::size_t i = 0; i < graph.num_nodes(); ++i) {
        if (NormalizeTitleKey(graph.title(static_cast<NodeId>(i))) == needle) {
            return static_cast<NodeId>(i);
        }
    }
    return kInvalidNodeId;
}

NodeId HighestOutDegreeNode(const Digraph& graph) {
    if (graph.num_nodes() == 0) return kInvalidNodeId;

    NodeId best = 0;
    std::size_t best_degree = graph.out_degree(best);
    for (std::size_t i = 1; i < graph.num_nodes(); ++i) {
        const auto node = static_cast<NodeId>(i);
        const std::size_t degree = graph.out_degree(node);
        if (degree > best_degree) {
            best = node;
            best_degree = degree;
        }
    }
    return best;
}

std::vector<NodeId> ShortestPathBfs(const Digraph& graph, NodeId start, NodeId end) {
    if (start == kInvalidNodeId || end == kInvalidNodeId) return {};
    if (start < 0 || end < 0) return {};
    if (static_cast<std::size_t>(start) >= graph.num_nodes() ||
        static_cast<std::size_t>(end) >= graph.num_nodes()) {
        return {};
    }

    std::vector<NodeId> predecessor(graph.num_nodes(), kInvalidNodeId);
    std::vector<char> visited(graph.num_nodes(), 0);
    std::queue<NodeId> frontier;

    visited[static_cast<std::size_t>(start)] = 1;
    frontier.push(start);

    while (!frontier.empty()) {
        const NodeId current = frontier.front();
        frontier.pop();

        if (current == end) break;

        for (const auto& edge : graph.out_edges(current)) {
            const auto to_index = static_cast<std::size_t>(edge.to);
            if (to_index >= graph.num_nodes() || visited[to_index]) continue;

            visited[to_index] = 1;
            predecessor[to_index] = current;
            frontier.push(edge.to);
        }
    }

    if (!visited[static_cast<std::size_t>(end)]) return {};

    std::vector<NodeId> path;
    for (NodeId at = end; at != kInvalidNodeId; at = predecessor[static_cast<std::size_t>(at)]) {
        path.push_back(at);
        if (at == start) break;
    }

    if (path.empty() || path.back() != start) return {};

    std::reverse(path.begin(), path.end());
    return path;
}

} // namespace Graph
