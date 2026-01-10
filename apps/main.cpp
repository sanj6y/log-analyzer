#include "analytics/log_aggregator.hpp"
#include "analytics/thread_safe_aggregator.hpp"
#include "parser/log_parser.hpp"
#include "utils/json_output.hpp"
#include "utils/streaming_reader.hpp"
#include "utils/thread_safe_queue.hpp"
#include <atomic>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#if __cplusplus >= 202002L && defined(__cpp_lib_jthread)
#include <stop_token>
#define HAS_JTHREAD 1
#else
#define HAS_JTHREAD 0
#endif

using namespace log_analyzer;

int main(int argc, char* argv[]) {
    std::size_t buffer_size = 1024 * 1024; // 1MB default
    std::size_t time_window = 60; // 60 seconds default
    bool show_windows = false;
    bool json_output = false;
    unsigned int num_threads = std::thread::hardware_concurrency(); // Default to hardware threads
    std::size_t queue_size = 10000;

    std::vector<std::string> patterns;
    std::string filepath;

    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--buffer-size" && i + 1 < argc) {
            buffer_size = std::stoull(argv[++i]);
        } else if (arg == "--time-window" && i + 1 < argc) {
            time_window = std::stoull(argv[++i]);
        } else if (arg == "--show-windows") {
            show_windows = true;
        } else if (arg == "--json") {
            json_output = true;
        } else if (arg == "--pattern" && i + 1 < argc) {
            patterns.push_back(argv[++i]);
        } else if (arg == "--threads" && i + 1 < argc) {
            num_threads = std::stoul(argv[++i]);
        } else if (arg == "--queue-size" && i + 1 < argc) {
            queue_size = std::stoull(argv[++i]);
        } else if (arg == "--help" || arg == "-h") {
            std::cerr << "Usage: " << argv[0] << " [log_file] [options]\n";
            std::cerr << "       " << argv[0] << " [options] < log_file\n";
            std::cerr << "       cat log_file | " << argv[0] << " [options]\n";
            std::cerr << "\n";
            std::cerr << "Options:\n";
            std::cerr << "  [log_file]              Log file to process (use '-' or omit for stdin)\n";
            std::cerr << "  --json                  Output results in JSON format\n";
            std::cerr << "  --buffer-size <size>    Buffer size in bytes (default: 1MB)\n";
            std::cerr << "  --time-window <seconds> Time window for analysis in seconds (default: 60)\n";
            std::cerr << "  --show-windows          Show time window statistics\n";
            std::cerr << "  --pattern <pattern>     Add pattern to match (can be repeated)\n";
            std::cerr << "  --threads <count>       Number of worker threads (default: auto, 0 = single-threaded)\n";
            std::cerr << "  --queue-size <size>     Queue size for multi-threading (default: 10000)\n";
            return 0;
        } else if (arg[0] != '-') {
            // Non-option argument - treat as filepath
            if (filepath.empty()) {
                filepath = arg;
            }
        }
    }

    // If no filepath provided, use stdin ("-")
    if (filepath.empty()) {
        filepath = "-";
    }

    // Single-threaded mode if threads == 0
    bool use_multithreading = (num_threads > 0);
    if (num_threads == 0) {
        num_threads = 1;
        use_multithreading = false;
    }

    auto start_time = std::chrono::high_resolution_clock::now();

    if (!json_output) {
        std::cout << "Processing log " << (filepath == "-" ? "(stdin)" : "file: " + filepath) << '\n';
        std::cout << "Buffer size: " << buffer_size << " bytes\n";
        std::cout << "Time window: " << time_window << " seconds\n";
        std::cout << "Threads: " << (use_multithreading ? std::to_string(num_threads) : "1 (single-threaded)") << '\n';
        if (use_multithreading) {
            std::cout << "Queue size: " << queue_size << '\n';
        }
        if (!patterns.empty()) {
            std::cout << "Patterns: " << patterns.size() << '\n';
        }
        std::cout << '\n';
    }

    if (use_multithreading && num_threads > 1) {
        // Multi-threaded processing
        utils::StreamingReader reader(filepath, buffer_size);
        
        if (!reader.is_open()) {
            std::cerr << "Error: Could not open file: " << filepath << '\n';
            return 1;
        }

        // Create thread-safe queue and aggregator
        utils::ThreadSafeQueue queue(queue_size);
        analytics::ThreadSafeAggregator aggregator(time_window);

        // Add patterns
        for (const auto& pattern : patterns) {
            aggregator.add_pattern(pattern);
        }

        std::atomic<std::size_t> lines_produced{0};
        std::atomic<std::size_t> lines_processed{0};
        std::atomic<bool> producer_done{false};

        // Producer thread: reads from file and enqueues lines
        std::thread producer_thread([&]() {
            while (!reader.eof()) {
                std::string_view line_view = reader.next_line();
                if (line_view.empty()) {
                    break;
                }
                
                // Convert string_view to string for queue (ensures lifetime)
                std::string line(line_view);
                queue.push(std::move(line));
                lines_produced++;
                
                // Progress indicator
                if (lines_produced % 100000 == 0) {
                    auto now = std::chrono::high_resolution_clock::now();
                    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                        now - start_time).count();
                    std::size_t bytes = reader.bytes_read();
                    double mbps = (bytes / 1024.0 / 1024.0) / std::max(1.0, elapsed / 60.0);
                    
                    if (!json_output) {
                        std::cout << "Produced " << lines_produced << " lines, "
                                  << (bytes / 1024 / 1024) << " MB, "
                                  << std::fixed << std::setprecision(2) << mbps << " MB/min\n";
                    }
                }
            }
            producer_done = true;
            queue.close();
        });

        // Consumer threads: process lines from queue
        std::vector<std::thread> consumer_threads;
        consumer_threads.reserve(num_threads);

        for (unsigned int i = 0; i < num_threads; ++i) {
            consumer_threads.emplace_back([&]() {
                while (true) {
                    std::string line = queue.pop();
                    if (line.empty() && queue.is_closed()) {
                        break;
                    }
                    
                    if (!line.empty()) {
                        aggregator.process_line(line);
                        std::size_t processed = lines_processed.fetch_add(1) + 1;
                        
            // Progress indicator (only if not JSON output)
            if (!json_output && processed % 100000 == 0) {
                std::cout << "Processed " << processed << " lines\n";
            }
                    }
                }
            });
        }

        // Wait for producer to finish
        producer_thread.join();

        // Wait for all consumers to finish
        for (auto& thread : consumer_threads) {
            thread.join();
        }

        std::size_t total_bytes = reader.bytes_read();

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - start_time).count();

        double mbps = (total_bytes / 1024.0 / 1024.0) / std::max(1.0, duration / 60000.0);

        // Get aggregator data for output
        auto level_counts = aggregator.get_histogram_counts();
        std::size_t total = aggregator.get_histogram_total();

        if (json_output) {
            // JSON output for multi-threaded mode
            utils::JsonOutput json_out(std::cout);
            
            std::cout << "{\n";
            json_out.output_summary(lines_processed, total_bytes, duration, mbps);
            if (!level_counts.empty()) {
                json_out.output_histogram(level_counts, total);
            }
            if (show_windows) {
                auto window_stats = aggregator.get_time_window_stats();
                if (!window_stats.empty()) {
                    json_out.output_time_windows(window_stats);
                }
            }
            auto pattern_counts = aggregator.get_pattern_matches();
            if (!pattern_counts.empty()) {
                json_out.output_patterns(pattern_counts);
            }
            std::cout << "\n}\n";
        } else {
            // Text output
            std::cout << "\n=== Processing Summary ===\n";
            std::cout << "Total lines produced: " << lines_produced << '\n';
            std::cout << "Total lines processed: " << lines_processed << '\n';
            std::cout << "Total bytes: " << total_bytes << " (" 
                      << (total_bytes / 1024 / 1024) << " MB)\n";
            std::cout << "Processing time: " << duration << " ms (" 
                      << (duration / 1000.0) << " seconds)\n";
            std::cout << "Throughput: " << std::fixed << std::setprecision(2) << mbps 
                      << " MB/min\n";

            // Display log level histogram
            std::cout << "\n=== Log Level Frequency ===\n";
            if (level_counts.empty()) {
                std::cout << "No log levels detected.\n";
            } else {
                std::cout << std::left << std::setw(12) << "Level" 
                          << std::right << std::setw(12) << "Count" 
                          << std::setw(12) << "Percentage" << '\n';
                std::cout << std::string(36, '-') << '\n';
                
                for (const auto& [level, count] : level_counts) {
                    double percentage = total > 0 ? (100.0 * count / total) : 0.0;
                    std::cout << std::left << std::setw(12) << level
                              << std::right << std::setw(12) << count
                              << std::fixed << std::setprecision(2) << std::setw(12) << percentage << "%\n";
                }
                std::cout << std::string(36, '-') << '\n';
                std::cout << std::left << std::setw(12) << "TOTAL"
                          << std::right << std::setw(12) << total << '\n';
            }

            // Display time window statistics if requested
            if (show_windows) {
                std::cout << "\n=== Time Window Analysis ===\n";
                auto window_stats = aggregator.get_time_window_stats();
                
                if (window_stats.empty()) {
                    std::cout << "No time windows detected.\n";
                } else {
                    std::cout << std::left << std::setw(20) << "Window Start"
                              << std::setw(20) << "Window End"
                              << std::right << std::setw(8) << "Total"
                              << std::setw(8) << "Error"
                              << std::setw(8) << "Warn"
                              << std::setw(8) << "Info" << '\n';
                    std::cout << std::string(72, '-') << '\n';
                    
                    for (const auto& stats : window_stats) {
                        std::cout << std::left << std::setw(20) << stats.window_start
                                  << std::setw(20) << stats.window_end
                                  << std::right << std::setw(8) << stats.total_entries
                                  << std::setw(8) << stats.error_count
                                  << std::setw(8) << stats.warning_count
                                  << std::setw(8) << stats.info_count << '\n';
                    }
                }
            }

            // Display pattern matches if patterns were provided
            auto pattern_counts = aggregator.get_pattern_matches();
            if (!pattern_counts.empty()) {
                std::cout << "\n=== Pattern Matches ===\n";
                for (const auto& [pattern, count] : pattern_counts) {
                    std::cout << std::left << std::setw(40) << pattern 
                              << ": " << count << " matches\n";
                }
            }
        }

    } else {
        // Single-threaded processing (original code)
        utils::StreamingReader reader(filepath, buffer_size);

        if (!reader.is_open()) {
            std::cerr << "Error: Could not open file: " << filepath << '\n';
            return 1;
        }

        // Create log aggregator
        analytics::LogAggregator aggregator(time_window);

        // Add patterns from command line
        for (const auto& pattern : patterns) {
            aggregator.pattern_matcher().add_pattern(pattern);
        }

        // Process log file
        std::size_t line_count = 0;

        while (!reader.eof()) {
            std::string_view line = reader.next_line();
            if (line.empty()) {
                break;
            }

            ++line_count;

            // Process line through aggregator (zero-copy)
            aggregator.process_line(line);

            // Progress indicator every 100k lines (only if not JSON output)
            if (!json_output && line_count % 100000 == 0) {
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

        if (json_output) {
            utils::JsonOutput json_out(std::cout);
            json_out.output_complete(aggregator, line_count, total_bytes, duration, mbps, show_windows);
        } else {
            // Text output
            std::cout << "\n=== Processing Summary ===\n";
            std::cout << "Total lines: " << line_count << '\n';
            std::cout << "Total bytes: " << total_bytes << " (" 
                      << (total_bytes / 1024 / 1024) << " MB)\n";
            std::cout << "Processing time: " << duration << " ms (" 
                      << (duration / 1000.0) << " seconds)\n";
            std::cout << "Throughput: " << std::fixed << std::setprecision(2) << mbps 
                      << " MB/min\n";

            // Display log level histogram
            std::cout << "\n=== Log Level Frequency ===\n";
            const auto& histogram = aggregator.histogram();
            auto level_counts = histogram.get_all_counts();
            
            if (level_counts.empty()) {
                std::cout << "No log levels detected.\n";
            } else {
                std::cout << std::left << std::setw(12) << "Level" 
                          << std::right << std::setw(12) << "Count" 
                          << std::setw(12) << "Percentage" << '\n';
                std::cout << std::string(36, '-') << '\n';
                
                std::size_t total = histogram.total();
                for (const auto& [level, count] : level_counts) {
                    double percentage = total > 0 ? (100.0 * count / total) : 0.0;
                    std::cout << std::left << std::setw(12) << level
                              << std::right << std::setw(12) << count
                              << std::fixed << std::setprecision(2) << std::setw(12) << percentage << "%\n";
                }
                std::cout << std::string(36, '-') << '\n';
                std::cout << std::left << std::setw(12) << "TOTAL"
                          << std::right << std::setw(12) << total << '\n';
            }

            // Display time window statistics if requested
            if (show_windows) {
                std::cout << "\n=== Time Window Analysis ===\n";
                const auto& time_windows = aggregator.time_windows();
                auto window_stats = time_windows.get_all_stats();
                
                if (window_stats.empty()) {
                    std::cout << "No time windows detected.\n";
                } else {
                    std::cout << std::left << std::setw(20) << "Window Start"
                              << std::setw(20) << "Window End"
                              << std::right << std::setw(8) << "Total"
                              << std::setw(8) << "Error"
                              << std::setw(8) << "Warn"
                              << std::setw(8) << "Info" << '\n';
                    std::cout << std::string(72, '-') << '\n';
                    
                    for (const auto& stats : window_stats) {
                        std::cout << std::left << std::setw(20) << stats.window_start
                                  << std::setw(20) << stats.window_end
                                  << std::right << std::setw(8) << stats.total_entries
                                  << std::setw(8) << stats.error_count
                                  << std::setw(8) << stats.warning_count
                                  << std::setw(8) << stats.info_count << '\n';
                    }
                }
            }

            // Display pattern matches if patterns were provided
            auto pattern_counts = aggregator.pattern_matcher().get_match_counts();
            if (!pattern_counts.empty()) {
                std::cout << "\n=== Pattern Matches ===\n";
                for (const auto& [pattern, count] : pattern_counts) {
                    std::cout << std::left << std::setw(40) << pattern 
                              << ": " << count << " matches\n";
                }
            }
        }
    }

    return 0;
}
