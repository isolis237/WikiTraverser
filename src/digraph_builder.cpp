#include "digraph_builder.h"
#include "title_utils.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Graph {
namespace {

void strip_trailing_cr(std::string& s) {
    if (!s.empty() && s.back() == '\r') s.pop_back();
}

std::vector<std::string> split_tabs(const std::string& line) {
    std::vector<std::string> cols;
    std::size_t start = 0;
    while (start <= line.size()) {
        const std::size_t end = line.find('\t', start);
        if (end == std::string::npos) {
            cols.push_back(line.substr(start));
            break;
        }
        cols.push_back(line.substr(start, end - start));
        start = end + 1;
    }
    return cols;
}

bool is_integer(const std::string& value) {
    if (value.empty()) return false;
    std::size_t i = value[0] == '-' ? 1 : 0;
    if (i == value.size()) return false;
    for (; i < value.size(); ++i) {
        if (!std::isdigit(static_cast<unsigned char>(value[i]))) return false;
    }
    return true;
}

struct ParsedEdge {
    std::string from_key;
    std::string from_title;
    std::string to_key;
    std::string to_title;
};

bool parse_tsv_edge(const std::string& line, ParsedEdge& edge) {
    if (line.empty() || line[0] == '#') return false;

    const auto cols = split_tabs(line);
    if (cols.size() >= 4 && is_integer(cols[0]) && is_integer(cols[2])) {
        edge.from_key = "id:" + cols[0];
        edge.from_title = DecodeWikiTitle(cols[1]);
        edge.to_key = "id:" + cols[2];
        edge.to_title = DecodeWikiTitle(cols[3]);
        return !edge.from_title.empty() && !edge.to_title.empty();
    }

    if (cols.size() == 2) {
        edge.from_title = DecodeWikiTitle(cols[0]);
        edge.to_title = DecodeWikiTitle(cols[1]);
        edge.from_key = "title:" + edge.from_title;
        edge.to_key = "title:" + edge.to_title;
        return !edge.from_title.empty() && !edge.to_title.empty();
    }

    return false;
}

template <typename T>
void write_pod(std::ofstream& out, const T& v) {
    out.write(reinterpret_cast<const char*>(&v), sizeof(T));
    if (!out) throw std::runtime_error("Failed writing cache (pod)");
}

template <typename T>
void read_pod(std::ifstream& in, T& v) {
    in.read(reinterpret_cast<char*>(&v), sizeof(T));
    if (!in) throw std::runtime_error("Failed reading cache (pod)");
}

void write_string(std::ofstream& out, const std::string& s) {
    std::uint32_t len = static_cast<std::uint32_t>(s.size());
    write_pod(out, len);
    if (len > 0) out.write(s.data(), len);
    if (!out) throw std::runtime_error("Failed writing cache (string)");
}

void read_string(std::ifstream& in, std::string& s) {
    std::uint32_t len = 0;
    read_pod(in, len);
    s.resize(len);
    if (len > 0) {
        in.read(&s[0], len);
        if (!in) throw std::runtime_error("Failed reading cache (string)");
    }
}

// Cache format:
// magic (8 bytes) + version (u32)
// num_nodes (u64), num_edges (u64)
// titles: num_nodes * (len+bytes)
// adjacency: for each node: out_degree (u32) then out_degree * (to (i32), weight (i32))
static constexpr const char* kMagic = "WIKIGRPH"; // 8 bytes
static constexpr std::uint32_t kVersion = 2;

std::string default_cache_path(const std::string& filepath) {
    return filepath + ".bin";
}

bool cache_is_fresh(const std::string& data_path, const std::string& cache_path) {
    namespace fs = std::filesystem;
    if (!fs::exists(cache_path)) return false;
    if (!fs::exists(data_path)) return false;
    return fs::last_write_time(cache_path) >= fs::last_write_time(data_path);
}

} // namespace

bool DigraphBuilder::TryLoadCache_(const std::string& cache_path, Digraph& out) {
    std::ifstream in(cache_path, std::ios::binary);
    if (!in) return false;

    char magic[8];
    in.read(magic, 8);
    if (!in) return false;
    if (std::string(magic, 8) != std::string(kMagic, 8)) return false;

    std::uint32_t version = 0;
    read_pod(in, version);
    if (version != kVersion) return false;

    std::uint64_t n64 = 0, m64 = 0;
    read_pod(in, n64);
    read_pod(in, m64);

    const std::size_t n = static_cast<std::size_t>(n64);
    const std::size_t m_expected = static_cast<std::size_t>(m64);

    Digraph tmp;
    tmp.titles_.resize(n);
    for (std::size_t i = 0; i < n; ++i) {
        read_string(in, tmp.titles_[i]);
    }

    tmp.adjacency_list_.resize(n);

    std::size_t edge_count_check = 0;
    for (std::size_t u = 0; u < n; ++u) {
        std::uint32_t deg = 0;
        read_pod(in, deg);
        tmp.adjacency_list_[u].resize(deg);

        for (std::uint32_t j = 0; j < deg; ++j) {
            std::int32_t to = 0;
            std::int32_t w = 1;
            read_pod(in, to);
            read_pod(in, w);
            tmp.adjacency_list_[u][j] = OutEdge{static_cast<NodeId>(to), static_cast<int>(w)};
        }

        edge_count_check += deg;
    }

    tmp.edge_count_ = edge_count_check;

    // sanity check (detect corrupt cache)
    if (tmp.edge_count_ != m_expected) return false;

    out = std::move(tmp);
    return true;
}

