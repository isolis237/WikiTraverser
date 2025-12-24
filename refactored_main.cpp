#include "main/refactored_include/digraph_builder.h"

#include <iostream>
#include <string>
#include <algorithm>
#include <chrono>
#include <iomanip>     // std::fixed, std::setprecision

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <wikilinks.tsv>\n";
        return 1;
    }

    const std::string filepath = argv[1];

    try {
        const auto t0 = std::chrono::steady_clock::now();

        auto g = Graph::DigraphBuilder::BuildFromTsvCached(
            filepath, "",
            [](int pct, std::size_t nodes, std::size_t edges) {
                std::cerr << pct << "%... (nodes=" << nodes << ", edges=" << edges << ")\n";
            },
            10 // report every 10%
        );

        const auto t1 = std::chrono::steady_clock::now();
        const std::chrono::duration<double> elapsed = t1 - t0;
        const double secs = elapsed.count();

        std::cout << std::fixed << std::setprecision(3);
        std::cout << "\n=== Timing ===\n";
        std::cout << "Total time: " << secs << " s\n";
        if (secs > 0.0) {
            std::cout << "Edges/sec:  " << (static_cast<double>(g.num_edges()) / secs) << "\n";
            std::cout << "Nodes/sec:  " << (static_cast<double>(g.num_nodes()) / secs) << "\n";
        }

        std::cout << "\n=== Graph Loaded ===\n";
        std::cout << "File:  " << filepath << "\n";
        std::cout << "Nodes: " << g.num_nodes() << "\n";
        std::cout << "Edges: " << g.num_edges() << "\n";

        if (g.num_nodes() == 0) {
            std::cout << "Graph is empty.\n";
            return 0;
        }

        // Basic sanity checks: sample a few nodes and print out-degree + title
        std::cout << "\n=== Sample Nodes ===\n";
        const std::size_t samples = std::min<std::size_t>(5, g.num_nodes());
        for (std::size_t i = 0; i < samples; ++i) {
            const auto deg = g.out_degree(static_cast<Graph::NodeId>(i));
            std::cout << "Node " << i
                      << " | out_degree=" << deg
                      << " | title=\"" << g.title(static_cast<Graph::NodeId>(i)) << "\"\n";
        }

        // Also print max out-degree among sampled nodes (tiny quick check)
        std::size_t max_deg = 0;
        Graph::NodeId max_node = 0;
        const std::size_t scan = std::min<std::size_t>(10000, g.num_nodes()); // scan first N for speed
        for (std::size_t i = 0; i < scan; ++i) {
            auto deg = g.out_degree(static_cast<Graph::NodeId>(i));
            if (deg > max_deg) {
                max_deg = deg;
                max_node = static_cast<Graph::NodeId>(i);
            }
        }
        std::cout << "\n=== Quick Degree Check ===\n";
        std::cout << "Max out_degree among first " << scan << " nodes: "
                  << max_deg << " (node " << max_node
                  << ", title=\"" << g.title(max_node) << "\")\n";

    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}
