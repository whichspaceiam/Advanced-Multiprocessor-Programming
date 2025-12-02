cmake_minimum_required(VERSION 3.18)
project(fifo LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 14)

include_directories(include)
add_library(project_includes INTERFACE)
target_include_directories(project_includes INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(main
src/main.cpp
)

target_include_directories(main PRIVATE project_includes)


enable_testing()
add_subdirectory(tests)