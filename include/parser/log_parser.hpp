#pragma once

#include <string_view>
#include <utility>
#include <vector>

#if __has_include(<charconv>)
#include <charconv>
#define LOG_ANALYZER_HAS_CHARCONV 1
#else
#define LOG_ANALYZER_HAS_CHARCONV 0
#endif

namespace log_analyzer::parser {

/**
 * Zero-copy log line parser using std::string_view.
 * Supports allocation-free parsing and conversion operations.
 */
class LogParser {
public:
    /**
     * Parse a log line and extract fields using string_view.
     * No allocations performed - all views point into original line.
     */
    [[nodiscard]] static std::vector<std::string_view> split_fields(
        std::string_view line,
        char delimiter = ' ');

    /**
     * Find a substring in the line (zero-copy).
     * Returns position if found, npos otherwise.
     */
    [[nodiscard]] static std::size_t find(std::string_view line, std::string_view pattern);

    /**
     * Check if line starts with a prefix (zero-copy).
     */
    [[nodiscard]] static bool starts_with(std::string_view line, std::string_view prefix);

    /**
     * Check if line contains a substring (zero-copy).
     */
    [[nodiscard]] static bool contains(std::string_view line, std::string_view pattern);

    /**
     * Trim whitespace from both ends (returns new view, zero-copy).
     */
    [[nodiscard]] static std::string_view trim(std::string_view line);

    /**
     * Extract timestamp from log line (zero-copy).
     * Assumes format: "YYYY-MM-DD HH:MM:SS" or similar at start of line.
     * Returns string_view pointing to timestamp portion, or empty if not found.
     */
    [[nodiscard]] static std::string_view extract_timestamp(std::string_view line);

#if LOG_ANALYZER_HAS_CHARCONV
    /**
     * Parse integer from string_view using std::from_chars (allocation-free).
     * Returns pair of (value, success).
     */
    template<typename T>
    [[nodiscard]] static std::pair<T, bool> parse_integer(std::string_view str);

    /**
     * Parse floating point from string_view using std::from_chars (allocation-free).
     * Returns pair of (value, success).
     */
    template<typename T>
    [[nodiscard]] static std::pair<T, bool> parse_float(std::string_view str);
#endif

private:
    /**
     * Skip whitespace from start.
     */
    [[nodiscard]] static std::size_t skip_whitespace(std::string_view str, std::size_t start);
    
    /**
     * Find next non-whitespace character.
     */
    [[nodiscard]] static std::size_t find_non_whitespace(std::string_view str, std::size_t start);
};

#if LOG_ANALYZER_HAS_CHARCONV
// Template implementations
template<typename T>
std::pair<T, bool> LogParser::parse_integer(std::string_view str) {
    T value{};
    auto result = std::from_chars(str.data(), str.data() + str.size(), value);
    if (result.ec == std::errc{} && result.ptr == str.data() + str.size()) {
        return {value, true};
    }
    return {T{}, false};
}

template<typename T>
std::pair<T, bool> LogParser::parse_float(std::string_view str) {
    T value{};
    auto result = std::from_chars(str.data(), str.data() + str.size(), value);
    if (result.ec == std::errc{} && result.ptr == str.data() + str.size()) {
        return {value, true};
    }
    return {T{}, false};
}
#endif

} // namespace log_analyzer::parser
