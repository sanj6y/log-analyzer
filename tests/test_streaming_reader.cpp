#include <gtest/gtest.h>
#include "utils/streaming_reader.hpp"
#include <fstream>
#include <filesystem>
#include <string>

using namespace log_analyzer::utils;

class StreamingReaderTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a temporary test file
        test_file_ = "test_log.txt";
        std::ofstream file(test_file_);
        file << "Line 1\n";
        file << "Line 2\n";
        file << "Line 3\n";
        file << "This is a longer line with more content\n";
        file << "Final line";
        file.close();
    }

    void TearDown() override {
        // Clean up test file
        if (std::filesystem::exists(test_file_)) {
            std::filesystem::remove(test_file_);
        }
    }

    std::string test_file_;
};

// Test basic file reading
TEST_F(StreamingReaderTest, BasicReading) {
    StreamingReader reader(test_file_, 1024);
    
    ASSERT_TRUE(reader.is_open());
    
    auto line1 = reader.next_line();
    EXPECT_EQ(line1, "Line 1");
    
    auto line2 = reader.next_line();
    EXPECT_EQ(line2, "Line 2");
    
    auto line3 = reader.next_line();
    EXPECT_EQ(line3, "Line 3");
}

// Test reading all lines
TEST_F(StreamingReaderTest, ReadAllLines) {
    StreamingReader reader(test_file_, 1024);
    
    std::vector<std::string> lines;
    while (!reader.eof()) {
        auto line = reader.next_line();
        if (!line.empty()) {
            lines.push_back(std::string(line));
        }
    }
    
    ASSERT_EQ(lines.size(), 5);
    EXPECT_EQ(lines[0], "Line 1");
    EXPECT_EQ(lines[1], "Line 2");
    EXPECT_EQ(lines[2], "Line 3");
    EXPECT_EQ(lines[4], "Final line");
}

// Test EOF detection
TEST_F(StreamingReaderTest, EOFDetection) {
    StreamingReader reader(test_file_, 1024);
    
    // Read all lines
    while (!reader.eof()) {
        auto line = reader.next_line();
        (void)line; // Suppress unused warning
    }
    
    // Next read should return empty
    auto line = reader.next_line();
    EXPECT_TRUE(line.empty());
    EXPECT_TRUE(reader.eof());
}

// Test with small buffer (forces multiple reads)
TEST_F(StreamingReaderTest, SmallBuffer) {
    StreamingReader reader(test_file_, 16); // Very small buffer
    
    std::vector<std::string> lines;
    while (!reader.eof()) {
        auto line = reader.next_line();
        if (!line.empty()) {
            lines.push_back(std::string(line));
        }
    }
    
    ASSERT_EQ(lines.size(), 5);
}

// Test bytes_read counter
TEST_F(StreamingReaderTest, BytesRead) {
    StreamingReader reader(test_file_, 1024);
    
    // Read a few lines
    auto line1 = reader.next_line();
    auto line2 = reader.next_line();
    (void)line1; // Suppress unused warning
    (void)line2; // Suppress unused warning
    
    std::size_t bytes = reader.bytes_read();
    EXPECT_GT(bytes, 0);
    EXPECT_LT(bytes, 1000); // Should be small for our test file
}

// Test with non-existent file
TEST_F(StreamingReaderTest, NonExistentFile) {
    StreamingReader reader("nonexistent_file.txt", 1024);
    
    EXPECT_FALSE(reader.is_open());
    EXPECT_TRUE(reader.eof());
}

// Test empty file
TEST_F(StreamingReaderTest, EmptyFile) {
    std::string empty_file = "empty_test.txt";
    std::ofstream file(empty_file);
    file.close();
    
    StreamingReader reader(empty_file, 1024);
    
    EXPECT_TRUE(reader.is_open());
    auto line = reader.next_line();
    EXPECT_TRUE(line.empty());
    EXPECT_TRUE(reader.eof());
    
    std::filesystem::remove(empty_file);
}

// Test file with only newlines
TEST_F(StreamingReaderTest, OnlyNewlines) {
    std::string newline_file = "newline_test.txt";
    std::ofstream file(newline_file);
    file << "\n\n\n";
    file.close();
    
    StreamingReader reader(newline_file, 1024);
    
    int empty_lines = 0;
    while (!reader.eof()) {
        auto line = reader.next_line();
        if (line.empty() && !reader.eof()) {
            empty_lines++;
        }
    }
    
    EXPECT_EQ(empty_lines, 3);
    
    std::filesystem::remove(newline_file);
}
