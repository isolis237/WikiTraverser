#include "community_detection.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <limits>
#include <queue>
#include <stdexcept>
#include <unordered_map>
#include <utility>

namespace Graph {
namespace {

struct WeightedNeighbor {
    std::size_t to;
    double weight;
};

struct LocalGraph {
    std::vector<std::vector<WeightedNeighbor>> adjacency;
    std::vector<double> degree;
    double total_edge_weight = 0.0;
};

std::string normalize_algorithm_name(std::string value) {
    std::string normalized;
    normalized.reserve(value.size());
    for (unsigned char c : value) {
        if (c == '_' || c == ' ') {
            normalized.push_back('-');
        } else {
            normalized.push_back(static_cast<char>(std::tolower(c)));
        }
    }
    return normalized;
}

LocalGraph build_local_undirected_graph(const Digraph& graph,
                                        const std::vector<NodeId>& subset_nodes) {
    LocalGraph local;
    local.adjacency.resize(subset_nodes.size());
    local.degree.assign(subset_nodes.size(), 0.0);

    std::unordered_map<NodeId, std::size_t> local_id_by_node;
    local_id_by_node.reserve(subset_nodes.size());
    for (std::size_t i = 0; i < subset_nodes.size(); ++i) {
        local_id_by_node.emplace(subset_nodes[i], i);
    }

    for (std::size_t local_source = 0; local_source < subset_nodes.size(); ++local_source) {
        const NodeId source = subset_nodes[local_source];
        for (const auto& edge : graph.out_edges(source)) {
            const auto found = local_id_by_node.find(edge.to);
            if (found == local_id_by_node.end()) continue;

            const std::size_t local_target = found->second;
            if (local_target == local_source) continue;

            const double weight = std::max(1, edge.weight);
            local.adjacency[local_source].push_back(WeightedNeighbor{local_target, weight});
            local.adjacency[local_target].push_back(WeightedNeighbor{local_source, weight});
            local.degree[local_source] += weight;
            local.degree[local_target] += weight;
            local.total_edge_weight += weight;
        }
    }

    return local;
}

std::vector<int> compact_labels(const std::vector<std::size_t>& labels,
                                const std::vector<double>& degree) {
    struct Summary {
        std::size_t label;
        std::size_t size = 0;
        double degree = 0.0;
    };

    std::unordered_map<std::size_t, Summary> by_label;
    by_label.reserve(labels.size());
    for (std::size_t i = 0; i < labels.size(); ++i) {
        auto& summary = by_label[labels[i]];
        summary.label = labels[i];
        summary.size += 1;
        if (i < degree.size()) summary.degree += degree[i];
    }

    std::vector<Summary> summaries;
    summaries.reserve(by_label.size());
    for (const auto& entry : by_label) {
        summaries.push_back(entry.second);
    }

    std::sort(summaries.begin(), summaries.end(), [](const Summary& a, const Summary& b) {
        if (a.size != b.size) return a.size > b.size;
        if (std::fabs(a.degree - b.degree) > 1e-9) return a.degree > b.degree;
        return a.label < b.label;
    });

    std::unordered_map<std::size_t, int> compact_by_label;
    compact_by_label.reserve(summaries.size());
    for (std::size_t i = 0; i < summaries.size(); ++i) {
        compact_by_label.emplace(summaries[i].label, static_cast<int>(i));
    }

    std::vector<int> compacted(labels.size(), 0);
    for (std::size_t i = 0; i < labels.size(); ++i) {
        compacted[i] = compact_by_label[labels[i]];
    }
    return compacted;
}

std::vector<int> detect_weak_components(const LocalGraph& local) {
    const std::size_t n = local.adjacency.size();
    const std::size_t unassigned = std::numeric_limits<std::size_t>::max();
    std::vector<std::size_t> labels(n, unassigned);
    std::queue<std::size_t> frontier;

    for (std::size_t start = 0; start < n; ++start) {
        if (labels[start] != unassigned) continue;

        labels[start] = start;
        frontier.push(start);

        while (!frontier.empty()) {
            const std::size_t current = frontier.front();
            frontier.pop();

            for (const auto& neighbor : local.adjacency[current]) {
                if (labels[neighbor.to] != unassigned) continue;
                labels[neighbor.to] = start;
                frontier.push(neighbor.to);
            }
        }
    }

    return compact_labels(labels, local.degree);
}

std::vector<int> detect_label_propagation(const LocalGraph& local,
                                          std::size_t max_iterations) {
    const std::size_t n = local.adjacency.size();
    std::vector<std::size_t> labels(n);
    std::vector<std::size_t> label_size(n, 1);
    std::vector<std::size_t> order(n);
    for (std::size_t i = 0; i < n; ++i) {
        labels[i] = i;
        order[i] = i;
    }

    std::sort(order.begin(), order.end(), [&](std::size_t a, std::size_t b) {
        if (std::fabs(local.degree[a] - local.degree[b]) > 1e-9) {
            return local.degree[a] > local.degree[b];
        }
        return a < b;
    });

    max_iterations = std::max<std::size_t>(1, max_iterations);
    for (std::size_t iteration = 0; iteration < max_iterations; ++iteration) {
        std::size_t changes = 0;

        for (std::size_t node : order) {
            if (local.adjacency[node].empty()) continue;

            std::unordered_map<std::size_t, double> votes;
            votes.reserve(local.adjacency[node].size() + 1);
            votes[labels[node]] += 0.75; // inertia keeps sparse bridges from collapsing every component.

            for (const auto& neighbor : local.adjacency[node]) {
                votes[labels[neighbor.to]] += neighbor.weight;
            }

            std::size_t best_label = labels[node];
            auto balanced_score = [&](std::size_t label, double raw_vote) {
                const double size_penalty = std::pow(
                    static_cast<double>(std::max<std::size_t>(1, label_size[label])),
                    0.58
                );
                return raw_vote / size_penalty;
            };

            double best_score = balanced_score(best_label, votes[best_label]);
            for (const auto& vote : votes) {
                const std::size_t candidate = vote.first;
                const double score = balanced_score(candidate, vote.second);
                if (score > best_score + 1e-9 ||
                    (std::fabs(score - best_score) <= 1e-9 &&
                     candidate == labels[node]) ||
                    (std::fabs(score - best_score) <= 1e-9 &&
                     best_label != labels[node] &&
                     candidate < best_label)) {
                    best_label = candidate;
                    best_score = score;
                }
            }

            if (best_label != labels[node]) {
                label_size[labels[node]] -= 1;
                label_size[best_label] += 1;
                labels[node] = best_label;
                ++changes;
            }
        }

        if (changes == 0) break;
    }

    return compact_labels(labels, local.degree);
}

std::vector<int> detect_louvain_local_moving(const LocalGraph& local,
                                             std::size_t max_iterations,
                                             double resolution) {
    const std::size_t n = local.adjacency.size();
    std::vector<std::size_t> communities(n);
    std::vector<double> community_degree = local.degree;
    std::vector<std::size_t> order(n);
    for (std::size_t i = 0; i < n; ++i) {
        communities[i] = i;
        order[i] = i;
    }

    if (local.total_edge_weight <= 0.0) {
        return compact_labels(communities, local.degree);
    }

    std::sort(order.begin(), order.end(), [&](std::size_t a, std::size_t b) {
        if (std::fabs(local.degree[a] - local.degree[b]) > 1e-9) {
            return local.degree[a] > local.degree[b];
        }
        return a < b;
    });

    const double two_m = 2.0 * local.total_edge_weight;
    resolution = std::max(0.05, resolution);
    max_iterations = std::max<std::size_t>(1, max_iterations);

    for (std::size_t iteration = 0; iteration < max_iterations; ++iteration) {
        std::size_t moves = 0;

        for (std::size_t node : order) {
            if (local.degree[node] <= 0.0) continue;

            std::unordered_map<std::size_t, double> weight_by_community;
            weight_by_community.reserve(local.adjacency[node].size() + 1);
            for (const auto& neighbor : local.adjacency[node]) {
                weight_by_community[communities[neighbor.to]] += neighbor.weight;
            }

            const std::size_t current = communities[node];
            community_degree[current] -= local.degree[node];

            auto gain_for = [&](std::size_t community) {
                const auto found = weight_by_community.find(community);
                const double edge_weight = found == weight_by_community.end() ? 0.0 : found->second;
                return edge_weight - (resolution * local.degree[node] * community_degree[community] / two_m);
            };

            std::size_t best = current;
            double best_gain = gain_for(current);

            for (const auto& entry : weight_by_community) {
                const std::size_t candidate = entry.first;
                const double gain = gain_for(candidate);
                if (gain > best_gain + 1e-9 ||
                    (std::fabs(gain - best_gain) <= 1e-9 && candidate == current) ||
                    (std::fabs(gain - best_gain) <= 1e-9 &&
                     best != current &&
                     candidate < best)) {
                    best = candidate;
                    best_gain = gain;
                }
            }

            communities[node] = best;
            community_degree[best] += local.degree[node];
            if (best != current) ++moves;
        }

        if (moves == 0) break;
    }

    return compact_labels(communities, local.degree);
}

} // namespace

CommunityAlgorithm ParseCommunityAlgorithm(const std::string& value) {
    const std::string normalized = normalize_algorithm_name(value);

    if (normalized.empty() ||
        normalized == "label" ||
        normalized == "label-propagation" ||
        normalized == "lp") {
        return CommunityAlgorithm::LabelPropagation;
    }
    if (normalized == "louvain" ||
        normalized == "modularity" ||
        normalized == "modularity-local") {
        return CommunityAlgorithm::Louvain;
    }
    if (normalized == "components" ||
        normalized == "weak-components" ||
        normalized == "wcc") {
        return CommunityAlgorithm::WeakComponents;
    }
    if (normalized == "seed" ||
        normalized == "seed-expansion" ||
        normalized == "expansion") {
        return CommunityAlgorithm::SeedExpansion;
    }

    throw std::runtime_error("Unknown clustering algorithm: " + value);
}

std::string CommunityAlgorithmName(CommunityAlgorithm algorithm) {
    switch (algorithm) {
        case CommunityAlgorithm::SeedExpansion: return "seed-expansion";
        case CommunityAlgorithm::WeakComponents: return "weak-components";
        case CommunityAlgorithm::LabelPropagation: return "label-propagation";
        case CommunityAlgorithm::Louvain: return "louvain";
    }
    return "label-propagation";
}

std::vector<int> DetectCommunities(const Digraph& graph,
                                   const std::vector<NodeId>& subset_nodes,
                                   const CommunityDetectionOptions& options) {
    if (subset_nodes.empty()) return {};

    if (options.algorithm == CommunityAlgorithm::SeedExpansion) {
        std::vector<std::size_t> labels(subset_nodes.size());
        for (std::size_t i = 0; i < labels.size(); ++i) labels[i] = i;
        return compact_labels(labels, std::vector<double>(labels.size(), 0.0));
    }

    const LocalGraph local = build_local_undirected_graph(graph, subset_nodes);

    switch (options.algorithm) {
        case CommunityAlgorithm::WeakComponents:
            return detect_weak_components(local);
        case CommunityAlgorithm::Louvain:
            return detect_louvain_local_moving(local, options.max_iterations, options.resolution);
        case CommunityAlgorithm::LabelPropagation:
            return detect_label_propagation(local, options.max_iterations);
        case CommunityAlgorithm::SeedExpansion:
            break;
    }

    return detect_label_propagation(local, options.max_iterations);
}

} // namespace Graph
