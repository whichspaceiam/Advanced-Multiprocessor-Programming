cmake_minimum_required(VERSION 3.18)
project(fifo LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build" FORCE)
endif()

find_package(OpenMP REQUIRED)


include_directories(include)

add_library(core STATIC
src/sequential.cpp
src/global_lock.cpp
src/fine_lock.cpp
src/atomic.cpp
src/basic_structures.cpp
)
target_include_directories(core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)



enable_testing()
add_subdirectory(tests)
add_subdirectory(benchmarks)