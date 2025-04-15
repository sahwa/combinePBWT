cmake_minimum_required(VERSION 3.10)
project(merge_matrices)

# Use C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Find zlib
find_package(ZLIB REQUIRED)

# Add the executable
add_executable(merge_matrices combine_chunklengths.cpp)

# Link zlib
target_link_libraries(merge_matrices ZLIB::ZLIB)
