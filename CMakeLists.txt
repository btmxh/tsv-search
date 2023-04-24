cmake_minimum_required(VERSION 3.14)

include(cmake/prelude.cmake)

project(
    tsv-search
    VERSION 0.1.0
    DESCRIPTION "tsv search thing for aod-extras"
    HOMEPAGE_URL "https://example.com/"
    LANGUAGES CXX
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

# Avoid warning about DOWNLOAD_EXTRACT_TIMESTAMP in CMake 3.24:
if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
  cmake_policy(SET CMP0135 NEW)
endif()

include(FetchContent)

set(FMT_HEADERS "")

# ---- Fetch dependencies ----
FetchContent_Declare(RapidFuzz
  GIT_REPOSITORY "https://github.com/maxbachmann/rapidfuzz-cpp.git"
  GIT_TAG         v1.11.3
  GIT_SHALLOW     1
)

FetchContent_Declare(mio
  GIT_REPOSITORY "https://github.com/mandreyel/mio.git"
  GIT_SHALLOW     1
)

FetchContent_Declare(json-rpc-cxx
  GIT_REPOSITORY "https://github.com/jsonrpcx/json-rpc-cxx"
  GIT_TAG         v0.3.1
  GIT_SHALLOW     1
)

FetchContent_Declare(json
  URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz
)

FetchContent_Declare(fmt
  URL https://github.com/fmtlib/fmt/releases/download/9.1.0/fmt-9.1.0.zip
)

FetchContent_Declare(cpp-httplib
  URL https://github.com/yhirose/cpp-httplib/archive/refs/tags/v0.12.2.zip
)

FetchContent_MakeAvailable(RapidFuzz)
FetchContent_MakeAvailable(mio)

option(COMPILE_TESTS OFF)
option(COMPILE_EXAMPLES OFF)
FetchContent_MakeAvailable(json-rpc-cxx)
FetchContent_MakeAvailable(json)
FetchContent_MakeAvailable(fmt)
FetchContent_MakeAvailable(cpp-httplib)

find_package(TBB REQUIRED)

function(target_link_libraries_system target)
  set(libs ${ARGN})
  foreach(lib ${libs})
    get_target_property(lib_include_dirs ${lib} INTERFACE_INCLUDE_DIRECTORIES)
    target_include_directories(${target} SYSTEM PRIVATE ${lib_include_dirs})
    target_link_libraries(${target} PRIVATE ${lib})
  endforeach(lib)
endfunction(target_link_libraries_system)

# ---- Declare library ----

add_library(
    tsv-search_lib OBJECT
    source/lib.cpp
)

target_include_directories(
    tsv-search_lib ${warning_guard}
    PUBLIC
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/source>"
)

target_compile_features(tsv-search_lib PUBLIC cxx_std_17)
target_link_libraries_system(tsv-search_lib rapidfuzz mio TBB::tbb)

if(${CMAKE_CXX_COMPILER_ID} STREQUAL "AppleClang")
  FetchContent_Declare(pstld
    URL https://github.com/mikekazakov/pstld/archive/refs/tags/0.1.0.tar.gz
  )

  option(BUILD_TESTING OFF)
  FetchContent_MakeAvailable(pstld)
  target_link_libraries_system(tsv-search_lib pstld)
endif()

# ---- Declare executable ----

add_executable(tsv-search_exe source/main.cpp)
add_executable(tsv-search::exe ALIAS tsv-search_exe)

set_property(TARGET tsv-search_exe PROPERTY OUTPUT_NAME tsv-search)

target_compile_features(tsv-search_exe PRIVATE cxx_std_17)

target_link_libraries_system(tsv-search_exe json-rpc-cxx nlohmann_json::nlohmann_json fmt::fmt httplib)
target_link_libraries(tsv-search_exe PRIVATE tsv-search_lib)

# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/install-rules.cmake)
endif()

# ---- Developer mode ----

if(NOT tsv-search_DEVELOPER_MODE)
  return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
  message(
      AUTHOR_WARNING
      "Developer mode is intended for developers of tsv-search"
  )
endif()

include(cmake/dev-mode.cmake)
