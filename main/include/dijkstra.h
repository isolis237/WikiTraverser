#pragma once

#include <queue>
#include <iostream>

#include "digraph.h"

using namespace Parsing;

struct DijkstraNode {
    int distance;
    DijkstraNode * predecessor;
    Node * vertex;

    
    bool operator<(const DijkstraNode & other) {
        if (distance < other.distance) {
            return true;
        }

        return false;
    }
    // Needed for use in the priority queue in the dijkstra algorithm
    bool operator>(const DijkstraNode & other) {
        return !(*this < other);
    }
};

class CompareDijkstraNodes {
    public:
        bool operator()(const DijkstraNode * lhs, const DijkstraNode * rhs) {
            return lhs->distance > rhs->distance;
        }
};
/**
 * @brief A class that has all of the necessary components to run
 * Dijkstra's shortest path algorithm
 * 
 */
class Dijkstra {
    public:
        

        /**
         * @brief Constructor for Dijkstra class
         * 
         * @param G The Graph that the Dijkstra algorithm will run on
         */
        Dijkstra(Digraph * G);

        ~Dijkstra();

        /**
         * @brief Runs Dijkstra's shortest path algorithm on the graph G and returns path
         * from start_name's vertex to end_name's vertex
         * 
         * @param start_name The name of the vertex to start the algorithm on
         * @param end_name The name of the vertex we want a path to from starting vertex
         * 
         * @return std::vector<std::string> Of all the vertices visited (in order)
         * to get from starting vertex to ending vertex. The starting vertex and
         * ending vertex are included in this vector.
         */
        std::vector<std::string> get_path(std::string start_name, std::string end_name);

        /**
         * @brief Runs Dijkstra's algorithm on Graph G with the starting vertex that corresponds
         * to start_name
         * 
         * @param start_name The name of the vertex to start the algorithm on.
         */
        void run(DijkstraNode * s);

        std::unordered_map<std::string, DijkstraNode *> & get_vertex_map();
        
    
    private:
        Digraph * G_;

        std::unordered_map<std::string, DijkstraNode *> vertex_map_;
        std::vector<DijkstraNode *> vertex_list_;
};


