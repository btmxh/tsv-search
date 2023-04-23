#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

#include <fmt/core.h>
#include <httplib.h>
#include <jsonrpccxx/server.hpp>
#include <nlohmann/json.hpp>

#include "lib.hpp"

namespace tst
{
using nlohmann::json;

inline void to_json(json& jsn, const search_entry& entry)
{
  jsn = json {
      {"identifier", entry.identifier},
      {"title", entry.title},
      {"confidence", entry.confidence},
  };
}

inline void from_json(const json& jsn, search_entry& entry)
{
  jsn.at("identifier").get_to(entry.identifier);
  jsn.at("title").get_to(entry.title);
  jsn.at("confidence").get_to(entry.confidence);
}

inline void to_json(json& jsn, const search_result& result)
{
  jsn = json {{"results", json {result.results}}};
}

inline void from_json(const json& jsn, search_result& result)
{
  jsn.at("results").get_to(result.results);
}
}  // namespace tst

auto main(int argc, char* argv[]) -> int
{
  if (argc < 4) {
    fmt::print("Usage: tsv-search INDEX_FILE_PATH HOST PORT\n");
    return 1;
  }

  // NOLINTNEXTLINE(*pointer-arithmetic*)
  const auto port = static_cast<int>(strtol(argv[3], nullptr, 10));

  // NOLINTNEXTLINE(*pointer-arithmetic*)
  const std::filesystem::path index_file_path {argv[1]};
  const tst::searcher searcher {index_file_path};
  jsonrpccxx::JsonRpc2Server server;
  httplib::Server http_server {};
  std::optional<std::thread> stop_thread;
  std::mutex stop_thread_mutex;
  http_server.Post("/jsonrpc",
                   [&](const httplib::Request& req, httplib::Response& res)
                   {
                     // NOLINTNEXTLINE(*magic-numbers*)
                     res.status = 200;
                     res.set_content(server.HandleRequest(req.body),
                                     "application/json");
                   });

  server.Add("search",
             jsonrpccxx::GetHandle(
                 std::function {[&](const std::string& query, size_t limit)
                                {
                                  auto result = searcher.search(query);
                                  if (result.results.size() > limit) {
                                    result.results.resize(limit);
                                  }
                                  return result;
                                }}));

  server.Add("exit",
             jsonrpccxx::GetHandle(std::function {
                 [&]
                 {
                   // we're probably doing single threaded so this kinda is
                   // unnecessary
                   const std::scoped_lock lock(stop_thread_mutex);
                   if (!stop_thread.has_value()) {
                     stop_thread.emplace([&] { http_server.stop(); });
                     return "requested server stop";
                   }

                   return "server is stopping";
                 }}));

  // NOLINTNEXTLINE(*pointer-arithmetic*)
  http_server.listen(argv[2], port);

  if(stop_thread.has_value()) {
    stop_thread.value().join();
  }
  return 0;
}

#if 0
auto main_test(int argc, char* argv[]) -> int
{
  auto start = std::chrono::high_resolution_clock::now();
  auto measure = [=]
  {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
               std::chrono::high_resolution_clock::now() - start)
               .count()
        * 1e-6;
  };
  const tst::searcher searcher {"../../../aod-extras/search.tsv"};
  auto t1 = measure();
  const auto result = searcher.search(argc > 1 ? argv[1] : "show by rock");
  auto t2 = measure();
  std::cout << result.results.size() << " results:\n";
  for (size_t i = 0; i < std::min<size_t>(result.results.size(), 10); ++i) {
    std::cout << result.results[i].identifier << " ("
              << result.results[i].confidence
              << ")"
                 "\n";
  }
  auto t3 = measure();
  std::cout << t1 << " " << t2 << " " << t3 << "\n";
  return 0;
}
#endif
