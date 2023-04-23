# Parent project does not export its library target, so this CML implicitly
# depends on being added from it, i.e. the testing is done only from the build
# tree and is not feasible from an install location

project(tsv-searchTests LANGUAGES CXX)

# ---- Tests ----

add_executable(tsv-search_test source/tsv-search_test.cpp)
target_link_libraries(tsv-search_test PRIVATE tsv-search_lib)
target_compile_features(tsv-search_test PRIVATE cxx_std_17)

add_test(NAME tsv-search_test COMMAND tsv-search_test)

# ---- End-of-file commands ----

add_folders(Test)
