cmake_minimum_required(VERSION 3.22)

project(parallel_compute
VERSION 0.0.1
LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wextra -Wpedantic -O2)

add_executable(pc_example examples/main.cpp)

add_library(parallel_compute STATIC
    src/thread_pool.cpp
)

target_include_directories(parallel_compute PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(pc_example PRIVATE parallel_compute)