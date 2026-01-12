# log-analyzer

High-performance C++ CLI tool for parsing and analyzing multi-gigabyte EDA telemetry logs.

## Overview

A C++20 utility designed to parse and analyze multi-gigabyte simulation logs produced by EDA tools. Focused on zero-copy parsing, high-throughput data aggregation, and constant memory footprint regardless of input size.

## Features

- **Zero-Copy Parsing:** Utilizes `std::string_view` to minimize memory allocations
- **Streaming Architecture:** Handles files larger than physical RAM with constant ~50MB memory footprint
- **Multi-Threading:** Producer-consumer model with configurable thread count
- **Stdin/Piping Support:** Process logs via stdin or file input
- **JSON Output:** Machine-readable output format with `--json` flag
- **Log Level Histogram:** Error/Warning/Info frequency analysis
- **Time Window Analysis:** Aggregates logs over configurable time intervals
- **Pattern Matching:** Find and count custom patterns in logs

## Building

```bash
mkdir build
cd build
cmake ..
cmake --build .
cd ..
```

The executable `loganalyze` will be created in the `build/` directory.

## Usage

### Basic Commands

```bash
# Analyze a log file (text output)
./build/loganalyze <log_file>

# JSON output
./build/loganalyze <log_file> --json

# Read from stdin/piping
cat log_file | ./build/loganalyze --json

# Explicit stdin
./build/loganalyze - --json
```

### Options

- `--json` - Output results in JSON format
- `--buffer-size <size>` - Buffer size in bytes (default: 1MB)
- `--time-window <seconds>` - Time window for analysis in seconds (default: 60)
- `--show-windows` - Show time window statistics
- `--pattern <pattern>` - Add pattern to match (can be repeated multiple times)
- `--threads <count>` - Number of worker threads (default: auto-detect, 0 = single-threaded)
- `--queue-size <size>` - Queue size for multi-threading (default: 10000)
- `--help` or `-h` - Show help message

### Examples

```bash
# Basic analysis with histogram
./build/loganalyze data/sample.log

# JSON output with patterns
./build/loganalyze data/sample.log --json --pattern "ERROR" --pattern "timeout"

# Multi-threaded processing
./build/loganalyze data/sample.log --threads 4 --json

# Time window analysis
./build/loganalyze data/sample.log --show-windows --time-window 30

# Piping with JSON
cat log_file.txt | ./build/loganalyze --json

# Single-threaded mode
./build/loganalyze data/sample.log --threads 0 --json
```

### JSON Output Format

When using `--json`, output includes:
- `summary`: Total lines, bytes, processing time, throughput
- `log_levels`: Histogram of log levels with counts and percentages
- `time_windows`: Time-based analysis (when `--show-windows` is used)
- `pattern_matches`: Pattern match counts (when patterns are specified)

Example JSON:
```json
{
  "summary": {
    "total_lines": 50105,
    "total_bytes": 2853967,
    "total_mb": 2.72,
    "processing_time_ms": 163.0,
    "processing_time_sec": 0.163,
    "throughput_mb_per_min": 2.72
  },
  "log_levels": {
    "total": 50000,
    "levels": [
      {
        "level": "ERROR",
        "count": 10128,
        "percentage": 20.26
      },
      ...
    ]
  }
}
```

## Testing

### Quick Tests

```bash
# Verify it works
./build/loganalyze data/sample.log | grep "Total lines"

# Validate JSON output
./build/loganalyze data/sample.log --json | python3 -m json.tool

# Test piping
cat data/sample.log | head -100 | ./build/loganalyze --json

# Test pattern matching
echo -e "ERROR: test\nWARNING: test\nINFO: test" | ./build/loganalyze --json --pattern "ERROR"
```

### Performance Testing

```bash
# Time single-threaded
time ./build/loganalyze data/sample.log --threads 0 > /dev/null

# Time multi-threaded
time ./build/loganalyze data/sample.log --threads 4 > /dev/null

# Compare throughput
./build/loganalyze data/sample.log --threads 0 | grep "Throughput"
./build/loganalyze data/sample.log --threads 4 | grep "Throughput"
```

## Technical Goals

- **Zero-Copy Parsing:** Utilize `std::string_view` to minimize memory allocations
- **Concurrency:** Multi-threaded file processing using a producer-consumer model
- **Memory Efficiency:** Handle logs larger than physical RAM via streaming
- **High Throughput:** Target 2GB/min single-threaded throughput
- **Extensibility:** Support for custom log formats (Spectre, Xcelium, etc.)


## Installation

To use `loganalyze` from anywhere:

```bash
# Copy to a directory in your PATH
sudo cp build/loganalyze /usr/local/bin/

# Or add build directory to PATH
export PATH="$PATH:$(pwd)/build"
```
