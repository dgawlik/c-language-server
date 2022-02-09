cmake_minimum_required(VERSION 3.16)
project(c-language-server)

set(THREADS_PREFER_PTHREAD_FLAG ON)


include_directories(
    lib/include
    deps/google-test/include
    deps/tree-sitter/include
)

find_library(GTEST gtest HINTS deps/google-test/lib)
find_library(GTEST_MAIN gtest_main HINTS deps/google-test/lib)
find_library(TREE_SITTER tree-sitter HINTS deps/tree-sitter/lib)
find_package(Threads REQUIRED)


add_executable(c_language_server app/main.cpp deps/tree-sitter-c/parser.c lib/src/stack-graph-tree.cpp lib/src/stack-graph-engine.cpp)
add_executable(tst tests/syntax-tree-test.cpp tests/engine-test.cpp deps/tree-sitter-c/parser.c lib/src/stack-graph-tree.cpp lib/src/stack-graph-engine.cpp)

set_target_properties(c_language_server PROPERTIES CXX_STANDARD 17)
set_target_properties(tst PROPERTIES CXX_STANDARD 17)

target_link_libraries(c_language_server ${TREE_SITTER})
target_link_libraries(tst ${TREE_SITTER}  Threads::Threads ${GTEST} ${GTEST_MAIN} )

