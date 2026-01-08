#include "utils/thread_safe_queue.hpp"
#include <cstring>

namespace log_analyzer::utils {

ThreadSafeQueue::ThreadSafeQueue(std::size_t max_size)
    : max_size_(max_size)
    , closed_(false)
{
}

void ThreadSafeQueue::push(std::string line) {
    std::unique_lock<std::mutex> lock(mutex_);
    
    // Wait until there's space
    not_full_.wait(lock, [this] { return has_space() || closed_; });
    
    if (closed_) {
        return;
    }
    
    queue_.push(std::move(line));
    not_empty_.notify_one();
}

bool ThreadSafeQueue::try_push(std::string line) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!has_space() || closed_) {
        return false;
    }
    
    queue_.push(std::move(line));
    not_empty_.notify_one();
    return true;
}

std::string ThreadSafeQueue::pop() {
    std::unique_lock<std::mutex> lock(mutex_);
    
    // Wait until there's data or queue is closed
    not_empty_.wait(lock, [this] { return !queue_.empty() || closed_; });
    
    if (queue_.empty() && closed_) {
        return {}; // Queue is closed and empty
    }
    
    std::string line = std::move(queue_.front());
    queue_.pop();
    not_full_.notify_one();
    
    return line;
}

bool ThreadSafeQueue::try_pop(std::string& line) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (queue_.empty()) {
        return false;
    }
    
    line = std::move(queue_.front());
    queue_.pop();
    not_full_.notify_one();
    return true;
}

void ThreadSafeQueue::close() {
    std::lock_guard<std::mutex> lock(mutex_);
    closed_ = true;
    not_empty_.notify_all();
    not_full_.notify_all();
}

bool ThreadSafeQueue::is_closed() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return closed_;
}

std::size_t ThreadSafeQueue::size() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();
}

bool ThreadSafeQueue::empty() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.empty();
}

bool ThreadSafeQueue::has_space() const noexcept {
    return queue_.size() < max_size_;
}

// ThreadSafeRingBuffer implementation
ThreadSafeRingBuffer::ThreadSafeRingBuffer(std::size_t chunk_size, std::size_t num_chunks)
    : chunk_size_(chunk_size)
    , write_chunk_idx_(0)
    , write_pos_(0)
    , read_chunk_idx_(0)
    , read_pos_(0)
{
    chunks_.reserve(num_chunks);
    for (std::size_t i = 0; i < num_chunks; ++i) {
        chunks_.emplace_back(chunk_size_, '\0');
    }
}

std::string_view ThreadSafeRingBuffer::store_line(std::string_view line) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (write_pos_ + line.size() + 1 > chunk_size_) {
        // Not enough space in current chunk
        return {};
    }
    
    std::string& chunk = chunks_[write_chunk_idx_];
    std::memcpy(chunk.data() + write_pos_, line.data(), line.size());
    chunk[write_pos_ + line.size()] = '\0';
    
    std::string_view view(chunk.data() + write_pos_, line.size());
    write_pos_ += line.size() + 1;
    
    return view;
}

std::string& ThreadSafeRingBuffer::current_chunk() {
    std::lock_guard<std::mutex> lock(mutex_);
    return chunks_[write_chunk_idx_];
}

void ThreadSafeRingBuffer::advance_chunk() {
    std::lock_guard<std::mutex> lock(mutex_);
    write_chunk_idx_ = (write_chunk_idx_ + 1) % chunks_.size();
    write_pos_ = 0;
}

} // namespace log_analyzer::utils
