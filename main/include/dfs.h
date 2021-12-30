#pragma once

#include "digraph.h"

#include <stdint.h>
#include <string>
#include <vector>
#include <stack>

using std::string;
using std::vector;
using std::stack;

using namespace Parsing;

struct DFSNode {
    Node * vertex;
    DFSNode * predecessor;
};

class DFS {
    public:
        /**
         * @brief Constructor for DFS class
         * 
         * @param g The Graph that the DFS algorithm will run on
         */
        DFS(Digraph * g);

        ~DFS();

        /**
         * @brief Runs DFS on the graph and returns true if path is found
         * from start vertex to end vertex 
         * 
         * @param start The vertex the DFS will begin at
         * @param end the vertex the DFS will end at
         */
        bool run(string start, string end);

        /**
        * @brief returns a path from the starting vertex to the end
        * using DFS algorithm
        *
        * @param start The name of the vertex to start at
        * @param end The name of the vertex the DFS will end at
        */
        vector<string> get_path(string start, string end);

    private:
        Digraph * g_;
        stack<DFSNode*> toVisit;
        vector<string> path_;
        Node* start_;
        std::unordered_map<std::string, DFSNode *> dfs_nodes_;
};
