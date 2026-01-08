#include "analytics/thread_safe_aggregator.hpp"
#include "analytics/log_aggregator.hpp"
#include <algorithm>

namespace log_analyzer::analytics {

ThreadSafeAggregator::ThreadSafeAggregator(std::size_t time_window_seconds)
    : aggregator_(time_window_seconds)
{
}

void ThreadSafeAggregator::process_line(std::string_view line) {
    std::lock_guard<std::mutex> lock(mutex_);
    aggregator_.process_line(line);
}

std::vector<std::pair<std::string, std::size_t>> ThreadSafeAggregator::get_histogram_counts() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return aggregator_.histogram().get_all_counts();
}

std::size_t ThreadSafeAggregator::get_histogram_total() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return aggregator_.histogram().total();
}

std::vector<TimeWindowAnalyzer::WindowStats> ThreadSafeAggregator::get_time_window_stats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return aggregator_.time_windows().get_all_stats();
}

std::vector<std::pair<std::string, std::size_t>> ThreadSafeAggregator::get_pattern_matches() const {
    std::lock_guard<std::mutex> lock(mutex_);
    // pattern_matcher() is non-const but we're protected by mutex
    // get_match_counts() is const, so safe to call
    return const_cast<LogAggregator&>(aggregator_).pattern_matcher().get_match_counts();
}

void ThreadSafeAggregator::add_pattern(const std::string& pattern) {
    std::lock_guard<std::mutex> lock(mutex_);
    aggregator_.pattern_matcher().add_pattern(pattern);
}

void ThreadSafeAggregator::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    aggregator_.reset();
}

void ThreadSafeAggregator::merge(const ThreadSafeAggregator& other) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::lock_guard<std::mutex> other_lock(other.mutex_);
    
    // Merge histogram
    merge_histogram(other.aggregator_.histogram());
    
    // For time windows and patterns, we'd need to implement merge methods
    // For now, we'll just use a single shared aggregator with locking
}

void ThreadSafeAggregator::merge_histogram(const LogHistogram& other) {
    // Get all counts from other histogram and add to ours
    auto other_counts = other.get_all_counts();
    for (const auto& [level, count] : other_counts) {
        std::string_view level_view(level);
        // histogram() returns const ref, but we need non-const to increment
        // Since we're already locked in merge(), we can safely access non-const
        auto& hist = const_cast<LogHistogram&>(aggregator_.histogram());
        for (std::size_t i = 0; i < count; ++i) {
            hist.increment(level_view);
        }
    }
}

} // namespace log_analyzer::analytics
