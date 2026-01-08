#include "analytics/log_aggregator.hpp"
#include <algorithm>
#include <cctype>
#include <cstring>
#include <iomanip>
#include <sstream>

namespace log_analyzer::analytics {

// LogHistogram implementation
void LogHistogram::increment(std::string_view level) {
    // Convert to string for storage (ensures lifetime)
    std::string level_str(level);
    level_counts_[level_str]++;
    total_count_++;
}

std::size_t LogHistogram::count(std::string_view level) const {
    std::string level_str(level);
    auto it = level_counts_.find(level_str);
    return it != level_counts_.end() ? it->second : 0;
}

std::vector<std::pair<std::string, std::size_t>> LogHistogram::get_all_counts() const {
    std::vector<std::pair<std::string, std::size_t>> result;
    result.reserve(level_counts_.size());
    
    for (const auto& [level, count] : level_counts_) {
        result.emplace_back(level, count);
    }
    
    // Sort by count descending
    std::sort(result.begin(), result.end(),
        [](const auto& a, const auto& b) {
            return a.second > b.second;
        });
    
    return result;
}

void LogHistogram::reset() {
    level_counts_.clear();
    total_count_ = 0;
}

// TimeWindowAnalyzer implementation
TimeWindowAnalyzer::TimeWindowAnalyzer(std::size_t window_seconds)
    : window_seconds_(window_seconds)
{
}

bool TimeWindowAnalyzer::process_entry(std::string_view timestamp, std::string_view log_level) {
    std::size_t window_idx = get_or_create_window(timestamp);
    
    if (window_idx >= windows_.size()) {
        return false;
    }
    
    auto& window = windows_[window_idx];
    window.total_entries++;
    
    // Categorize by log level
    std::string level_str(log_level);
    if (level_str == "ERROR" || level_str == "FATAL") {
        window.error_count++;
    } else if (level_str == "WARNING" || level_str == "WARN") {
        window.warning_count++;
    } else if (level_str == "INFO") {
        window.info_count++;
    } else {
        window.other_count++;
    }
    
    return window_idx == windows_.size() - 1; // Return true if this was a new window
}

TimeWindowAnalyzer::WindowStats TimeWindowAnalyzer::get_window_stats(std::size_t window_index) const {
    WindowStats stats;
    
    if (window_index >= windows_.size()) {
        return stats;
    }
    
    const auto& window = windows_[window_index];
    stats.window_start = window.start_time;
    stats.window_end = window.end_time;
    stats.total_entries = window.total_entries;
    stats.error_count = window.error_count;
    stats.warning_count = window.warning_count;
    stats.info_count = window.info_count;
    stats.other_count = window.other_count;
    
    return stats;
}

std::vector<TimeWindowAnalyzer::WindowStats> TimeWindowAnalyzer::get_all_stats() const {
    std::vector<WindowStats> result;
    result.reserve(windows_.size());
    
    for (std::size_t i = 0; i < windows_.size(); ++i) {
        result.push_back(get_window_stats(i));
    }
    
    return result;
}

void TimeWindowAnalyzer::reset() {
    windows_.clear();
}

std::size_t TimeWindowAnalyzer::parse_timestamp_to_seconds(std::string_view timestamp) const {
    // Simple timestamp parser: "YYYY-MM-DD HH:MM:SS" or "YYYY-MM-DD HH:MM:SS.mmm"
    // Returns seconds since epoch (simplified - just for windowing)
    
    if (timestamp.length() < 19) { // Minimum: "YYYY-MM-DD HH:MM:SS"
        return 0;
    }
    
    // Extract date and time components
    int year = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0;
    
    // Try to parse: "YYYY-MM-DD HH:MM:SS"
    // This is a simplified parser - for production, use a proper date/time library
    // But keeping it allocation-free for performance
    
    auto parse_int = [](std::string_view str, std::size_t start, std::size_t length) -> int {
        int value = 0;
        for (std::size_t i = 0; i < length && start + i < str.length(); ++i) {
            char c = str[start + i];
            if (c >= '0' && c <= '9') {
                value = value * 10 + (c - '0');
            } else {
                return -1;
            }
        }
        return value;
    };
    
    year = parse_int(timestamp, 0, 4);
    month = parse_int(timestamp, 5, 2);
    day = parse_int(timestamp, 8, 2);
    hour = parse_int(timestamp, 11, 2);
    minute = parse_int(timestamp, 14, 2);
    second = parse_int(timestamp, 17, 2);
    
    if (year < 0 || month < 0 || day < 0 || hour < 0 || minute < 0 || second < 0) {
        return 0;
    }
    
    // Simplified: convert to approximate seconds since epoch
    // For 2024-01-01, assume epoch start
    // This is approximate - for production, use proper time functions
    std::size_t days = (year - 2024) * 365 + (month - 1) * 30 + (day - 1);
    return days * 86400 + hour * 3600 + minute * 60 + second;
}

std::string TimeWindowAnalyzer::format_seconds_to_timestamp(std::size_t seconds) const {
    // Simplified formatter - in production, use proper time functions
    std::size_t days = seconds / 86400;
    std::size_t remaining = seconds % 86400;
    std::size_t hours = remaining / 3600;
    remaining %= 3600;
    std::size_t minutes = remaining / 60;
    std::size_t secs = remaining % 60;
    
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(4) << (2024 + days / 365) << "-"
        << std::setw(2) << ((days % 365) / 30 + 1) << "-"
        << std::setw(2) << ((days % 365) % 30 + 1) << " "
        << std::setw(2) << hours << ":"
        << std::setw(2) << minutes << ":"
        << std::setw(2) << secs;
    
    return oss.str();
}

std::size_t TimeWindowAnalyzer::get_or_create_window(std::string_view timestamp) {
    std::size_t timestamp_seconds = parse_timestamp_to_seconds(timestamp);
    if (timestamp_seconds == 0) {
        return SIZE_MAX; // Invalid timestamp
    }
    
    std::size_t window_start_seconds = (timestamp_seconds / window_seconds_) * window_seconds_;
    
    // Find existing window or create new one
    for (std::size_t i = 0; i < windows_.size(); ++i) {
        std::size_t window_start = parse_timestamp_to_seconds(windows_[i].start_time);
        if (window_start == window_start_seconds) {
            // Update end time
            windows_[i].end_time = std::string(timestamp);
            return i;
        }
    }
    
    // Create new window
    TimeWindow window;
    window.start_time = format_seconds_to_timestamp(window_start_seconds);
    window.end_time = std::string(timestamp);
    windows_.push_back(std::move(window));
    
    return windows_.size() - 1;
}

// LogPatternMatcher implementation
void LogPatternMatcher::add_pattern(const std::string& pattern) {
    patterns_.push_back(pattern);
    match_counts_.push_back(0);
}

bool LogPatternMatcher::matches(std::string_view line) const {
    for (std::size_t i = 0; i < patterns_.size(); ++i) {
        if (line.find(patterns_[i]) != std::string_view::npos) {
            const_cast<LogPatternMatcher*>(this)->match_counts_[i]++;
            return true;
        }
    }
    return false;
}

std::vector<std::pair<std::string, std::size_t>> LogPatternMatcher::get_match_counts() const {
    std::vector<std::pair<std::string, std::size_t>> result;
    result.reserve(patterns_.size());
    
    for (std::size_t i = 0; i < patterns_.size(); ++i) {
        result.emplace_back(patterns_[i], match_counts_[i]);
    }
    
    return result;
}

void LogPatternMatcher::reset() {
    std::fill(match_counts_.begin(), match_counts_.end(), 0);
}

// LogAggregator implementation
LogAggregator::LogAggregator(std::size_t time_window_seconds)
    : time_analyzer_(time_window_seconds)
{
}

void LogAggregator::process_line(std::string_view line) {
    // Extract log level
    std::string_view log_level = extract_log_level(line);
    if (!log_level.empty()) {
        histogram_.increment(log_level);
    }
    
    // Extract timestamp and process time window
    // Try to find timestamp at the beginning of the line
    std::string_view timestamp;
    std::size_t space_pos = line.find(' ');
    if (space_pos != std::string_view::npos) {
        std::size_t next_space = line.find(' ', space_pos + 1);
        if (next_space != std::string_view::npos) {
            // Found two spaces - likely timestamp format: "YYYY-MM-DD HH:MM:SS"
            timestamp = line.substr(0, next_space);
        }
    }
    
    if (!timestamp.empty() && !log_level.empty()) {
        time_analyzer_.process_entry(timestamp, log_level);
    }
    
    // Check pattern matches (ignore return value - matches() updates internal counts)
    (void)pattern_matcher_.matches(line);
}

std::string_view LogAggregator::extract_log_level(std::string_view line) const {
    // Look for common log level patterns: "INFO:", "ERROR:", "WARNING:", etc.
    const char* level_patterns[] = {
        "FATAL:", "ERROR:", "WARN:", "WARNING:", "INFO:", "DEBUG:", "TRACE:"
    };
    
    for (const char* pattern : level_patterns) {
        std::size_t pos = line.find(pattern);
        if (pos != std::string_view::npos) {
            // Extract the level name (before the colon)
            std::size_t start = pos;
            std::size_t end = pos;
            while (end < line.length() && line[end] != ':') {
                end++;
            }
            if (end < line.length()) {
                return line.substr(start, end - start);
            }
        }
    }
    
    // Try uppercase versions without colon
    for (const char* pattern : level_patterns) {
        std::string_view pattern_view(pattern, strlen(pattern) - 1); // Remove colon
        std::size_t pos = line.find(pattern_view);
        if (pos != std::string_view::npos) {
            // Check if it's a word boundary
            if ((pos == 0 || std::isspace(line[pos - 1])) &&
                (pos + pattern_view.length() >= line.length() || 
                 std::isspace(line[pos + pattern_view.length()]))) {
                return pattern_view;
            }
        }
    }
    
    return {};
}

void LogAggregator::reset() {
    histogram_.reset();
    time_analyzer_.reset();
    pattern_matcher_.reset();
}

} // namespace log_analyzer::analytics
