cmake_minimum_required(VERSION 3.15)
project(log-analyzer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Optimizations for high-performance parsing
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(-O3 -march=native)
endif()

# Add executable
add_executable(log-analyzer
    apps/main.cpp
    src/analytics/log_aggregator.cpp
    src/analytics/thread_safe_aggregator.cpp
    src/parser/log_parser.cpp
    src/utils/memory_pool.cpp
    src/utils/streaming_reader.cpp
    src/utils/thread_safe_queue.cpp
)

# Include directories
target_include_directories(log-analyzer PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# Explicitly add C++ standard library path for macOS
# Some macOS SDK configurations don't auto-detect libc++ headers
if(APPLE AND CMAKE_OSX_SYSROOT)
    target_include_directories(log-analyzer PRIVATE
        ${CMAKE_OSX_SYSROOT}/usr/include/c++/v1
    )
endif()

# Link standard libraries
target_link_libraries(log-analyzer PRIVATE pthread)

# TODO: Add GoogleTest for tests (Part 4)
