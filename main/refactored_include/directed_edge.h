#pragma once

#include "node.h"

namespace Graph {
    /**
     * @brief Represents a path between two wikipedia articles (graph edge).
     * @param from - id of the starting node.
     * @param to - id of the ending node.
     * @param weight (Optional) - weight used for algos such as dijkstras. default 1.
     */

     using NodeId = int;

    struct Edge {
        NodeId from;
        NodeId to;
        int weight = 1;

        Edge(NodeId f, NodeId t, int w = 1) : from(f), to(t), weight(w) {}
        Edge(const Node& start, const Node& end, int w = 1)
            : from(start.id), to(end.id), weight(w) {}
    };

    inline std::ostream& operator<<(std::ostream& os, const Edge& e) {
        return os << "Edge{from=" << e.from
                    << ", to=" << e.to
                    << ", weight=" << e.weight
                    << "}";
    }

}