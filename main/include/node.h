#pragma once

#include <stdint.h>
#include <string>
#include <vector>


namespace Parsing {
    /**
     * @brief Class to represent a wikipedia article and its hyperlinks
     * 
     */
    class Node {
        public:
            /**
             * @brief Default constructor
             * 
             */
            Node();

            /**
             * @brief Construct a new Node object
             * 
             * @param name The name of the article the node will be representing.
             */
            Node(std::string name, int weight = 1);
            
            /**
             * @brief Copy constructor for Node.
             * 
             * @param other 
             */
            Node(const Node & other);

            /**
             * @brief Copy assignment operator
             * 
             * @param other The node to copy
             * @return Node& 
             */
            Node& operator=(const Node & other);

            /**
             * @brief Equality operator
             * 
             * @param other The node to compare to
             * @return bool reperesenting equality between the nodes
             */
            bool operator==(const Node & other) const;

            bool operator!=(const Node & other) const;

            std::string get_name() const;

            int get_weight() const;


        private:
            void copy_(const Node & other);
            std::string name_;
            int weight_;
    };
} // namespace Parsing
