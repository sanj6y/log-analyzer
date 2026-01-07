# log-analyzer
High-performance C++ log and telemetry analysis CLI tool

## Overview
A C++ CLI tool designed to parse and analyze multi-gigabyte simulation logs 
produced by EDA tools. Focused on zero-copy parsing and high-throughput data aggregation.

## Technical Goals
* **Zero-Copy Parsing:** Utilize `std::string_view` to minimize memory allocations.
* **Concurrency:** Multi-threaded file processing using a producer-consumer model.
* **Memory Efficiency:** Handle logs larger than physical RAM via streaming.
* **Extensibility:** Support for custom log formats (Spectre, Xcelium, etc.).

## Roadmap
- Part 1: Core streaming engine and basic `string_view` parser.
- Part 2: Aggregator engine (Error/Warning frequency, Time-window analysis).
- Part 3: Multi-threading with `std::jthread` and thread-safe queues.
- Part 4: Integration of GoogleTest for unit testing.
- Part 5: Benchmarking suite vs. standard `regex` approaches.
