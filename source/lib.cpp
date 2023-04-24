#include <algorithm>
#include <cstring>
#include <iterator>
#include <mutex>
#include <unordered_map>

#include "lib.hpp"

#include <mio/mmap.hpp>
#include <rapidfuzz/fuzz.hpp>

// apple clang doesn't support parallel execution
// we need to use an external library
#ifdef __APPLE__
#  define PSTLD_HACK_INTO_STD
#  include "pstld.h"
#else
#  include <execution>
#endif

namespace tst
{

struct entry_name
{
  std::string_view m_titles;
  std::vector<size_t> m_delimiters;

  explicit entry_name(std::string_view titles)
      : m_titles(titles)
  {
    size_t last = 0;
    do {
      const auto delimiter = titles.find('\t');
      if (delimiter == std::string::npos) {
        break;
      }

      titles = titles.substr(delimiter + 1);
      m_delimiters.push_back(last = last + delimiter);
      ++last;
    } while (!titles.empty());
    m_delimiters.push_back(m_titles.size());
  }
};

struct searcher::searcher_internals
{
  mio::mmap_source m_source;
  std::unordered_map<std::string_view, entry_name> m_search_data;
};

searcher::searcher(const std::filesystem::path& path)
    : m_internals(std::make_unique<searcher_internals>())
{
  m_internals->m_source = mio::mmap_source {path.string()};
  // const auto* const data = m_internals->m_source.data();
  const auto size = m_internals->m_source.size();
  std::string_view content {m_internals->m_source.begin(), size};
  while (!content.empty()) {
    const auto tab = content.find('\t');
    if (tab >= content.size() - 1) {
      break;
    }

    const auto key = content.substr(0, tab);
    content = content.substr(tab + 1);
    const auto newline = content.find('\n');
    if (newline >= content.size()) {
      break;
    }

    const auto value = content.substr(0, newline);

    if (newline + 1 < content.size()) {
      content = content.substr(newline + 1);
    }

    m_internals->m_search_data.insert({key, entry_name {value}});
  }
}

searcher::~searcher() = default;

auto searcher::search(std::string_view query) const -> search_result
{
  const auto confidence_threshold = 0.5;
  search_result result {};
  std::mutex result_mutex {};
  std::for_each(std::execution::par_unseq,
                m_internals->m_search_data.begin(),
                m_internals->m_search_data.end(),
                [&](const auto& pair)
                {
                  const auto& [key, value] = pair;
                  auto confidence = 0.0;
                  size_t offset = 0;
                  for (const auto delim_index : value.m_delimiters) {
                    const std::string_view name =
                        value.m_titles.substr(offset, delim_index);
                    confidence =
                        std::max(confidence,
                                 // NOLINTNEXTLINE(*magic-numbers*)
                                 rapidfuzz::fuzz::ratio(query, name) * 1e-2);

                    offset = delim_index + 1;
                  }

                  if (confidence >= confidence_threshold) {
                    const std::scoped_lock lock {result_mutex};
                    result.results.push_back(search_entry {
                        key,
                        value.m_titles.substr(0, value.m_delimiters[0]),
                        confidence});
                  }
                });
  std::sort(std::execution::par_unseq,
            result.results.begin(),
            result.results.end(),
            [](const auto& lhs, const auto& rhs)
            { return lhs.confidence > rhs.confidence; });
  return result;
}
}  // namespace tst
