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

# GoogleTest setup (Part 4.1)
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Set C++ standard library path before making googletest available
if(APPLE AND CMAKE_OSX_SYSROOT)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I${CMAKE_OSX_SYSROOT}/usr/include/c++/v1")
endif()

FetchContent_MakeAvailable(googletest)

# Enable testing
enable_testing()

# Test executable
add_executable(log-analyzer-tests
    tests/test_main.cpp
    tests/test_parser.cpp
    tests/test_streaming_reader.cpp
    tests/test_histogram.cpp
    tests/test_thread_safe_queue.cpp
    src/parser/log_parser.cpp
    src/utils/streaming_reader.cpp
    src/utils/thread_safe_queue.cpp
    src/analytics/log_aggregator.cpp
)

target_include_directories(log-analyzer-tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${gtest_SOURCE_DIR}/include
    ${gtest_SOURCE_DIR}
)

# Explicitly add C++ standard library path for macOS
if(APPLE AND CMAKE_OSX_SYSROOT)
    target_include_directories(log-analyzer-tests PRIVATE
        ${CMAKE_OSX_SYSROOT}/usr/include/c++/v1
    )
endif()

target_link_libraries(log-analyzer-tests PRIVATE
    GTest::gtest
    GTest::gtest_main
    GTest::gmock
    pthread
)

# Add test to CTest
add_test(NAME LogAnalyzerTests COMMAND log-analyzer-tests)
