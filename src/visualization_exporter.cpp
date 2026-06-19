#include "visualization_exporter.h"

#include "community_detection.h"
#include "graph_algorithms.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <queue>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace Graph {
namespace {

struct FrontierItem {
    NodeId node;
    std::size_t depth;
    int cluster;
};

struct EdgeJson {
    std::size_t source;
    std::size_t target;
    bool in_path;
};

std::uint64_t edge_key(NodeId from, NodeId to) {
    return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(from)) << 32) |
           static_cast<std::uint32_t>(to);
}

void write_json_string(std::ostream& out, const std::string& value) {
    out << '"';
    for (unsigned char c : value) {
        switch (c) {
            case '"': out << "\\\""; break;
            case '\\': out << "\\\\"; break;
            case '\b': out << "\\b"; break;
            case '\f': out << "\\f"; break;
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;
            default:
                if (c < 0x20) {
                    out << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                        << static_cast<int>(c) << std::dec << std::setfill(' ');
                } else {
                    out << static_cast<char>(c);
                }
                break;
        }
    }
    out << '"';
}

std::vector<NodeId> highest_out_degree_nodes(const Digraph& graph, std::size_t count) {
    std::vector<NodeId> ids;
    ids.reserve(graph.num_nodes());
    for (std::size_t i = 0; i < graph.num_nodes(); ++i) {
        ids.push_back(static_cast<NodeId>(i));
    }

    const auto by_degree = [&](NodeId a, NodeId b) {
        const std::size_t da = graph.out_degree(a);
        const std::size_t db = graph.out_degree(b);
        if (da != db) return da > db;
        return graph.title(a) < graph.title(b);
    };

    if (ids.size() > count) {
        std::partial_sort(ids.begin(), ids.begin() + static_cast<std::ptrdiff_t>(count),
                          ids.end(), by_degree);
        ids.resize(count);
    } else {
        std::sort(ids.begin(), ids.end(), by_degree);
    }

    return ids;
}

NodeId resolve_title_or_throw(const Digraph& graph,
                              const std::string& title,
                              const char* role) {
    const NodeId node = FindNodeIdByTitle(graph, title);
    if (node == kInvalidNodeId) {
        throw std::runtime_error(std::string("Could not find ") + role +
                                 " article in graph: " + title);
    }
    return node;
}

std::size_t count_included_clusters(const std::vector<NodeId>& included_order,
                                    const std::vector<int>& cluster_by_node) {
    std::unordered_set<int> clusters;
    clusters.reserve(included_order.size());
    for (NodeId node : included_order) {
        const auto idx = static_cast<std::size_t>(node);
        if (idx < cluster_by_node.size() && cluster_by_node[idx] >= 0) {
            clusters.insert(cluster_by_node[idx]);
        }
    }
    return clusters.size();
}

} // namespace

