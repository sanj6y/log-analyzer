#include <gtest/gtest.h>
#include "parser/log_parser.hpp"
#include <string>
#include <vector>

using namespace log_analyzer::parser;

class LogParserTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code if needed
    }

    void TearDown() override {
        // Cleanup code if needed
    }
};

// Test split_fields with space delimiter
TEST_F(LogParserTest, SplitFieldsSpaceDelimiter) {
    std::string_view line = "ERROR: Connection timeout at line 1234";
    auto fields = LogParser::split_fields(line, ' ');
    
    ASSERT_EQ(fields.size(), 6); // All space-separated tokens
    EXPECT_EQ(fields[0], "ERROR:");
    EXPECT_EQ(fields[1], "Connection");
    EXPECT_EQ(fields[2], "timeout");
    EXPECT_EQ(fields[3], "at");
    EXPECT_EQ(fields[4], "line");
    EXPECT_EQ(fields[5], "1234");
}

// Test split_fields with custom delimiter
TEST_F(LogParserTest, SplitFieldsCustomDelimiter) {
    std::string_view line = "2024-01-07|10:00:00|ERROR|Connection failed";
    auto fields = LogParser::split_fields(line, '|');
    
    ASSERT_EQ(fields.size(), 4);
    EXPECT_EQ(fields[0], "2024-01-07");
    EXPECT_EQ(fields[1], "10:00:00");
    EXPECT_EQ(fields[2], "ERROR");
    EXPECT_EQ(fields[3], "Connection failed");
}

// Test split_fields with multiple consecutive delimiters
TEST_F(LogParserTest, SplitFieldsMultipleDelimiters) {
    std::string_view line = "  ERROR   WARNING  INFO  ";
    auto fields = LogParser::split_fields(line, ' ');
    
    ASSERT_EQ(fields.size(), 3);
    EXPECT_EQ(fields[0], "ERROR");
    EXPECT_EQ(fields[1], "WARNING");
    EXPECT_EQ(fields[2], "INFO");
}

// Test find substring
TEST_F(LogParserTest, FindSubstring) {
    std::string_view line = "ERROR: Connection timeout occurred";
    std::size_t pos = LogParser::find(line, "timeout");
    
    EXPECT_NE(pos, std::string_view::npos);
    EXPECT_GT(pos, 0);
}

// Test find non-existent substring
TEST_F(LogParserTest, FindNonExistentSubstring) {
    std::string_view line = "ERROR: Connection timeout occurred";
    std::size_t pos = LogParser::find(line, "nonexistent");
    
    EXPECT_EQ(pos, std::string_view::npos);
}

// Test starts_with
TEST_F(LogParserTest, StartsWith) {
    std::string_view line = "ERROR: Something went wrong";
    EXPECT_TRUE(LogParser::starts_with(line, "ERROR:"));
    EXPECT_FALSE(LogParser::starts_with(line, "WARNING:"));
}

// Test contains
TEST_F(LogParserTest, Contains) {
    std::string_view line = "ERROR: Connection timeout occurred";
    EXPECT_TRUE(LogParser::contains(line, "timeout"));
    EXPECT_TRUE(LogParser::contains(line, "Connection"));
    EXPECT_FALSE(LogParser::contains(line, "nonexistent"));
}

// Test trim
TEST_F(LogParserTest, Trim) {
    std::string_view line1 = "  ERROR  ";
    auto trimmed1 = LogParser::trim(line1);
    EXPECT_EQ(trimmed1, "ERROR");
    
    std::string_view line2 = "\t\n  WARNING  \t\n";
    auto trimmed2 = LogParser::trim(line2);
    EXPECT_EQ(trimmed2, "WARNING");
    
    std::string_view line3 = "INFO";
    auto trimmed3 = LogParser::trim(line3);
    EXPECT_EQ(trimmed3, "INFO");
}

// Test extract_timestamp
TEST_F(LogParserTest, ExtractTimestamp) {
    std::string_view line1 = "2024-01-07 10:00:00 ERROR: Something";
    auto timestamp1 = LogParser::extract_timestamp(line1);
    EXPECT_FALSE(timestamp1.empty());
    EXPECT_TRUE(timestamp1.starts_with("2024-01-07"));
    
    std::string_view line2 = "2024-01-07 10:00:00.123 ERROR: Something";
    auto timestamp2 = LogParser::extract_timestamp(line2);
    EXPECT_FALSE(timestamp2.empty());
    
    std::string_view line3 = "No timestamp here";
    auto timestamp3 = LogParser::extract_timestamp(line3);
    // May be empty or may extract something - depends on implementation
}

// Test parse_integer (if charconv is available)
#if LOG_ANALYZER_HAS_CHARCONV
TEST_F(LogParserTest, ParseInteger) {
    std::string_view str1 = "12345";
    auto [value1, success1] = LogParser::parse_integer<int>(str1);
    EXPECT_TRUE(success1);
    EXPECT_EQ(value1, 12345);
    
    std::string_view str2 = "-42";
    auto [value2, success2] = LogParser::parse_integer<int>(str2);
    EXPECT_TRUE(success2);
    EXPECT_EQ(value2, -42);
    
    std::string_view str3 = "invalid";
    auto [value3, success3] = LogParser::parse_integer<int>(str3);
    EXPECT_FALSE(success3);
}
#endif
