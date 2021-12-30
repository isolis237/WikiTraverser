#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cctype>
#include <utility>
#include <sstream>

#include "main/include/digraph.h"
#include "main/include/dijkstra.h"
#include "main/include/dfs.h"


int main(int argc, char* argv[]) {
    
    //Create graph from data
    std::cout << "************************************************************************************" << std::endl;
    std::cout << "Creating graph from wikilinks." << std::endl << std::endl;
    Digraph wikigraph("./data/links.tsv");
    std::cout << "Vertex Count: " << wikigraph.vertex_count_ << std::endl;
    std::cout << "Edge Count: " << wikigraph.edge_count_ << std::endl;
    std::cout << "************************************************************************************" << std::endl << std::endl;

    //if input isn't in dataset give default values
    string start = "Military_dictatorship";
    string end = "Michael_Jordan";

    if (wikigraph.vertex_exists(argv[1]))
        start = argv[1];
    if (wikigraph.vertex_exists(argv[2]))
        end = argv[2];

    //run dijkstra's algorithm from given input
    Dijkstra d(&wikigraph);
    std::cout << "************************************************************************************" << std::endl;
    std::cout << "Performing Dijkstra's Algorithm to get the path from "<< start << " to " << end << std::endl << std::endl;
    std::vector<std::string> path = d.get_path(start, end);
    for (std::string edge: path) {
        std::cout << edge << std::endl;
    }
    std::cout << std::endl;
    std::cout << "Path Length: " << path.size() << std::endl;
    std::cout << "************************************************************************************" << std::endl << std::endl;

    //run dfs algorithm from given input
    DFS dfs(&wikigraph);
    std::cout << "************************************************************************************" << std::endl;
    std::cout << "Performing DFS Algorithm to get the path from " << start << " to " << end << std::endl << std::endl;
    std::vector<std::string> dfs_path = dfs.get_path(start, end);
    for (size_t i = 0; i < 5; i++) {
        std::cout << dfs_path[i] << std::endl;
    }
    std::cout << "..." << std::endl << "..." << std::endl << "..." << std::endl;
    for (size_t i = dfs_path.size() - 6; i < dfs_path.size(); i++) {
        std::cout << dfs_path[i] << std::endl;
    }
    std::cout << std::endl;
    std::cout << "Path Length: " << dfs_path.size() << std::endl;
    std::cout << "************************************************************************************" << std::endl;

    return 0;
}