void ExportVisualizationJson(const Digraph& graph, const VisualizationOptions& options) {
    if (graph.num_nodes() == 0) {
        throw std::runtime_error("Cannot export visualization for an empty graph.");
    }

    const std::size_t requested_cap = std::max<std::size_t>(1, options.max_nodes);
    const std::size_t depth_limit = options.neighbor_depth;
    const std::size_t neighbor_limit = std::max<std::size_t>(1, options.max_neighbors_per_node);
    CommunityDetectionOptions community_options;
    community_options.algorithm = ParseCommunityAlgorithm(options.cluster_algorithm);
    community_options.max_iterations = std::max<std::size_t>(1, options.cluster_iterations);
    community_options.resolution = std::isfinite(options.cluster_resolution)
        ? std::max(0.05, options.cluster_resolution)
        : 1.0;
    const std::string community_algorithm = CommunityAlgorithmName(community_options.algorithm);

    NodeId start = kInvalidNodeId;
    NodeId end = kInvalidNodeId;

    if (!options.start_title.empty()) {
        start = resolve_title_or_throw(graph, options.start_title, "start");
    } else if (options.end_title.empty()) {
        start = HighestOutDegreeNode(graph);
    }

    if (!options.end_title.empty()) {
        end = resolve_title_or_throw(graph, options.end_title, "end");
    }

    std::vector<NodeId> path;
    if (start != kInvalidNodeId && end != kInvalidNodeId) {
        path = ShortestPathBfs(graph, start, end);
    }

    const std::size_t effective_cap = std::max(requested_cap, path.size());
    std::vector<char> included(graph.num_nodes(), 0);
    std::vector<int> cluster_by_node(graph.num_nodes(), -1);
    std::vector<int> path_index_by_node(graph.num_nodes(), -1);
    std::vector<NodeId> included_order;
    included_order.reserve(std::min(effective_cap, graph.num_nodes()));
    std::queue<FrontierItem> frontier;

    auto add_node = [&](NodeId node, int cluster) {
        const auto idx = static_cast<std::size_t>(node);
        if (!included[idx]) {
            included[idx] = 1;
            included_order.push_back(node);
            cluster_by_node[idx] = cluster;
        } else if (cluster_by_node[idx] < 0) {
            cluster_by_node[idx] = cluster;
        }
    };

    if (!path.empty()) {
        for (std::size_t i = 0; i < path.size(); ++i) {
            const NodeId node = path[i];
            const int cluster = static_cast<int>(i % 12);
            add_node(node, cluster);
            path_index_by_node[static_cast<std::size_t>(node)] = static_cast<int>(i);
            frontier.push(FrontierItem{node, 0, cluster});
        }
    } else if (start != kInvalidNodeId) {
        add_node(start, 0);
        frontier.push(FrontierItem{start, 0, 0});
    } else {
        const auto hubs = highest_out_degree_nodes(graph, std::min<std::size_t>(8, requested_cap));
        for (std::size_t i = 0; i < hubs.size(); ++i) {
            add_node(hubs[i], static_cast<int>(i));
            frontier.push(FrontierItem{hubs[i], 0, static_cast<int>(i)});
        }
    }

    while (!frontier.empty() && included_order.size() < effective_cap) {
        const FrontierItem current = frontier.front();
        frontier.pop();
        if (current.depth >= depth_limit) continue;

        std::size_t explored_neighbors = 0;
        for (const auto& edge : graph.out_edges(current.node)) {
            if (explored_neighbors >= neighbor_limit) break;
            ++explored_neighbors;

            const auto to_idx = static_cast<std::size_t>(edge.to);
            if (to_idx >= graph.num_nodes()) continue;

            if (!included[to_idx]) {
                if (included_order.size() >= effective_cap) break;
                add_node(edge.to, current.cluster);
                frontier.push(FrontierItem{edge.to, current.depth + 1, current.cluster});
            }
        }
    }

    std::unordered_map<NodeId, std::size_t> local_id_by_node;
    local_id_by_node.reserve(included_order.size());
    for (std::size_t i = 0; i < included_order.size(); ++i) {
        local_id_by_node.emplace(included_order[i], i);
    }

    std::unordered_set<std::uint64_t> path_edges;
    for (std::size_t i = 1; i < path.size(); ++i) {
        path_edges.insert(edge_key(path[i - 1], path[i]));
    }

    std::vector<std::size_t> visible_out_degree(included_order.size(), 0);
    std::vector<EdgeJson> edges;
    for (std::size_t local_source = 0; local_source < included_order.size(); ++local_source) {
        const NodeId source = included_order[local_source];
        for (const auto& edge : graph.out_edges(source)) {
            const auto found = local_id_by_node.find(edge.to);
            if (found == local_id_by_node.end()) continue;

            ++visible_out_degree[local_source];
            edges.push_back(EdgeJson{
                local_source,
                found->second,
                path_edges.find(edge_key(source, edge.to)) != path_edges.end()
            });
        }
    }

    if (community_options.algorithm != CommunityAlgorithm::SeedExpansion) {
        const auto communities = DetectCommunities(graph, included_order, community_options);
        if (communities.size() != included_order.size()) {
            throw std::runtime_error("Community detection returned an unexpected node count.");
        }
        for (std::size_t local_id = 0; local_id < included_order.size(); ++local_id) {
            cluster_by_node[static_cast<std::size_t>(included_order[local_id])] = communities[local_id];
        }
    }

    const std::size_t detected_communities =
        count_included_clusters(included_order, cluster_by_node);

    std::vector<std::size_t> local_path;
    local_path.reserve(path.size());
    for (NodeId node : path) {
        const auto found = local_id_by_node.find(node);
        if (found != local_id_by_node.end()) {
            local_path.push_back(found->second);
        }
    }

    const std::filesystem::path output_path(options.output_path);
    if (output_path.has_parent_path()) {
        std::filesystem::create_directories(output_path.parent_path());
    }

    std::ofstream out(options.output_path, std::ios::out | std::ios::trunc);
    if (!out) {
        throw std::runtime_error("Failed to open visualization output: " + options.output_path);
    }

    out << "{\n";
    out << "  \"meta\": {\n";
    out << "    \"source\": ";
    write_json_string(out, options.source_path);
    out << ",\n";
    out << "    \"totalNodes\": " << graph.num_nodes() << ",\n";
    out << "    \"totalEdges\": " << graph.num_edges() << ",\n";
    out << "    \"includedNodes\": " << included_order.size() << ",\n";
    out << "    \"includedEdges\": " << edges.size() << ",\n";
    out << "    \"requestedMaxNodes\": " << requested_cap << ",\n";
    out << "    \"neighborDepth\": " << depth_limit << ",\n";
    out << "    \"maxNeighborsPerNode\": " << neighbor_limit << ",\n";
    out << "    \"clusterAlgorithm\": ";
    write_json_string(out, community_algorithm);
    out << ",\n";
    out << "    \"clusterIterations\": " << community_options.max_iterations << ",\n";
    out << "    \"clusterResolution\": " << community_options.resolution << ",\n";
    out << "    \"detectedCommunities\": " << detected_communities << ",\n";
    out << "    \"start\": ";
    write_json_string(out, start == kInvalidNodeId ? "" : graph.title(start));
    out << ",\n";
    out << "    \"end\": ";
    write_json_string(out, end == kInvalidNodeId ? "" : graph.title(end));
    out << ",\n";
    out << "    \"pathFound\": " << (!path.empty() ? "true" : "false") << ",\n";
    out << "    \"pathLength\": " << local_path.size() << "\n";
    out << "  },\n";

    out << "  \"path\": [";
    for (std::size_t i = 0; i < local_path.size(); ++i) {
        if (i > 0) out << ", ";
        out << local_path[i];
    }
    out << "],\n";

    out << "  \"nodes\": [\n";
    for (std::size_t local_id = 0; local_id < included_order.size(); ++local_id) {
        const NodeId node = included_order[local_id];
        const auto node_idx = static_cast<std::size_t>(node);
        const std::size_t out_degree = graph.out_degree(node);
        const std::size_t visible = visible_out_degree[local_id];
        out << "    {\"id\": " << local_id
            << ", \"graphId\": " << node
            << ", \"title\": ";
        write_json_string(out, graph.title(node));
        out << ", \"outDegree\": " << out_degree
            << ", \"visibleOutDegree\": " << visible
            << ", \"externalOutDegree\": " << (out_degree >= visible ? out_degree - visible : 0)
            << ", \"cluster\": " << cluster_by_node[node_idx]
            << ", \"pathIndex\": " << path_index_by_node[node_idx]
            << "}";
        if (local_id + 1 < included_order.size()) out << ",";
        out << "\n";
    }
    out << "  ],\n";

    out << "  \"edges\": [\n";
    for (std::size_t i = 0; i < edges.size(); ++i) {
        const auto& edge = edges[i];
        out << "    {\"source\": " << edge.source
            << ", \"target\": " << edge.target
            << ", \"inPath\": " << (edge.in_path ? "true" : "false") << "}";
        if (i + 1 < edges.size()) out << ",";
        out << "\n";
    }
    out << "  ]\n";
    out << "}\n";
}

} // namespace Graph
