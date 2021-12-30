#pragma once

#include <vector>
#include <string>
#include <unordered_map>

#include "node.h"
#include "directed_edge.h"
#include "adjacency_list.h"

/**
 * @brief This is a representation of a directed graph (digraph).
 * 
 */
class Digraph {
  public:
    unsigned vertex_count_ = 0;
    unsigned edge_count_ = 0;
    Digraph();

    /**
     * @brief Construct a new Digraph object from a .tsv file
     * 
     * NOTE: This implementation relies on the fact that our values will either start with
     * a-z, A-Z, 0-9, or %, all other lines in the tsv file will be skipped.
     * 
     * NOTE: This implementation relies that the tsv file structure only has two data entries per line.
     * 
     * @param tsv_path The file path to the tsv_file
     */
    Digraph(std::string tsv_path);

    ~Digraph();

    void add_vertex(Parsing::Node * vertex);

    void add_edge(Parsing::Node * parent, Parsing::Node * destination, int weight = 1);

    void remove_vertex(Parsing::Node * vertex);

    void remove_edge(Parsing::Node * parent, Parsing::Node * destination);

    bool vertex_exists(std::string name);

    bool vertex_exists(Parsing::Node * vertex);

    bool edge_exists(Parsing::Node * parent, Parsing::Node * destination);

    /**
     * @brief Get the node that corresponds with the string name
     * 
     * @param name 
     * @returns The node object that corresponds with the input string.
     */
    Parsing::Node * get_vertex(std::string name);

    Parsing::DirectedEdge * get_edge(Parsing::Node * parent, Parsing::Node * destination);

    Parsing::AdjacencyList * get_adj_list(Parsing::Node * vertex);

    std::unordered_map<std::string, Parsing::AdjacencyList *> & get_adj_map();

    bool empty();

  private:
    // Article names (string) are associated with corresponding AdjacencyList
    std::unordered_map<std::string, Parsing::AdjacencyList *> adjacency_lists_;
};