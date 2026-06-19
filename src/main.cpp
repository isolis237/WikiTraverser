#include "digraph_builder.h"
#include "graph_algorithms.h"
#include "visualization_exporter.h"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void print_usage(const char* program) {
    std::cerr
        << "Usage:\n"
        << "  " << program << " <wikilinks.tsv>\n"
        << "  " << program << " path <wikilinks.tsv> <start title> <end title>\n"
        << "  " << program << " export <wikilinks.tsv> <output.json> [options]\n\n"
        << "Export options:\n"
        << "  --start <title>       Article to expand from\n"
        << "  --end <title>         Article to find/animate a path to\n"
        << "  --nodes <count>       Max nodes in exported subset (default: 320)\n"
        << "  --depth <count>       Outgoing-neighbor expansion depth (default: 2)\n"
        << "  --neighbors <count>   Max outgoing neighbors per expanded node (default: 32)\n"
        << "  --cluster <name>      Clustering mode: louvain, label-propagation,\n"
        << "                        weak-components, or seed-expansion (default: louvain)\n"
        << "  --cluster-resolution <value>\n"
        << "                        Louvain resolution; higher means more clusters (default: 1.0)\n"
        << "  --cluster-iterations <count>\n"
        << "                        Max clustering iterations for label/louvain modes (default: 24)\n";
}

std::size_t parse_size_arg(const std::string& value, const std::string& name) {
    try {
        std::size_t consumed = 0;
        const auto parsed = std::stoull(value, &consumed);
        if (consumed != value.size()) {
            throw std::invalid_argument("extra characters");
        }
        return static_cast<std::size_t>(parsed);
    } catch (const std::exception&) {
        throw std::runtime_error("Invalid value for " + name + ": " + value);
    }
}

double parse_double_arg(const std::string& value, const std::string& name) {
    try {
        std::size_t consumed = 0;
        const double parsed = std::stod(value, &consumed);
        if (consumed != value.size()) {
            throw std::invalid_argument("extra characters");
        }
        return parsed;
    } catch (const std::exception&) {
        throw std::runtime_error("Invalid value for " + name + ": " + value);
    }
}

Graph::Digraph load_graph(const std::string& filepath) {
    return Graph::DigraphBuilder::BuildFromTsvCached(
        filepath, "",
        [](int pct, std::size_t nodes, std::size_t edges) {
            std::cerr << pct << "%... (nodes=" << nodes << ", edges=" << edges << ")\n";
        },
        10
    );
}

void print_graph_summary(const Graph::Digraph& g, const std::string& filepath, double secs) {
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
        return;
    }

    std::cout << "\n=== Sample Nodes ===\n";
    const std::size_t samples = std::min<std::size_t>(5, g.num_nodes());
    for (std::size_t i = 0; i < samples; ++i) {
        const auto deg = g.out_degree(static_cast<Graph::NodeId>(i));
        std::cout << "Node " << i
                  << " | out_degree=" << deg
                  << " | title=\"" << g.title(static_cast<Graph::NodeId>(i)) << "\"\n";
    }

    std::size_t max_deg = 0;
    Graph::NodeId max_node = 0;
    const std::size_t scan = std::min<std::size_t>(10000, g.num_nodes());
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
}

int run_summary(const std::string& filepath) {
    const auto t0 = std::chrono::steady_clock::now();
    auto g = load_graph(filepath);
    const auto t1 = std::chrono::steady_clock::now();
    const std::chrono::duration<double> elapsed = t1 - t0;
    print_graph_summary(g, filepath, elapsed.count());
    return 0;
}

int run_path(int argc, char** argv) {
    if (argc < 5) {
        print_usage(argv[0]);
        return 1;
    }

    const std::string filepath = argv[2];
    const std::string start_title = argv[3];
    const std::string end_title = argv[4];

    auto g = load_graph(filepath);
    const Graph::NodeId start = Graph::FindNodeIdByTitle(g, start_title);
    const Graph::NodeId end = Graph::FindNodeIdByTitle(g, end_title);

    if (start == Graph::kInvalidNodeId) {
        throw std::runtime_error("Start article not found: " + start_title);
    }
    if (end == Graph::kInvalidNodeId) {
        throw std::runtime_error("End article not found: " + end_title);
    }

    const auto path = Graph::ShortestPathBfs(g, start, end);
    if (path.empty()) {
        std::cout << "No path found from \"" << g.title(start)
                  << "\" to \"" << g.title(end) << "\".\n";
        return 0;
    }

    std::cout << "Shortest path from \"" << g.title(start)
              << "\" to \"" << g.title(end) << "\":\n";
    for (std::size_t i = 0; i < path.size(); ++i) {
        std::cout << "  " << (i + 1) << ". " << g.title(path[i]) << "\n";
    }
    std::cout << "Path length: " << path.size() << " nodes\n";
    return 0;
}

int run_export(int argc, char** argv) {
    if (argc < 4) {
        print_usage(argv[0]);
        return 1;
    }

    Graph::VisualizationOptions options;
    const std::string filepath = argv[2];
    options.source_path = filepath;
    options.output_path = argv[3];

    for (int i = 4; i < argc; ++i) {
        const std::string arg = argv[i];
        auto require_value = [&](const std::string& option_name) -> std::string {
            if (i + 1 >= argc) {
                throw std::runtime_error("Missing value for " + option_name);
            }
            return argv[++i];
        };

        if (arg == "--start") {
            options.start_title = require_value(arg);
        } else if (arg == "--end") {
            options.end_title = require_value(arg);
        } else if (arg == "--nodes" || arg == "--max-nodes") {
            options.max_nodes = parse_size_arg(require_value(arg), arg);
        } else if (arg == "--depth") {
            options.neighbor_depth = parse_size_arg(require_value(arg), arg);
        } else if (arg == "--neighbors") {
            options.max_neighbors_per_node = parse_size_arg(require_value(arg), arg);
        } else if (arg == "--cluster" || arg == "--clustering") {
            options.cluster_algorithm = require_value(arg);
        } else if (arg == "--cluster-resolution" || arg == "--resolution") {
            options.cluster_resolution = parse_double_arg(require_value(arg), arg);
        } else if (arg == "--cluster-iterations") {
            options.cluster_iterations = parse_size_arg(require_value(arg), arg);
        } else {
            throw std::runtime_error("Unknown option: " + arg);
        }
    }

    auto g = load_graph(filepath);
    Graph::ExportVisualizationJson(g, options);

    std::cout << "Wrote visualization data to " << options.output_path << "\n";
    std::cout << "Open visualization/index.html through a local web server to explore it.\n";
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    try {
        const std::string command = argv[1];
        if (command == "-h" || command == "--help" || command == "help") {
            print_usage(argv[0]);
            return 0;
        }
        if (command == "path") return run_path(argc, argv);
        if (command == "export") return run_export(argc, argv);
        return run_summary(command);
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}
