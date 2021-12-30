#include <limits>
#include <iostream>

#include "../include/dijkstra.h"

using namespace Parsing;

Dijkstra::Dijkstra(Digraph * G): G_(G) {
    //Setting up vertex map to use with dijkstra's
    for (auto pair : G_->get_adj_map()) {
        AdjacencyList * list = pair.second;
        Node * vertex = list->get_vertex();
        DijkstraNode * dijkstra_vertex = new DijkstraNode;
        dijkstra_vertex->vertex = vertex;
        dijkstra_vertex->predecessor = nullptr;
        vertex_map_[vertex->get_name()] = dijkstra_vertex;
        vertex_list_.push_back(dijkstra_vertex);
    }
}

Dijkstra::~Dijkstra() {
    for (DijkstraNode * node : vertex_list_) {
        node->vertex = nullptr;
        node->predecessor = nullptr;
        delete node;
        node = nullptr;
    }
}

std::vector<std::string> Dijkstra::get_path(std::string start_name, std::string end_name) {
    // // todo make sure that passed names are in vertex mapping
    // run(start_name);
    std::vector<std::string> path;

    if (vertex_map_[start_name]->predecessor != nullptr || vertex_map_[end_name]->predecessor == nullptr) {
        // The current vertex_map is not set up with a Dijkstra run with start_name, run dijkstra's
        run(vertex_map_[start_name]);
    }

    // get path from end to start, starting at end node
    DijkstraNode * node = vertex_map_[end_name];

    while (node != nullptr) {
        path.insert(path.begin(), node->vertex->get_name());
        node = node->predecessor;
    }

    return path;
}

void Dijkstra::run(DijkstraNode * s) {
    // todo check to make sure the passed name is in vertex mapping
    for (DijkstraNode * vertex : vertex_list_) {
        // Set distance to max int value (basically +inf);
        vertex->distance = std::numeric_limits<int>::max();
        vertex->predecessor = nullptr;
    }

    s->distance = 0;

    std::priority_queue<DijkstraNode *,
        std::vector<DijkstraNode *>,
        CompareDijkstraNodes>
            p_queue(vertex_list_.begin(), vertex_list_.end());
    
    std::unordered_map<std::string, DijkstraNode *> added;

    for (size_t i = 0; i < vertex_list_.size(); i++) {
        DijkstraNode * u = p_queue.top();
        added[u->vertex->get_name()] = u;

        for (DirectedEdge * edge : G_->get_adj_list(u->vertex)->get_edges()) {
            Node * v_vertex = edge->get_destination();

            if (edge->is_parent(u->vertex) && added.find(v_vertex->get_name()) == added.end()) {
                // We havent added this node to the new graph, continue
                DijkstraNode * v = vertex_map_[v_vertex->get_name()];

                if (edge->get_weight() + u->distance < v->distance) {
                    v->distance = edge->get_weight() + u->distance;
                    v->predecessor = u;
                    // STL priority_queue does not support updating of values once placed in the queue, add duplicate to resort
                    // Comes at the cost of higher memory complexity, but since the outer loop only runs once for the total amount
                    // of nodes, this is fine.
                    p_queue.push(v);
                }
            }
        }

        p_queue.pop();
    }
}

std::unordered_map<std::string, DijkstraNode *> & Dijkstra::get_vertex_map() {
    return vertex_map_;
}

//TODO FIGURE THIS OUT LATER