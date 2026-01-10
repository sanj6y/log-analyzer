#pragma once

#include "analytics/log_aggregator.hpp"
#include <iostream>
#include <string>
#include <vector>

namespace log_analyzer::utils {

/**
 * JSON output formatter for analytics results.
 */
class JsonOutput {
public:
    JsonOutput(std::ostream& os) : out_(os) {}
    
    /**
     * Output processing summary as JSON.
     */
    void output_summary(std::size_t total_lines, std::size_t total_bytes, 
                       double processing_time_ms, double throughput_mb_per_min);
    
    /**
     * Output log level histogram as JSON.
     */
    void output_histogram(const std::vector<std::pair<std::string, std::size_t>>& level_counts,
                         std::size_t total);
    
    /**
     * Output time window statistics as JSON.
     */
    void output_time_windows(const std::vector<analytics::TimeWindowAnalyzer::WindowStats>& windows);
    
    /**
     * Output pattern matches as JSON.
     */
    void output_patterns(const std::vector<std::pair<std::string, std::size_t>>& pattern_counts);
    
    /**
     * Output complete results as JSON object.
     */
    void output_complete(const analytics::LogAggregator& aggregator,
                        std::size_t total_lines, std::size_t total_bytes,
                        double processing_time_ms, double throughput_mb_per_min,
                        bool include_time_windows = false);
    
    /**
     * Escape string for JSON output.
     */
    static std::string escape_json(const std::string& str);

private:
    std::ostream& out_;
    bool first_item_ = true;
    
    void comma_if_needed();
    void reset_comma();
};

} // namespace log_analyzer::utils
