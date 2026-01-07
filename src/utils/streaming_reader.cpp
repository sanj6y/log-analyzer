#include "utils/streaming_reader.hpp"
#include <algorithm>
#include <cstring>
#include <ios>

namespace log_analyzer::utils {

StreamingReader::StreamingReader(std::string_view filepath, std::size_t buffer_size)
    : buffer_size_(buffer_size)
    , buffer_(std::make_unique<char[]>(buffer_size_))
    , buffer_pos_(0)
    , buffer_used_(0)
    , total_bytes_read_(0)
{
    file_.open(std::string(filepath), std::ios::binary);
    if (file_.is_open()) {
        refill_buffer();
    }
}

StreamingReader::~StreamingReader() {
    if (file_.is_open()) {
        file_.close();
    }
}

StreamingReader::StreamingReader(StreamingReader&& other) noexcept
    : file_(std::move(other.file_))
    , buffer_size_(other.buffer_size_)
    , buffer_(std::move(other.buffer_))
    , buffer_pos_(other.buffer_pos_)
    , buffer_used_(other.buffer_used_)
    , total_bytes_read_(other.total_bytes_read_)
{
}

StreamingReader& StreamingReader::operator=(StreamingReader&& other) noexcept {
    if (this != &other) {
        if (file_.is_open()) {
            file_.close();
        }
        file_ = std::move(other.file_);
        buffer_size_ = other.buffer_size_;
        buffer_ = std::move(other.buffer_);
        buffer_pos_ = other.buffer_pos_;
        buffer_used_ = other.buffer_used_;
        total_bytes_read_ = other.total_bytes_read_;
    }
    return *this;
}

bool StreamingReader::eof() const noexcept {
    return !file_.is_open() || (buffer_pos_ >= buffer_used_ && file_.eof());
}

bool StreamingReader::refill_buffer() {
    if (!file_.is_open() || file_.eof()) {
        return false;
    }

    // If we haven't consumed all data, move remaining to start
    std::size_t remaining = buffer_used_ - buffer_pos_;
    if (remaining > 0 && buffer_pos_ > 0) {
        std::memmove(buffer_.get(), buffer_.get() + buffer_pos_, remaining);
    }

    // Read new data into buffer starting at remaining position
    std::size_t read_pos = remaining;
    file_.read(buffer_.get() + read_pos, static_cast<std::streamsize>(buffer_size_ - read_pos));
    std::streamsize bytes_read = file_.gcount();

    buffer_pos_ = 0;
    buffer_used_ = remaining + static_cast<std::size_t>(bytes_read);
    total_bytes_read_ += static_cast<std::size_t>(bytes_read);

    return bytes_read > 0 || remaining > 0;
}

std::size_t StreamingReader::find_newline(std::size_t start_pos) const {
    const char* start = buffer_.get() + start_pos;
    const char* end = buffer_.get() + buffer_used_;
    const char* newline = std::find(start, end, '\n');
    return newline != end ? static_cast<std::size_t>(newline - buffer_.get()) : buffer_used_;
}

std::string_view StreamingReader::next_line() {
    while (true) {
        if (buffer_pos_ >= buffer_used_) {
            if (!refill_buffer()) {
                return {}; // EOF
            }
        }

        std::size_t newline_pos = find_newline(buffer_pos_);
        
        if (newline_pos < buffer_used_) {
            // Found newline in current buffer
            std::size_t line_start = buffer_pos_;
            std::size_t line_length = newline_pos - line_start;
            
            // Skip the newline character
            buffer_pos_ = newline_pos + 1;

            // Handle \r\n (Windows line endings)
            if (line_length > 0 && buffer_[newline_pos - 1] == '\r') {
                --line_length;
            }

            return std::string_view(buffer_.get() + line_start, line_length);
        } else {
            // No newline found, need to refill
            if (!refill_buffer()) {
                // EOF - return remaining buffer as last line
                if (buffer_pos_ < buffer_used_) {
                    std::size_t line_start = buffer_pos_;
                    std::size_t line_length = buffer_used_ - buffer_pos_;
                    buffer_pos_ = buffer_used_;
                    return std::string_view(buffer_.get() + line_start, line_length);
                }
                return {}; // EOF
            }
        }
    }
}

} // namespace log_analyzer::utils
