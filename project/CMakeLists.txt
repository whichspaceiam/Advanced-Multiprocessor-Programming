cmake_minimum_required(VERSION 3.18)
project(fifo LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build" FORCE)
endif()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic")

find_package(OpenMP REQUIRED)


include_directories(include)

add_library(project_includes INTERFACE)
target_include_directories(project_includes INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(main
src/main.cpp
)

# target_include_directories(main PRIVATE project_includes)
target_link_libraries(main PRIVATE project_includes OpenMP::OpenMP_CXX)

enable_testing()
add_subdirectory(tests)