#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace tst
{

struct search_entry
{
  std::string_view identifier;
  std::string_view title;
  double confidence = 0.0;
};

struct search_result
{
  std::vector<search_entry> results;
};

class searcher
{
public:
  explicit searcher(const std::filesystem::path& path);
  ~searcher();

  searcher(searcher&&) = default;
  searcher(const searcher&) = delete;

  auto operator=(searcher&&) -> searcher& = default;
  auto operator=(const searcher&) = delete;

  auto search(std::string_view query) const -> search_result;

private:
  struct searcher_internals;

  std::unique_ptr<searcher_internals> m_internals;
};

}  // namespace tst
