#include <iostream>
#include <algorithm>
#include <stdexcept>
#include <unordered_map>

#include "../include/dfs.h"
#include "../include/digraph.h"

using namespace Parsing;

DFS::DFS(Digraph * g): g_(g) {
    for (auto pair: g_->get_adj_map()) {
        AdjacencyList * list = pair.second;
        Node * vertex = list->get_vertex();
        DFSNode * node = new DFSNode();
        node->vertex = vertex;
        node->predecessor = nullptr;
        dfs_nodes_[vertex->get_name()] = node;
    }
}

DFS::~DFS() {
    for (auto pair : dfs_nodes_) {
        DFSNode * node = pair.second;
        delete node;
    }
}

bool DFS::run(std::string start, std::string end){
    // Clearing potential previous run
    toVisit = stack<DFSNode *>();
    for (auto pair: dfs_nodes_) {
        DFSNode * node = pair.second;
        node->predecessor = nullptr;
    }

    if (!(g_->vertex_exists(start)) || !(g_->vertex_exists(end))) {
        return false;
    }

    DFSNode* current = dfs_nodes_[start];
    //DFSNode* end_node = dfs_nodes_[end];

    std::unordered_map<string, DFSNode *> visited; // (node name, True/F)

    toVisit.push(current);
    
    while (!toVisit.empty()) {
        current = toVisit.top();
        toVisit.pop();
        visited[current->vertex->get_name()] = current;

        
        if (current->vertex->get_name() == end) {
           //path found from start -> end

            //path_.push_back(current->get_name());
            return true; 
        }
        //current already visited
        // if (visited.find(current->vertex->get_name()) != visited.end())
        //     continue;

        for (auto neighbor : g_->get_adj_list(current->vertex)->get_edges()) { 
            if (neighbor->is_parent(current->vertex)) {
                if (visited.find(neighbor->get_destination()->get_name()) == visited.end()) {
                    DFSNode * to_push = dfs_nodes_[neighbor->get_destination()->get_name()];
                    to_push->predecessor = current;
                    toVisit.push(to_push);
                }
            }
        }

        //path_.push_back(current->get_name()); //includes "stray" paths

        
    }

    //all nodes traversed and no path found so delete path
    return false;

}

std::vector<std::string> DFS::get_path(std::string start, std::string end){
    path_.clear();
    if (run(start, end)) {
        DFSNode * current = dfs_nodes_[end];

        while (current) {
            path_.insert(path_.begin(), current->vertex->get_name());
            current = current->predecessor;
        }

        return path_;
    }
    return path_;
}