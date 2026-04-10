# stock-poller-coro CMakeLists.txt
# ==============================================================================
# Modern CMake setup for libcoro-based async stock/option data poller.
#
# LEARNING POINTS:
# - CMake 3.22+ required for proper C++23 support and FetchContent improvements
# - C++23 enables std::expected, std::mdspan, std::generator (where available)
# - libcoro requires LIBCORO_FEATURE_NETWORKING for coro::net::tcp components
# - Low-latency builds: -O3 -march=native -flto for maximum performance
# ==============================================================================

cmake_minimum_required(VERSION 3.22)

project(stock-poller-coro
    VERSION 1.0.0
    DESCRIPTION "Educational C++20/23 coroutine-based stock poller"
    LANGUAGES CXX
)

# ------------------------------------------------------------------------------
# C++23 Standard Configuration
# LEARNING: C++23 brings std::expected (error handling), std::generator,
# and improved constexpr. Essential for modern coroutine-based systems.
# ------------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)  # Prefer -std=c++23 over -std=gnu++23

# ------------------------------------------------------------------------------
# Release Build Flags for Low-Latency Trading Systems
# LEARNING: These flags are critical for HFT/low-latency systems:
# - -O3: Maximum inlining and optimization
# - -march=native: CPU-specific instructions (AVX2, AVX-512 where available)
# - -flto: Link-time optimization for whole-program analysis
# - -DNDEBUG: Disable assertions in release (careful: removes bounds checks)
# ------------------------------------------------------------------------------
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -flto -DNDEBUG")
set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)

# ------------------------------------------------------------------------------
# FetchContent: Download libcoro from GitHub
# LEARNING: FetchContent > find_package for learning projects:
# - No manual installation required
# - Always get latest stable API
# - Full source visibility for debugging/learning
# ------------------------------------------------------------------------------
include(FetchContent)

FetchContent_Declare(
    libcoro
    GIT_REPOSITORY https://github.com/jbaldwin/libcoro.git
    GIT_TAG        main  # Use latest; pin to release tag for production
    GIT_SHALLOW    TRUE  # Faster clone, only fetches latest commit
)

# ------------------------------------------------------------------------------
# Enable Networking Support in libcoro
# LEARNING: libcoro uses CMake options to enable optional features.
# LIBCORO_FEATURE_NETWORKING is REQUIRED for:
# - coro::net::tcp::server
# - coro::net::tcp::client
# - All I/O scheduling and epoll/kqueue backends
# ------------------------------------------------------------------------------
set(LIBCORO_FEATURE_NETWORKING ON CACHE BOOL "Enable TCP/UDP networking" FORCE)

FetchContent_MakeAvailable(libcoro)

# ------------------------------------------------------------------------------
# Main Executable
# ------------------------------------------------------------------------------
add_executable(stock_poller
    main.cpp
)

# ------------------------------------------------------------------------------
# Link Against libcoro
# LEARNING: Modern CMake uses target-based linking (not global variables).
# libcoro propagates include dirs, compile flags, and dependencies.
# NOTE: The target name is 'libcoro' (not 'libcoro::libcoro') from FetchContent.
# ------------------------------------------------------------------------------
target_link_libraries(stock_poller PRIVATE
    libcoro
)

# ------------------------------------------------------------------------------
# Compiler Warnings for Safety-Critical Systems Code
# LEARNING: C++ systems programming demands aggressive warnings:
# - Wall/Wextra: Standard comprehensive warnings
# - Werror: Treat warnings as errors (prevents UB sneaking in)
# - Pedantic: Strict standard conformance
# - Specific flags catch common coroutine pitfalls:
#   - Wdangling-reference: Catches lifetime issues in coroutines
#   - Wunreachable-code: Detects dead coroutine paths
# ------------------------------------------------------------------------------
target_compile_options(stock_poller PRIVATE
    $<$<CXX_COMPILER_ID:Clang,AppleClang,GCC>:
        -Wall -Wextra -Wpedantic
        -Werror=unreachable-code
    >
    $<$<CXX_COMPILER_ID:GNU>:
        -Wdangling-reference  # GCC 13+: critical for coroutine lifetime safety
    >
    $<$<CXX_COMPILER_ID:MSVC>:
        /W4 /WX
    >
)

# ------------------------------------------------------------------------------
# Future: Sanitizers for Debugging (Commented - add manually for learning)
# LEARNING: Sanitizers are essential for systems programming education:
# - AddressSanitizer (ASan): Detects memory errors, leaks, buffer overflows
# - ThreadSanitizer (TSan): Detects data races in concurrent code
# - UndefinedBehaviorSanitizer (UBSan): Detects UB (shifts, null derefs)
# Usage: cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZERS=ON ..
# ------------------------------------------------------------------------------
option(ENABLE_SANITIZERS "Enable sanitizers for debug builds" OFF)
if(ENABLE_SANITIZERS AND CMAKE_BUILD_TYPE MATCHES Debug)
    target_compile_options(stock_poller PRIVATE
        -fsanitize=address,thread,undefined
        -fno-omit-frame-pointer
    )
    target_link_options(stock_poller PRIVATE
        -fsanitize=address,thread,undefined
    )
endif()