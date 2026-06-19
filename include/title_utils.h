#pragma once

#include <string>
#include <string_view>

namespace Graph {

std::string DecodeWikiTitle(std::string_view raw_title);
std::string NormalizeTitleKey(std::string_view title);

} // namespace Graph
