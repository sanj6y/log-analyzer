#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace log_analyzer::analytics {

/**
 * Histogram for tracking log level frequencies (zero-copy).
 * Uses string_view for keys to avoid allocations.
 */
class LogHistogram {
public:
    LogHistogram() = default;
    ~LogHistogram() = default;

    // Non-copyable, movable
    LogHistogram(const LogHistogram&) = delete;
    LogHistogram& operator=(const LogHistogram&) = delete;
    LogHistogram(LogHistogram&&) noexcept = default;
    LogHistogram& operator=(LogHistogram&&) noexcept = default;

    /**
     * Increment count for a log level (zero-copy - stores string_view).
     * Note: The string_view must remain valid for the lifetime of this histogram.
     */
    void increment(std::string_view level);

    /**
     * Get count for a specific log level.
     */
    [[nodiscard]] std::size_t count(std::string_view level) const;

    /**
     * Get total count of all log entries.
     */
    [[nodiscard]] std::size_t total() const noexcept { return total_count_; }

    /**
     * Get all log levels and their counts.
     */
    [[nodiscard]] std::vector<std::pair<std::string, std::size_t>> get_all_counts() const;

    /**
     * Reset all counts.
     */
    void reset();

private:
    // Map from log level (as string) to count
    std::unordered_map<std::string, std::size_t> level_counts_;
    std::size_t total_count_ = 0;
};

/**
 * Time window aggregator for analyzing log entries over time intervals.
 * Maintains zero-copy operations where possible.
 */
class TimeWindowAnalyzer {
public:
    explicit TimeWindowAnalyzer(std::size_t window_seconds = 60);
    ~TimeWindowAnalyzer() = default;

    // Non-copyable, movable
    TimeWindowAnalyzer(const TimeWindowAnalyzer&) = delete;
    TimeWindowAnalyzer& operator=(const TimeWindowAnalyzer&) = delete;
    TimeWindowAnalyzer(TimeWindowAnalyzer&&) noexcept = default;
    TimeWindowAnalyzer& operator=(TimeWindowAnalyzer&&) noexcept = default;

    /**
     * Process a log entry with timestamp.
     * Returns true if a new time window was created.
     */
    bool process_entry(std::string_view timestamp, std::string_view log_level);

    /**
     * Get statistics for a specific time window (by index).
     */
    struct WindowStats {
        std::string window_start;
        std::string window_end;
        std::size_t total_entries = 0;
        std::size_t error_count = 0;
        std::size_t warning_count = 0;
        std::size_t info_count = 0;
        std::size_t other_count = 0;
    };

    [[nodiscard]] WindowStats get_window_stats(std::size_t window_index) const;

    /**
     * Get number of time windows.
     */
    [[nodiscard]] std::size_t window_count() const noexcept { return windows_.size(); }

    /**
     * Get all window statistics.
     */
    [[nodiscard]] std::vector<WindowStats> get_all_stats() const;

    /**
     * Reset all data.
     */
    void reset();

private:
    struct TimeWindow {
        std::string start_time;
        std::string end_time;
        std::size_t total_entries = 0;
        std::size_t error_count = 0;
        std::size_t warning_count = 0;
        std::size_t info_count = 0;
        std::size_t other_count = 0;
    };

    std::size_t window_seconds_;
    std::vector<TimeWindow> windows_;

    /**
     * Parse timestamp and convert to seconds since epoch for comparison.
     * Returns 0 if parsing fails.
     */
    std::size_t parse_timestamp_to_seconds(std::string_view timestamp) const;

    /**
     * Format seconds since epoch back to timestamp string.
     */
    std::string format_seconds_to_timestamp(std::size_t seconds) const;

    /**
     * Find or create time window for given timestamp.
     */
    std::size_t get_or_create_window(std::string_view timestamp);
};

/**
 * Pattern matcher for finding specific log patterns (zero-copy).
 */
class LogPatternMatcher {
public:
    LogPatternMatcher() = default;
    ~LogPatternMatcher() = default;

    /**
     * Add a pattern to match (stored as string to ensure lifetime).
     */
    void add_pattern(const std::string& pattern);

    /**
     * Check if a log line matches any pattern (zero-copy on log line).
     */
    [[nodiscard]] bool matches(std::string_view line) const;

    /**
     * Get count of matches for each pattern.
     */
    [[nodiscard]] std::vector<std::pair<std::string, std::size_t>> get_match_counts() const;

    /**
     * Reset all match counts.
     */
    void reset();

private:
    std::vector<std::string> patterns_;
    std::vector<std::size_t> match_counts_;
};

/**
 * Main aggregator that combines histogram, time-window analysis, and pattern matching.
 */
class LogAggregator {
public:
    explicit LogAggregator(std::size_t time_window_seconds = 60);
    ~LogAggregator() = default;

    // Non-copyable, movable
    LogAggregator(const LogAggregator&) = delete;
    LogAggregator& operator=(const LogAggregator&) = delete;
    LogAggregator(LogAggregator&&) noexcept = default;
    LogAggregator& operator=(LogAggregator&&) noexcept = default;

    /**
     * Process a single log line (zero-copy where possible).
     * Extracts timestamp and log level, then updates all aggregators.
     */
    void process_line(std::string_view line);

    /**
     * Get histogram statistics.
     */
    [[nodiscard]] const LogHistogram& histogram() const noexcept { return histogram_; }

    /**
     * Get time window analyzer.
     */
    [[nodiscard]] const TimeWindowAnalyzer& time_windows() const noexcept { return time_analyzer_; }

    /**
     * Get pattern matcher.
     */
    [[nodiscard]] LogPatternMatcher& pattern_matcher() noexcept { return pattern_matcher_; }

    /**
     * Reset all aggregators.
     */
    void reset();

private:
    LogHistogram histogram_;
    TimeWindowAnalyzer time_analyzer_;
    LogPatternMatcher pattern_matcher_;

    /**
     * Extract log level from a log line (zero-copy).
     * Common formats: "INFO:", "ERROR:", "WARNING:", etc.
     */
    [[nodiscard]] std::string_view extract_log_level(std::string_view line) const;
};

} // namespace log_analyzer::analytics
