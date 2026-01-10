#include "utils/json_output.hpp"
#include "analytics/log_aggregator.hpp"
#include <iomanip>
#include <sstream>

namespace log_analyzer::utils {

void JsonOutput::comma_if_needed() {
    if (!first_item_) {
        out_ << ",";
    }
    first_item_ = false;
}

void JsonOutput::reset_comma() {
    first_item_ = true;
}

std::string JsonOutput::escape_json(const std::string& str) {
    std::ostringstream oss;
    for (char c : str) {
        switch (c) {
            case '"':  oss << "\\\""; break;
            case '\\': oss << "\\\\"; break;
            case '\b': oss << "\\b";  break;
            case '\f': oss << "\\f";  break;
            case '\n': oss << "\\n";  break;
            case '\r': oss << "\\r";  break;
            case '\t': oss << "\\t";  break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    oss << "\\u" << std::hex << std::setw(4) << std::setfill('0') 
                        << static_cast<int>(c);
                } else {
                    oss << c;
                }
                break;
        }
    }
    return oss.str();
}

void JsonOutput::output_summary(std::size_t total_lines, std::size_t total_bytes,
                                double processing_time_ms, double throughput_mb_per_min) {
    out_ << "  \"summary\": {\n";
    out_ << "    \"total_lines\": " << total_lines << ",\n";
    out_ << "    \"total_bytes\": " << total_bytes << ",\n";
    out_ << "    \"total_mb\": " << std::fixed << std::setprecision(2) 
         << (total_bytes / 1024.0 / 1024.0) << ",\n";
    out_ << "    \"processing_time_ms\": " << processing_time_ms << ",\n";
    out_ << "    \"processing_time_sec\": " << std::fixed << std::setprecision(3) << (processing_time_ms / 1000.0) << ",\n";
    out_ << "    \"throughput_mb_per_min\": " << std::fixed << std::setprecision(2) 
         << throughput_mb_per_min << "\n";
    out_ << "  }";
    reset_comma(); // Reset for next section
}

void JsonOutput::output_histogram(const std::vector<std::pair<std::string, std::size_t>>& level_counts,
                                  std::size_t total) {
    comma_if_needed();
    out_ << ",\n";
    out_ << "  \"log_levels\": {\n";
    out_ << "    \"total\": " << total << ",\n";
    out_ << "    \"levels\": [";
    
    reset_comma();
    for (const auto& [level, count] : level_counts) {
        comma_if_needed();
        double percentage = total > 0 ? (100.0 * count / total) : 0.0;
        out_ << "\n      {\n";
        out_ << "        \"level\": \"" << escape_json(level) << "\",\n";
        out_ << "        \"count\": " << count << ",\n";
        out_ << "        \"percentage\": " << std::fixed << std::setprecision(2) << percentage << "\n";
        out_ << "      }";
    }
    
    out_ << "\n    ]\n";
    out_ << "  }";
}

void JsonOutput::output_time_windows(const std::vector<analytics::TimeWindowAnalyzer::WindowStats>& windows) {
    comma_if_needed();
    out_ << "\n  \"time_windows\": [";
    
    reset_comma();
    for (const auto& stats : windows) {
        comma_if_needed();
        out_ << "\n    {\n";
        out_ << "      \"window_start\": \"" << escape_json(stats.window_start) << "\",\n";
        out_ << "      \"window_end\": \"" << escape_json(stats.window_end) << "\",\n";
        out_ << "      \"total_entries\": " << stats.total_entries << ",\n";
        out_ << "      \"error_count\": " << stats.error_count << ",\n";
        out_ << "      \"warning_count\": " << stats.warning_count << ",\n";
        out_ << "      \"info_count\": " << stats.info_count << ",\n";
        out_ << "      \"other_count\": " << stats.other_count << "\n";
        out_ << "    }";
    }
    
    out_ << "\n  ]";
    reset_comma(); // Reset for next section
}

void JsonOutput::output_patterns(const std::vector<std::pair<std::string, std::size_t>>& pattern_counts) {
    if (pattern_counts.empty()) {
        return;
    }
    
    comma_if_needed();
    out_ << "\n  \"pattern_matches\": [";
    
    reset_comma();
    for (const auto& [pattern, count] : pattern_counts) {
        comma_if_needed();
        out_ << "\n    {\n";
        out_ << "      \"pattern\": \"" << escape_json(pattern) << "\",\n";
        out_ << "      \"matches\": " << count << "\n";
        out_ << "    }";
    }
    
    out_ << "\n  ]";
}

void JsonOutput::output_complete(const analytics::LogAggregator& aggregator,
                                 std::size_t total_lines, std::size_t total_bytes,
                                 double processing_time_ms, double throughput_mb_per_min,
                                 bool include_time_windows) {
    reset_comma();
    out_ << "{\n";
    
    // Summary
    output_summary(total_lines, total_bytes, processing_time_ms, throughput_mb_per_min);
    
    // Histogram
    const auto& histogram = aggregator.histogram();
    auto level_counts = histogram.get_all_counts();
    if (!level_counts.empty()) {
        output_histogram(level_counts, histogram.total());
    }
    
    // Time windows
    if (include_time_windows) {
        auto window_stats = aggregator.time_windows().get_all_stats();
        if (!window_stats.empty()) {
            output_time_windows(window_stats);
        }
    }
    
    // Patterns (pattern_matcher() is non-const, but get_match_counts() is const)
    // Use const_cast since we're only reading
    auto pattern_counts = const_cast<analytics::LogAggregator&>(aggregator).pattern_matcher().get_match_counts();
    if (!pattern_counts.empty()) {
        output_patterns(pattern_counts);
    }
    
    out_ << "\n}\n";
}

} // namespace log_analyzer::utils
