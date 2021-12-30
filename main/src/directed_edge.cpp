#include "../include/directed_edge.h"

namespace Parsing {
    Parsing::DirectedEdge::DirectedEdge(Parsing::Node * parent, Parsing::Node  * destination, int weight): parent_(parent), destination_(destination), weight_(weight) {}

    Parsing::DirectedEdge::~DirectedEdge() {}

    bool Parsing::DirectedEdge::operator==(const DirectedEdge & other) const {
        return (parent_ == other.parent_ && destination_ == other.destination_ && weight_ == other.weight_);
    }

    bool Parsing::DirectedEdge::operator!=(const DirectedEdge & other) const {
        return !(*this == other);
    }

    Parsing::Node * Parsing::DirectedEdge::get_parent() {
        return parent_;
    }

    Parsing::Node * Parsing::DirectedEdge::get_destination() {
        return destination_;
    }

    int Parsing::DirectedEdge::get_weight() const {
        return weight_;
    }

    void Parsing::DirectedEdge::set_weight(int weight) {
        weight_ = weight;
    }

    bool Parsing::DirectedEdge::is_parent(const Node * vertex) const{
        return vertex == parent_;
    }
} // Parsing