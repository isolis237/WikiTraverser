#pragma once

#include "node.h"

namespace Parsing {
    /**
     * @brief Class for representing directed edges (wikipedia hyperlinks)
     * 
     */
    class DirectedEdge {
        public:
            /**
             * @brief Constructor for DirectedEdge
             * 
             * @param parent The node that the directed edge is coming from. (the linking article)
             * @param destination The node that the directed edge is going to. (the linked article)
             */
            DirectedEdge(Parsing::Node * parent, Parsing::Node * destination, int weight = 1);

            ~DirectedEdge();

            /**
             * @brief Equality operator for DirectedEdge
             * 
             * @param other 
             */
            bool operator==(const Parsing::DirectedEdge & other) const;

            bool operator!=(const Parsing::DirectedEdge & other) const;

            /**
             * @brief Get the parent node. (The linking article)
             * 
             * @return The node object representing the parent article 
             */
            Parsing::Node * get_parent();

            /**
             * @brief Get the destination node. (The linked article)
             * 
             * @return The node object representing the linked article
             */
            Parsing::Node * get_destination();

            /**
             * @brief Get the weight of the edge
             * 
             * @return int representing the weight of the edge
             */
            int get_weight() const;

            /**
             * @brief Set the wieght of the edge
             * 
             */
            void set_weight(int weight);

            /**
             * @brief Used to determine if given vertex is parent of the edge (or else is destination)
             *  NOTE: THIS FUNCTION ASSUMES USER GIVE A VERTEX THAT IS CONTAINED IN THE EDGE, putting a random vertex
             * as the argument will always result in false
             * 
             * @param vertex The vertex to check.
             * 
             * @return A boolean representing whether or not the input is the parent vertex of the edge.
             */
            bool is_parent(const Parsing::Node * vertex) const;

        private:
            /**
             * Despite being pointers, do not delete these when edge is destroyed.
             * We want the lifetime of these Node objects to be longer than the edge.
             * (i.e. when we remove an edge between two nodes, we still want the nodes to exist)
             */
            Parsing::Node * parent_;
            Parsing::Node * destination_;
            int weight_;
    };
} // namespace Parsing