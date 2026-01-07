#include "parser/log_parser.hpp"
#include <algorithm>
#include <cctype>

namespace log_analyzer::parser {

std::vector<std::string_view> LogParser::split_fields(std::string_view line, char delimiter) {
    std::vector<std::string_view> fields;
    
    std::size_t start = 0;
    while (start < line.size()) {
        // Skip leading delimiters
        std::size_t field_start = line.find_first_not_of(delimiter, start);
        if (field_start == std::string_view::npos) {
            break;
        }

        // Find end of field (next delimiter or end of string)
        std::size_t field_end = line.find(delimiter, field_start);
        if (field_end == std::string_view::npos) {
            field_end = line.size();
        }

        fields.emplace_back(line.substr(field_start, field_end - field_start));
        start = field_end + 1;
    }

    return fields;
}

std::size_t LogParser::find(std::string_view line, std::string_view pattern) {
    return line.find(pattern);
}

bool LogParser::starts_with(std::string_view line, std::string_view prefix) {
    return line.size() >= prefix.size() &&
           line.substr(0, prefix.size()) == prefix;
}

bool LogParser::contains(std::string_view line, std::string_view pattern) {
    return line.find(pattern) != std::string_view::npos;
}

std::string_view LogParser::trim(std::string_view line) {
    if (line.empty()) {
        return line;
    }

    std::size_t start = 0;
    while (start < line.size() && std::isspace(static_cast<unsigned char>(line[start]))) {
        ++start;
    }

    if (start >= line.size()) {
        return {};
    }

    std::size_t end = line.size();
    while (end > start && std::isspace(static_cast<unsigned char>(line[end - 1]))) {
        --end;
    }

    return line.substr(start, end - start);
}

std::string_view LogParser::extract_timestamp(std::string_view line) {
    // Look for common timestamp patterns: "YYYY-MM-DD" or "HH:MM:SS"
    // Try to find a sequence that looks like a timestamp at the start
    std::size_t pos = 0;
    
    // Skip leading whitespace
    pos = skip_whitespace(line, pos);
    if (pos >= line.size()) {
        return {};
    }

    // Look for date pattern: YYYY-MM-DD or similar
    // This is a simple heuristic - can be extended for specific log formats
    std::size_t timestamp_start = pos;
    
    // Find end of potential timestamp (space or end)
    std::size_t timestamp_end = line.find(' ', timestamp_start);
    if (timestamp_end == std::string_view::npos) {
        // Try to find a reasonable timestamp length (e.g., up to 30 chars)
        timestamp_end = std::min(timestamp_start + 30, line.size());
    }

    // Check if we have a colon (common in timestamps) or dash
    std::size_t colon_pos = line.find(':', timestamp_start);
    std::size_t dash_pos = line.find('-', timestamp_start);
    
    if (colon_pos != std::string_view::npos && colon_pos < timestamp_end) {
        // Likely has time component, extend to include second time component
        std::size_t second_colon = line.find(':', colon_pos + 1);
        if (second_colon != std::string_view::npos && second_colon < timestamp_end) {
            // Include milliseconds or timezone if present (up to reasonable limit)
            std::size_t space_after = line.find(' ', second_colon + 1);
            if (space_after != std::string_view::npos && space_after < timestamp_start + 30) {
                timestamp_end = space_after;
            }
        }
    }

    if (timestamp_end > timestamp_start) {
        return line.substr(timestamp_start, timestamp_end - timestamp_start);
    }

    return {};
}

std::size_t LogParser::skip_whitespace(std::string_view str, std::size_t start) {
    while (start < str.size() && std::isspace(static_cast<unsigned char>(str[start]))) {
        ++start;
    }
    return start;
}

std::size_t LogParser::find_non_whitespace(std::string_view str, std::size_t start) {
    while (start < str.size() && !std::isspace(static_cast<unsigned char>(str[start]))) {
        ++start;
    }
    return start;
}

} // namespace log_analyzer::parser
