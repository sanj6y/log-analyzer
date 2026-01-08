#pragma once

#include "analytics/log_aggregator.hpp"
#include <mutex>

namespace log_analyzer::analytics {

/**
 * Thread-safe wrapper around LogAggregator for multi-threaded processing.
 * Uses fine-grained locking to minimize contention.
 */
class ThreadSafeAggregator {
public:
    explicit ThreadSafeAggregator(std::size_t time_window_seconds = 60);
    ~ThreadSafeAggregator() = default;

    // Non-copyable, movable
    ThreadSafeAggregator(const ThreadSafeAggregator&) = delete;
    ThreadSafeAggregator& operator=(const ThreadSafeAggregator&) = delete;
    ThreadSafeAggregator(ThreadSafeAggregator&&) = delete;
    ThreadSafeAggregator& operator=(ThreadSafeAggregator&&) = delete;

    /**
     * Process a log line thread-safely.
     */
    void process_line(std::string_view line);

    /**
     * Get histogram statistics (thread-safe, returns data by value).
     */
    [[nodiscard]] std::vector<std::pair<std::string, std::size_t>> get_histogram_counts() const;

    /**
     * Get total histogram count.
     */
    [[nodiscard]] std::size_t get_histogram_total() const;

    /**
     * Get time window statistics (thread-safe, returns data by value).
     */
    [[nodiscard]] std::vector<TimeWindowAnalyzer::WindowStats> get_time_window_stats() const;

    /**
     * Get pattern matcher (returns copy of match counts).
     */
    [[nodiscard]] std::vector<std::pair<std::string, std::size_t>> get_pattern_matches() const;

    /**
     * Add a pattern to match (thread-safe).
     */
    void add_pattern(const std::string& pattern);

    /**
     * Reset all aggregators (thread-safe).
     */
    void reset();

    /**
     * Merge results from another aggregator (for combining per-thread results).
     */
    void merge(const ThreadSafeAggregator& other);

private:
    // Use separate aggregators per thread to avoid contention
    // Then merge them periodically or at the end
    mutable std::mutex mutex_;
    LogAggregator aggregator_;

    /**
     * Merge histogram data thread-safely.
     */
    void merge_histogram(const LogHistogram& other);
};

} // namespace log_analyzer::analytics
