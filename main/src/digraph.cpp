#include <iostream>
#include <algorithm>
#include <stdexcept>
#include <fstream>
#include <cctype>
#include <utility>
#include <sstream>

#include "../include/digraph.h"

using namespace Parsing;

Digraph::Digraph() {}

Digraph::Digraph(std::string tsv_path) {
    std::ifstream file_stream;
    std::string line;
    std::vector<std::pair<std::string, std::string>> link_pairs;

    file_stream.open(tsv_path);

    if (!file_stream.is_open()) {
        throw std::invalid_argument("Could not open file! (Is the path correct?)");
    }

    while (std::getline(file_stream, line)) {
        // Check to make sure the first char is either alphanumeric or '%' (ignore all empty / comment lines)
        if (std::isalnum(line[0]) || line[0] == '%') {
            std::stringstream iss(line);
            std::string parent_string, destination_string;
            iss >> parent_string >> destination_string; // Puts first string in parent_string, second string in destination_string (tab separated)
            
            Parsing::Node * parent;
            Parsing::Node  * destination;

            if (!vertex_exists(parent_string)) {
                // Node with parent_string name doesn't exist, create one.
                parent = new Parsing::Node(parent_string);
                add_vertex(parent);
            } else {
                // Node with parent_string name already exists, load it for edge creation.
                parent = get_vertex(parent_string);
            }

            if (!vertex_exists(destination_string)) {
                // Node with destination_string name doesn't exist, create one.
                destination = new Parsing::Node(destination_string);
                add_vertex(destination);
            } else {
                // Node with destination_string name already exists, load it for edge creation.
                destination = get_vertex(destination_string);
            }

            if (!edge_exists(parent, destination)) {
                // Edge between parent and destination Nodes doesn't exist, create one.
                add_edge(parent, destination);
            }
        }
    }
}

Digraph::~Digraph() {
    for (auto pair : adjacency_lists_) {
        AdjacencyList * list = pair.second;
        delete list;
        list = nullptr;
    }
}

void Digraph::add_vertex(Node * vertex) {
    if (adjacency_lists_.find(vertex->get_name()) != adjacency_lists_.end()) {
        // Vertex already has been added, throw error
        throw std::invalid_argument("Tried to add vertex that was already in the graph!");
    }

    // Vertex not in graph, add vertex.
    AdjacencyList * adj_list = new AdjacencyList(vertex);
    adjacency_lists_[vertex->get_name()] = adj_list;
    
    ++vertex_count_;
}

void Digraph::add_edge(Node * parent, Node * destination, int weight) {
    if (!vertex_exists(parent)) {
        throw std::invalid_argument("Tried to create an edge with parent vertex that is not in the graph!");
    } else if (!vertex_exists(destination)) {
        throw std::invalid_argument("Tried to create an edge with destination vertex that is not in the graph!");
    }
    // check to make sure that edge doesnt already exist
    if (edge_exists(parent, destination)) {
        throw std::invalid_argument("Tried to add edge that already exists!");
    }
    DirectedEdge * edge = new DirectedEdge(parent, destination, weight);

    adjacency_lists_[parent->get_name()]->add_edge(edge);
    adjacency_lists_[destination->get_name()]->add_edge(edge);
    ++edge_count_;
}

void Digraph::remove_vertex(Node * vertex) {
    if (!vertex_exists(vertex)) {
        throw std::invalid_argument("Tried to remove vertex that doesn't exist in the graph");
    }
    AdjacencyList * list =  adjacency_lists_[vertex->get_name()];
    adjacency_lists_.erase(vertex->get_name());
    delete list;
    --vertex_count_;
}

void Digraph::remove_edge(Node * parent, Node * destination) {
    if (!edge_exists(parent, destination)) {
        throw std::invalid_argument("Tried to remove edge that doesn't exist in the graph!");
    }

    DirectedEdge * edge = get_edge(parent, destination);

    adjacency_lists_[parent->get_name()]->remove_edge(edge, true);
    adjacency_lists_[destination->get_name()]->remove_edge(edge, false);
    --edge_count_;
}

bool Digraph::vertex_exists(std::string name) {
    if (adjacency_lists_.find(name) != adjacency_lists_.end()) {
        // We do not compare weight equality because we do not want multiple vertices with the same name, but different weights;
        return true;
    }

    return false;
}

bool Digraph::vertex_exists(Node * vertex) {
    if (vertex == nullptr) {
        return false;
    }
    return vertex_exists(vertex->get_name());
}

bool Digraph::edge_exists(Parsing::Node * parent, Parsing::Node * destination) {
    if (vertex_exists(parent) && vertex_exists(destination)) {
        bool in_parent_list = false;
        bool in_destination_list = false;
        std::vector<DirectedEdge *> & parent_list = adjacency_lists_[parent->get_name()]->get_edges();
        std::vector<DirectedEdge *> & destination_list = adjacency_lists_[destination->get_name()]->get_edges();

        // We do not compare weight here because we do not want multiple edges with the same parent/destination node setup, but with different weights;

        for (DirectedEdge * plist_edge : parent_list) {
            if (parent == plist_edge->get_parent() && destination == plist_edge->get_destination()) {
                in_parent_list = true;
                break;
            }
        }

        for (DirectedEdge * dlist_edge : destination_list) {
            if (parent == dlist_edge->get_parent() && destination == dlist_edge->get_destination()) {
                in_destination_list = true;
                break;
            }
        }

        // Edge must in both adjacency lists in order to be a valid, existing edge.
        return (in_parent_list && in_destination_list);
    }

    return false;
}

Node * Digraph::get_vertex(std::string name) {
    if (adjacency_lists_.find(name) != adjacency_lists_.end()) {
        return adjacency_lists_[name]->get_vertex();
    } else {
        throw std::invalid_argument("Vertex with input name not found in graph!");
    }
}

DirectedEdge * Digraph::get_edge(Node * parent, Node * destination) {
    AdjacencyList * list = get_adj_list(parent);

    for (DirectedEdge * edge : list->get_edges()) {
        if (edge->get_destination() == destination) {
            return edge;
        }
    }

    throw std::invalid_argument("Could not find edge in graph! (does it exist?)");
}

AdjacencyList * Digraph::get_adj_list(Node * vertex) {
    if (!vertex_exists(vertex)) {
        throw std::invalid_argument("Tried to get adjacency list of vertex that does not exist in graph!");
    }
    return adjacency_lists_[vertex->get_name()];
}

std::unordered_map<std::string, Parsing::AdjacencyList *> & Digraph::get_adj_map() {
    return adjacency_lists_;
}

bool Digraph::empty() {
    return adjacency_lists_.empty();
}