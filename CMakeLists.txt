cmake_minimum_required(VERSION 3.18)
project(combine_chunklengths LANGUAGES CXX)

# ---------------------------------------------------------------------------
# Options
# ---------------------------------------------------------------------------
option(ENABLE_OPENMP   "Build with OpenMP support"        OFF)
option(USE_LIBDEFLATE  "Use libdeflate instead of zlib"   OFF)
option(ENABLE_LTO      "Enable link-time optimisation"    ON)

# ---------------------------------------------------------------------------
# Build type
# ---------------------------------------------------------------------------
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# ---------------------------------------------------------------------------
# Toolchain-agnostic compiler settings
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Core warnings
add_compile_options(-Wall -Wextra -Wpedantic -Wconversion)

# Optimisation flags for Release variants
if(CMAKE_BUILD_TYPE MATCHES "^Release$" OR CMAKE_BUILD_TYPE MATCHES "^RelWithDebInfo$")
    # Enable aggressive optimisation, strict aliasing, native tuning, and C++ exceptions
    add_compile_options(-O3 -march=native -fstrict-aliasing -fexceptions -fno-rtti)
endif()

# ---------------------------------------------------------------------------
# LTO / IPO
# ---------------------------------------------------------------------------
include(CheckIPOSupported)
if(ENABLE_LTO)
    check_ipo_supported(RESULT ipo_ok LANGUAGES CXX)
    if(ipo_ok)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
        message(STATUS "LTO enabled")
    else()
        message(STATUS "LTO requested but unsupported – skipped")
    endif()
endif()

# ---------------------------------------------------------------------------
# Dependencies – decompression library
# ---------------------------------------------------------------------------
if(USE_LIBDEFLATE)
    find_package(libdeflate CONFIG REQUIRED)
    set(DEFLATE_LIB libdeflate::libdeflate)
else()
    find_package(ZLIB REQUIRED)
    set(DEFLATE_LIB ZLIB::ZLIB)
endif()

# ---------------------------------------------------------------------------
# Optional OpenMP
# ---------------------------------------------------------------------------
set(OPENMP_LIB "")
if(ENABLE_OPENMP)
    find_package(OpenMP)
    if(OpenMP_CXX_FOUND)
        set(OPENMP_LIB OpenMP::OpenMP_CXX)
        message(STATUS "OpenMP enabled")
    else()
        message(WARNING "OpenMP requested but not found – continuing without")
    endif()
endif()

# ---------------------------------------------------------------------------
# Output directories – executables now land in <source>/bin
# ---------------------------------------------------------------------------
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
file(MAKE_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

# ---------------------------------------------------------------------------
# Single (verbose) target
# ---------------------------------------------------------------------------
add_executable(combine_chunklengths combine_chunklengths.cpp)

# Silence cosmetic warnings seen on GCC
target_compile_options(combine_chunklengths PRIVATE -Wno-comment -Wno-conversion)

# Link dependencies
target_link_libraries(combine_chunklengths PRIVATE ${DEFLATE_LIB} ${OPENMP_LIB})

# Definitions for optional features
if(ENABLE_OPENMP)
    target_compile_definitions(combine_chunklengths PRIVATE ENABLE_OPENMP)
endif()
if(USE_LIBDEFLATE)
    target_compile_definitions(combine_chunklengths PRIVATE USE_LIBDEFLATE)
endif()

# ---------------------------------------------------------------------------
# Misc tooling
# ---------------------------------------------------------------------------
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---------------------------------------------------------------------------
# Summary
# ---------------------------------------------------------------------------
message(STATUS "============= Build configuration summary =============")
message(STATUS "  Build type          : ${CMAKE_BUILD_TYPE}")
message(STATUS "  LTO enabled         : ${CMAKE_INTERPROCEDURAL_OPTIMIZATION}")
message(STATUS "  Decompressor lib    : ${DEFLATE_LIB}")
message(STATUS "  OpenMP enabled      : ${ENABLE_OPENMP}")
message(STATUS "  Binaries output dir : ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "========================================================")

