#pragma once

#include "digraph.h"

#include <cstddef>
#include <string>

namespace Graph {

struct VisualizationOptions {
    std::string source_path;
    std::string output_path = "visualization/graph.json";
    std::string start_title;
    std::string end_title;
    std::size_t max_nodes = 320;
    std::size_t neighbor_depth = 2;
    std::size_t max_neighbors_per_node = 32;
    std::string cluster_algorithm = "louvain";
    std::size_t cluster_iterations = 24;
    double cluster_resolution = 1.0;
};

void ExportVisualizationJson(const Digraph& graph, const VisualizationOptions& options);

} // namespace Graph
