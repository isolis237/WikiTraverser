#include <algorithm>

#include "../include/node.h"

namespace Parsing  {
    Parsing::Node::Node() {weight_ = 1;}
    
    Parsing::Node::Node(std::string name, int weight): name_(name), weight_(weight)  {}

    Parsing::Node::Node(const Node & other) {
        copy_(other);
    }

    Parsing::Node& Parsing::Node::operator=(const Node & other) {
        copy_(other);
        return *this;
    }

    bool Parsing::Node::operator==(const Node & other) const {
        return (name_ == other.name_ && weight_ == other.weight_);
    }

    bool Parsing::Node::operator!=(const Node & other) const {
        return !(*this == other);
    }

    std::string Parsing::Node::get_name() const {
        return name_;
    }

    int Parsing::Node::get_weight() const {
        return weight_;
    }

    void Parsing::Node::copy_(const Node & other) {
        name_ = other.name_;
        weight_ = other.weight_;
    }
}