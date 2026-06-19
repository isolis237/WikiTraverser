#pragma once

#include "digraph.h"

#include <functional>
#include <string>

namespace Graph {

/**
 * @brief Builds a Digraph from a TSV edge list.
 *
 * Input format (tab-separated):
 *   page_id_from    page_title_from    page_id_to    page_title_to
 *   10              AccessibleComputing 411964        Computer accessibility
 *   ...
 *
 * The builder:
 * - Remaps external IDs -> dense NodeId
 * - Stores one title per node (first observed)
 * - Builds adjacency list with pre-reservation (two-pass)
 */
class DigraphBuilder {
public:
    using ProgressCallback = std::function<void(int pct, std::size_t nodes, std::size_t edges)>;

    static Digraph BuildFromTsv(const std::string& filepath,
                                ProgressCallback on_progress = nullptr,
                                int progress_step_pct = 10);

    static Digraph BuildFromTsvCached(const std::string& filepath,
                                      const std::string& cache_path = "",
                                      ProgressCallback on_progress = nullptr,
                                      int progress_step_pct = 10);

private:
    static bool TryLoadCache_(const std::string& cache_path, Digraph& out);
    static void SaveCache_(const std::string& cache_path, const Digraph& g);
};

} // namespace Graph
