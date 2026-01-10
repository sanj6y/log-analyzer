#pragma once

#include <cstddef>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>

namespace log_analyzer::utils {

/**
 * Streaming file reader for multi-GB log files.
 * Maintains constant memory footprint regardless of file size.
 * Supports both file and stdin input (use "-" for stdin).
 */
class StreamingReader {
public:
    explicit StreamingReader(std::string_view filepath, std::size_t buffer_size = 1024 * 1024);
    ~StreamingReader();

    // Non-copyable, movable
    StreamingReader(const StreamingReader&) = delete;
    StreamingReader& operator=(const StreamingReader&) = delete;
    StreamingReader(StreamingReader&&) noexcept;
    StreamingReader& operator=(StreamingReader&&) noexcept;

    /**
     * Read the next line from the file.
     * Returns empty string_view if EOF reached.
     * Uses zero-copy approach by returning views into internal buffer.
     */
    [[nodiscard]] std::string_view next_line();

    /**
     * Check if file is open and valid (or reading from stdin).
     */
    [[nodiscard]] bool is_open() const noexcept { return file_.is_open() || read_from_stdin_; }

    /**
     * Check if EOF has been reached.
     */
    [[nodiscard]] bool eof() const noexcept;

    /**
     * Get current file position.
     * Returns 0 for stdin (position tracking not available).
     */
    [[nodiscard]] std::streampos tellg() { 
        if (read_from_stdin_) {
            return 0; // Position not available for stdin
        }
        return file_.tellg(); 
    }

    /**
     * Get bytes read so far.
     */
    [[nodiscard]] std::size_t bytes_read() const noexcept { return total_bytes_read_; }

private:
    std::ifstream file_;
    std::istream* input_stream_;  // Points to file_ or &std::cin
    bool read_from_stdin_;
    std::size_t buffer_size_;
    std::unique_ptr<char[]> buffer_;
    std::size_t buffer_pos_;
    std::size_t buffer_used_;
    std::size_t total_bytes_read_;

    /**
     * Refill buffer from file.
     * Returns true if more data available, false on EOF.
     */
    bool refill_buffer();

    /**
     * Find next newline in current buffer starting from pos.
     * Returns position of newline or buffer_used_ if not found.
     */
    std::size_t find_newline(std::size_t start_pos) const;
};

} // namespace log_analyzer::utils
