#include "title_utils.h"

#include <cctype>

namespace Graph {
namespace {

int hex_value(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
    return -1;
}

void trim_in_place(std::string& value) {
    std::size_t first = 0;
    while (first < value.size() &&
           std::isspace(static_cast<unsigned char>(value[first]))) {
        ++first;
    }

    std::size_t last = value.size();
    while (last > first &&
           std::isspace(static_cast<unsigned char>(value[last - 1]))) {
        --last;
    }

    if (first > 0 || last < value.size()) {
        value = value.substr(first, last - first);
    }
}

} // namespace

std::string DecodeWikiTitle(std::string_view raw_title) {
    std::string decoded;
    decoded.reserve(raw_title.size());

    for (std::size_t i = 0; i < raw_title.size(); ++i) {
        const char c = raw_title[i];
        if (c == '%' && i + 2 < raw_title.size()) {
            const int hi = hex_value(raw_title[i + 1]);
            const int lo = hex_value(raw_title[i + 2]);
            if (hi >= 0 && lo >= 0) {
                decoded.push_back(static_cast<char>((hi << 4) | lo));
                i += 2;
                continue;
            }
        }

        decoded.push_back(c == '_' ? ' ' : c);
    }

    trim_in_place(decoded);
    return decoded;
}

std::string NormalizeTitleKey(std::string_view title) {
    const std::string decoded = DecodeWikiTitle(title);
    std::string normalized;
    normalized.reserve(decoded.size());

    bool last_was_space = false;
    for (unsigned char c : decoded) {
        if (std::isspace(c)) {
            if (!last_was_space && !normalized.empty()) {
                normalized.push_back(' ');
            }
            last_was_space = true;
            continue;
        }

        if (c >= 'A' && c <= 'Z') {
            normalized.push_back(static_cast<char>(c - 'A' + 'a'));
        } else {
            normalized.push_back(static_cast<char>(c));
        }
        last_was_space = false;
    }

    if (!normalized.empty() && normalized.back() == ' ') {
        normalized.pop_back();
    }
    return normalized;
}

} // namespace Graph
