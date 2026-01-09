#include <gtest/gtest.h>
#include "analytics/log_aggregator.hpp"
#include <string>

using namespace log_analyzer::analytics;

class LogHistogramTest : public ::testing::Test {
protected:
    void SetUp() override {
        histogram_ = std::make_unique<LogHistogram>();
    }

    void TearDown() override {
        histogram_.reset();
    }

    std::unique_ptr<LogHistogram> histogram_;
};

// Test basic increment
TEST_F(LogHistogramTest, BasicIncrement) {
    histogram_->increment("ERROR");
    histogram_->increment("ERROR");
    histogram_->increment("WARNING");
    
    EXPECT_EQ(histogram_->count("ERROR"), 2);
    EXPECT_EQ(histogram_->count("WARNING"), 1);
    EXPECT_EQ(histogram_->count("INFO"), 0);
    EXPECT_EQ(histogram_->total(), 3);
}

// Test total count
TEST_F(LogHistogramTest, TotalCount) {
    EXPECT_EQ(histogram_->total(), 0);
    
    histogram_->increment("ERROR");
    EXPECT_EQ(histogram_->total(), 1);
    
    histogram_->increment("WARNING");
    histogram_->increment("INFO");
    EXPECT_EQ(histogram_->total(), 3);
}

// Test get_all_counts
TEST_F(LogHistogramTest, GetAllCounts) {
    histogram_->increment("ERROR");
    histogram_->increment("ERROR");
    histogram_->increment("ERROR");
    histogram_->increment("WARNING");
    histogram_->increment("INFO");
    
    auto counts = histogram_->get_all_counts();
    
    ASSERT_GE(counts.size(), 3);
    
    // Find ERROR count (should be highest)
    bool found_error = false;
    for (const auto& [level, count] : counts) {
        if (level == "ERROR") {
            EXPECT_EQ(count, 3);
            found_error = true;
        }
    }
    EXPECT_TRUE(found_error);
}

// Test reset
TEST_F(LogHistogramTest, Reset) {
    histogram_->increment("ERROR");
    histogram_->increment("WARNING");
    EXPECT_EQ(histogram_->total(), 2);
    
    histogram_->reset();
    
    EXPECT_EQ(histogram_->total(), 0);
    EXPECT_EQ(histogram_->count("ERROR"), 0);
    EXPECT_EQ(histogram_->count("WARNING"), 0);
}

// Test multiple log levels
TEST_F(LogHistogramTest, MultipleLevels) {
    histogram_->increment("ERROR");
    histogram_->increment("WARNING");
    histogram_->increment("INFO");
    histogram_->increment("DEBUG");
    histogram_->increment("TRACE");
    
    EXPECT_EQ(histogram_->total(), 5);
    EXPECT_EQ(histogram_->count("ERROR"), 1);
    EXPECT_EQ(histogram_->count("WARNING"), 1);
    EXPECT_EQ(histogram_->count("INFO"), 1);
    EXPECT_EQ(histogram_->count("DEBUG"), 1);
    EXPECT_EQ(histogram_->count("TRACE"), 1);
}

// Test case sensitivity
TEST_F(LogHistogramTest, CaseSensitivity) {
    histogram_->increment("ERROR");
    histogram_->increment("error");
    histogram_->increment("Error");
    
    EXPECT_EQ(histogram_->count("ERROR"), 1);
    EXPECT_EQ(histogram_->count("error"), 1);
    EXPECT_EQ(histogram_->count("Error"), 1);
    EXPECT_EQ(histogram_->total(), 3);
}

// Test with LogAggregator integration
TEST_F(LogHistogramTest, AggregatorIntegration) {
    LogAggregator aggregator(60);
    
    aggregator.process_line("2024-01-07 10:00:00 ERROR: Connection failed");
    aggregator.process_line("2024-01-07 10:00:01 WARNING: Retry attempt");
    aggregator.process_line("2024-01-07 10:00:02 ERROR: Timeout occurred");
    aggregator.process_line("2024-01-07 10:00:03 INFO: Process started");
    
    const auto& histogram = aggregator.histogram();
    
    EXPECT_EQ(histogram.count("ERROR"), 2);
    EXPECT_EQ(histogram.count("WARNING"), 1);
    EXPECT_EQ(histogram.count("INFO"), 1);
    EXPECT_EQ(histogram.total(), 4);
}
