#include <algorithm>
#include <iostream>

#include "../include/adjacency_list.h"

namespace Parsing {
    Parsing::AdjacencyList::AdjacencyList() {vertex_ = nullptr;}

    Parsing::AdjacencyList::AdjacencyList(Parsing::Node * vertex): vertex_(vertex) {}

    Parsing::AdjacencyList::~AdjacencyList() {
        // Delete edges where vertex_ is the parent
        for (Parsing::DirectedEdge * edge : incident_edges_) {
            // Only remove edges we are the parent of
            if (edge->is_parent(vertex_)) {
                delete edge;
                edge = nullptr;
            }
        }

        delete vertex_;
    }

    bool Parsing::AdjacencyList::operator==(const Parsing::AdjacencyList & other) const {
        if (*vertex_ != *other.vertex_) {
            return false;
        }

        if (incident_edges_.size() == other.incident_edges_.size()) {
            for (size_t i = 0; i < incident_edges_.size(); i++) {
                if (*incident_edges_[i] != *other.incident_edges_[i]) {
                    return false;
                }
            }
            // Went through all edges and they were all equal, return true
            return true;
        }

        // edge list sizes were not equal, return false
        return false;
    }

    bool Parsing::AdjacencyList::operator!=(const Parsing::AdjacencyList & other) const {
        return !(*this == other);
    }

    void Parsing::AdjacencyList::add_edge(Parsing::DirectedEdge * edge) {
        incident_edges_.push_back(edge);
    }

    void Parsing::AdjacencyList::remove_edge(Parsing::DirectedEdge * edge, bool is_parent) {
        for (size_t i = 0; i < incident_edges_.size(); i++) {
            if (incident_edges_[i] == edge) {
                incident_edges_.erase(incident_edges_.begin() + i);
                
                if (is_parent) {
                    delete incident_edges_[i];
                    incident_edges_[i] = nullptr;
                }

                break;
            }
        }
        
    }

    Parsing::Node * Parsing::AdjacencyList::get_vertex() {
        return vertex_;
    }

    std::vector<Parsing::DirectedEdge *> & Parsing::AdjacencyList::get_edges() {
        return incident_edges_;
    };
} // namespace Parsing