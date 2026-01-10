cmake_minimum_required(VERSION 3.15)
project(log-analyzer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Optimizations for high-performance parsing
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(-O3 -march=native)
endif()

# Add executable
add_executable(loganalyze
    apps/main.cpp
    src/analytics/log_aggregator.cpp
    src/analytics/thread_safe_aggregator.cpp
    src/parser/log_parser.cpp
    src/utils/json_output.cpp
    src/utils/memory_pool.cpp
    src/utils/streaming_reader.cpp
    src/utils/thread_safe_queue.cpp
)

# Include directories
target_include_directories(loganalyze PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# Explicitly add C++ standard library path for macOS
# Some macOS SDK configurations don't auto-detect libc++ headers
if(APPLE AND CMAKE_OSX_SYSROOT)
    target_include_directories(loganalyze PRIVATE
        ${CMAKE_OSX_SYSROOT}/usr/include/c++/v1
    )
endif()

# Link standard libraries
target_link_libraries(loganalyze PRIVATE pthread)

# GoogleTest setup (Part 4.1) - Commented out temporarily for main features
# Uncomment when working on Part 4.1/4.2
# include(FetchContent)
# FetchContent_Declare(
#     googletest
#     GIT_REPOSITORY https://github.com/google/googletest.git
#     GIT_TAG        v1.14.0
# )
# set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
# FetchContent_MakeAvailable(googletest)

# Enable testing (disabled temporarily for main features)
# enable_testing()

# Test executable (commented out - uncomment for Part 4.1/4.2)
# add_executable(log-analyzer-tests
#     tests/test_main.cpp
#     tests/test_parser.cpp
#     tests/test_streaming_reader.cpp
#     tests/test_histogram.cpp
#     tests/test_thread_safe_queue.cpp
#     src/parser/log_parser.cpp
#     src/utils/streaming_reader.cpp
#     src/utils/thread_safe_queue.cpp
#     src/analytics/log_aggregator.cpp
# )
# 
# target_include_directories(log-analyzer-tests PRIVATE
#     ${CMAKE_SOURCE_DIR}/include
# )
# 
# if(APPLE AND CMAKE_OSX_SYSROOT)
#     target_include_directories(log-analyzer-tests PRIVATE
#         ${CMAKE_OSX_SYSROOT}/usr/include/c++/v1
#     )
# endif()
# 
# target_link_libraries(log-analyzer-tests PRIVATE
#     GTest::gtest
#     GTest::gtest_main
#     GTest::gmock
#     pthread
# )
# 
# add_test(NAME LogAnalyzerTests COMMAND log-analyzer-tests)
