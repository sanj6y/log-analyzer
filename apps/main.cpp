#include "parser/log_parser.hpp"
#include "utils/streaming_reader.hpp"
#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>

using namespace log_analyzer;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <log_file> [options]\n";
        std::cerr << "Options:\n";
        std::cerr << "  --buffer-size <size>  Buffer size in bytes (default: 1MB)\n";
        return 1;
    }

    std::string filepath = argv[1];
    std::size_t buffer_size = 1024 * 1024; // 1MB default

    // Parse command line options
    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--buffer-size" && i + 1 < argc) {
            buffer_size = std::stoull(argv[++i]);
        }
    }

    // Create streaming reader
    utils::StreamingReader reader(filepath, buffer_size);

    if (!reader.is_open()) {
        std::cerr << "Error: Could not open file: " << filepath << '\n';
        return 1;
    }

    // Process log file
    std::size_t line_count = 0;
    auto start_time = std::chrono::high_resolution_clock::now();

    std::cout << "Processing log file: " << filepath << '\n';
    std::cout << "Buffer size: " << buffer_size << " bytes\n\n";

    while (!reader.eof()) {
        std::string_view line = reader.next_line();
        if (line.empty()) {
            break;
        }

        ++line_count;

        // Example: Parse timestamp (zero-copy)
        auto timestamp = parser::LogParser::extract_timestamp(line);
        if (!timestamp.empty()) {
            // Can process timestamp without allocation
        }

        // Example: Check for error patterns (zero-copy)
        if (parser::LogParser::contains(line, "ERROR") ||
            parser::LogParser::contains(line, "FATAL")) {
            // Found error line - can process without allocation
        }

        // Progress indicator every 100k lines
        if (line_count % 100000 == 0) {
            auto now = std::chrono::high_resolution_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                now - start_time).count();
            std::size_t bytes = reader.bytes_read();
            
            double mbps = (bytes / 1024.0 / 1024.0) / std::max(1.0, elapsed / 60.0);
            
            std::cout << "Processed " << line_count << " lines, "
                      << (bytes / 1024 / 1024) << " MB, "
                      << std::fixed << std::setprecision(2) << mbps << " MB/min\n";
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time).count();

    std::size_t total_bytes = reader.bytes_read();
    double mbps = (total_bytes / 1024.0 / 1024.0) / std::max(1.0, duration / 60000.0);

    std::cout << "\n=== Summary ===\n";
    std::cout << "Total lines: " << line_count << '\n';
    std::cout << "Total bytes: " << total_bytes << " (" 
              << (total_bytes / 1024 / 1024) << " MB)\n";
    std::cout << "Processing time: " << duration << " ms (" 
              << (duration / 1000.0) << " seconds)\n";
    std::cout << "Throughput: " << std::fixed << std::setprecision(2) << mbps 
              << " MB/min\n";

    return 0;
}
