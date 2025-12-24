#pragma once

#include <string>
#include <ostream>
#include <utility>

namespace Graph {
    /**
     * @brief Represents a Wikipedia article (graph vertex).
     */
    struct Node {
        int id;
        std::string title;
        Node(int id_, std::string title_) 
          : id(id_), title(std::move(title_)) {}
    };

    inline bool operator==(const Node& a, const Node& b) {
        return a.id == b.id;
    }

    inline std::ostream& operator<<(std::ostream& os, const Node& n) {
        return os << "Node{id=" << n.id << ", title=" << n.title << "}";
    }
}