cmake_minimum_required(VERSION 3.16)
project(merge_matrices LANGUAGES CXX)

# ---- Options ---------------------------------------------------------------
option(ENABLE_OPENMP "Build with OpenMP parallelism" ON)
option(USE_LIBDEFLATE "Link against libdeflate instead of zlib" OFF)

# ---- Global compile features & flags ---------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add very aggressive Release flags *in addition* to CMake's defaults
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    message(STATUS "Using aggressive compiler optimisations")
    string(APPEND CMAKE_CXX_FLAGS_RELEASE
           " -O3 -march=native -fstrict-aliasing -DNDEBUG")
    # Try LTO if the toolchain supports it
    include(CheckIPOSupported)
    check_ipo_supported(RESULT ipo_ok LANGUAGES CXX)
    if(ipo_ok)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
    endif()
endif()

# Always compile with a few sanity warnings
add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:-Wall;-Wextra;-Wpedantic>")

# ---- Dependencies ----------------------------------------------------------
# zlib or libdeflate
if(USE_LIBDEFLATE)
    find_package(libdeflate REQUIRED CONFIG)   # assumes you have a CMake package ↓
    set(DEF_LIB libdeflate::libdeflate)
else()
    find_package(ZLIB REQUIRED)
    set(DEF_LIB ZLIB::ZLIB)
endif()

# Optional OpenMP
if(ENABLE_OPENMP)
    find_package(OpenMP)
    if(OpenMP_CXX_FOUND)
        set(OPENMP_LIB OpenMP::OpenMP_CXX)
        message(STATUS "OpenMP enabled")
    else()
        message(WARNING "OpenMP requested but not found – continuing without")
    endif()
endif()

# ---- Executable(s) ---------------------------------------------------------
# Add whichever source files you want – here we keep both the original
# and the new speed-tuned version so you can A/B test them.
add_executable(combine_chunklengths_opt combine_chunklengths_opt.cpp)
target_link_libraries(combine_chunklengths_opt PUBLIC ${DEF_LIB} ${OPENMP_LIB})

add_executable(merge_matrices combine_chunklengths.cpp)   # original
target_link_libraries(merge_matrices PUBLIC ${DEF_LIB} ${OPENMP_LIB})