void DigraphBuilder::SaveCache_(const std::string& cache_path, const Digraph& g) {
    std::ofstream out(cache_path, std::ios::binary | std::ios::trunc);
    if (!out) throw std::runtime_error("Failed to open cache for write: " + cache_path);

    out.write(kMagic, 8);
    write_pod(out, kVersion);

    const std::uint64_t n = static_cast<std::uint64_t>(g.num_nodes());
    const std::uint64_t m = static_cast<std::uint64_t>(g.num_edges());
    write_pod(out, n);
    write_pod(out, m);

    for (const auto& t : g.titles_) {
        write_string(out, t);
    }

    for (const auto& vec : g.adjacency_list_) {
        std::uint32_t deg = static_cast<std::uint32_t>(vec.size());
        write_pod(out, deg);
        for (const auto& e : vec) {
            write_pod(out, static_cast<std::int32_t>(e.to));
            write_pod(out, static_cast<std::int32_t>(e.weight));
        }
    }

    if (!out) throw std::runtime_error("Failed while writing cache: " + cache_path);
}

Digraph DigraphBuilder::BuildFromTsv(const std::string& filepath,
                                    ProgressCallback on_progress,
                                    int progress_step_pct) {
    std::ifstream file(filepath, std::ios::in | std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open graph file: " + filepath);
    }

    file.seekg(0, std::ios::end);
    const std::streamoff file_size = file.tellg();
    file.seekg(0, std::ios::beg);
    if (file_size <= 0) return Digraph{};

    if (!on_progress) {
        std::cerr << "Building digraph from: " << filepath << "\n";
        on_progress = [&](int pct, std::size_t nodes, std::size_t edges) {
            std::cerr << pct << "%... (nodes=" << nodes << ", edges=" << edges << ")\n";
            std::cerr.flush();
        };
    }
    if (progress_step_pct <= 0) progress_step_pct = 10;

    std::unordered_map<std::string, NodeId> key_to_index;
    key_to_index.reserve(1 << 20);

    std::vector<std::string> titles;
    titles.reserve(1 << 20);

    auto get_or_create = [&](std::string key, std::string title) -> NodeId {
        auto it = key_to_index.find(key);
        if (it != key_to_index.end()) return it->second;

        NodeId idx = static_cast<NodeId>(titles.size());
        key_to_index.emplace(std::move(key), idx);
        titles.push_back(std::move(title));
        return idx;
    };

    std::vector<std::pair<NodeId, NodeId>> edges;
    edges.reserve(1 << 20);

    std::vector<std::size_t> out_degree;
    out_degree.reserve(1 << 20);

    auto ensure_degree_size = [&](std::size_t n) {
        if (out_degree.size() < n) out_degree.resize(n, 0);
    };

    auto process_line = [&](const std::string& ln) {
        ParsedEdge parsed;
        if (!parse_tsv_edge(ln, parsed)) return;

        NodeId u = get_or_create(std::move(parsed.from_key), std::move(parsed.from_title));
        NodeId v = get_or_create(std::move(parsed.to_key), std::move(parsed.to_title));

        ensure_degree_size(titles.size());
        edges.emplace_back(u, v);
        out_degree[static_cast<std::size_t>(u)] += 1;
    };

    std::string line;
    int next_report = progress_step_pct;

    while (std::getline(file, line)) {
        strip_trailing_cr(line);
        if (line.empty()) continue;

        process_line(line);

        const std::streamoff pos = file.tellg();
        if (pos > 0) {
            int pct = static_cast<int>((100.0 * pos) / file_size);
            pct = std::min(pct, 99);
            if (pct >= next_report) {
                on_progress(next_report, titles.size(), edges.size());
                next_report += progress_step_pct;
            }
        }
    }

    Digraph g;
    g.titles_ = std::move(titles);
    g.adjacency_list_.resize(g.titles_.size());
    g.edge_count_ = edges.size();

    ensure_degree_size(g.titles_.size());

    for (std::size_t u = 0; u < g.adjacency_list_.size(); ++u) {
        g.adjacency_list_[u].reserve(out_degree[u]);
    }

    for (const auto& [u, v] : edges) {
        g.adjacency_list_[static_cast<std::size_t>(u)].push_back(OutEdge{v, 1});
    }

    on_progress(100, g.num_nodes(), g.num_edges());
    return g;
}

Digraph DigraphBuilder::BuildFromTsvCached(const std::string& filepath,
                                          const std::string& cache_path_in,
                                          ProgressCallback on_progress,
                                          int progress_step_pct) {
    const std::string cache_path = cache_path_in.empty()
        ? default_cache_path(filepath)
        : cache_path_in;

    if (cache_is_fresh(filepath, cache_path)) {
        Digraph g;
        if (TryLoadCache_(cache_path, g)) {
            if (on_progress) on_progress(100, g.num_nodes(), g.num_edges());
            return g;
        }
        // fall through if cache invalid/corrupt
    }

    Digraph g = BuildFromTsv(filepath, on_progress, progress_step_pct);

    try {
        SaveCache_(cache_path, g);
    } catch (const std::exception& ex) {
        std::cerr << "Warning: failed to write cache '" << cache_path
                  << "': " << ex.what() << "\n";
    }

    return g;
}

} // namespace Graph
