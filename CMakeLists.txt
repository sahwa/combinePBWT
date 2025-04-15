cmake_minimum_required(VERSION 3.10)
project(merge_matrices)

# Use C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Set build type to Release by default if not specified
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
endif()

# Aggressive compiler optimizations for Release
if(CMAKE_BUILD_TYPE STREQUAL "Release")
  message(STATUS "Using aggressive compiler optimizations")
  set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -flto -fstrict-aliasing -DNDEBUG")
endif()

# Find zlib
find_package(ZLIB REQUIRED)

# Add the executable
add_executable(merge_matrices combine_chunklengths.cpp)

# Link zlib
target_link_libraries(merge_matrices ZLIB::ZLIB)
