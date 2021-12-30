#pragma once

#include <vector>

#include "node.h"
#include "directed_edge.h"



namespace Parsing {
    /**
     * @brief Class to represent the adjacency list of a node and its edges
     * 
     */
    class AdjacencyList {
        public:
            AdjacencyList();
            AdjacencyList(Parsing::Node * vertex);
            ~AdjacencyList();
            bool operator==(const AdjacencyList & other) const;
            bool operator!=(const AdjacencyList & other) const;
            void add_edge(Parsing::DirectedEdge * edge);

            /**
             * @brief Removed edge from the adjacency list
             * 
             * @param end_node The other node in the edge (that is not vertex_)
             * @param is_parent Whether or not vertex_ is the parent vertex of the directed edge.
             */
            void remove_edge(Parsing::DirectedEdge * edge, bool is_parent);
            Parsing::Node * get_vertex();
            std::vector<Parsing::DirectedEdge *> & get_edges();

        private:
            Parsing::Node * vertex_;
            std::vector<Parsing::DirectedEdge *> incident_edges_;
    };
} // namespace Parsing