cmake_minimum_required(VERSION 3.20)
project(log-analyzer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add executable
add_executable(log-analyzer
    apps/main.cpp
    # TODO: Add source files
)

# Include directories
target_include_directories(log-analyzer PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# TODO: Add GoogleTest for tests
# TODO: Add source files from src/ directory
