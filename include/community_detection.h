#pragma once

#include "digraph.h"

#include <cstddef>
#include <string>
#include <vector>

namespace Graph {

enum class CommunityAlgorithm {
    SeedExpansion,
    WeakComponents,
    LabelPropagation,
    Louvain
};

struct CommunityDetectionOptions {
    CommunityAlgorithm algorithm = CommunityAlgorithm::LabelPropagation;
    std::size_t max_iterations = 24;
    double resolution = 1.0;
};

CommunityAlgorithm ParseCommunityAlgorithm(const std::string& value);
std::string CommunityAlgorithmName(CommunityAlgorithm algorithm);

std::vector<int> DetectCommunities(const Digraph& graph,
                                   const std::vector<NodeId>& subset_nodes,
                                   const CommunityDetectionOptions& options);

} // namespace Graph
