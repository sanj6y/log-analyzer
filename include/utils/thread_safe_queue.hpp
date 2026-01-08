#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>
#include <string_view>
#include <vector>

namespace log_analyzer::utils {

/**
 * Thread-safe queue for producer-consumer model.
 * Uses std::string_view for zero-copy operations.
 * Manages lifetime of string data internally.
 */
class ThreadSafeQueue {
public:
    explicit ThreadSafeQueue(std::size_t max_size = 10000);
    ~ThreadSafeQueue() = default;

    // Non-copyable, movable
    ThreadSafeQueue(const ThreadSafeQueue&) = delete;
    ThreadSafeQueue& operator=(const ThreadSafeQueue&) = delete;
    ThreadSafeQueue(ThreadSafeQueue&&) = delete;
    ThreadSafeQueue& operator=(ThreadSafeQueue&&) = delete;

    /**
     * Push a line into the queue (producer operation).
     * Blocks if queue is full until space is available.
     * Takes ownership of the string data.
     */
    void push(std::string line);

    /**
     * Try to push without blocking.
     * Returns true if successful, false if queue is full.
     */
    bool try_push(std::string line);

    /**
     * Pop a line from the queue (consumer operation).
     * Blocks if queue is empty until data is available.
     * Returns empty string if queue is closed and empty.
     */
    std::string pop();

    /**
     * Try to pop without blocking.
     * Returns true if successful, false if queue is empty.
     */
    bool try_pop(std::string& line);

    /**
     * Mark queue as closed (no more data will be added).
     * This allows consumers to finish processing and exit.
     */
    void close();

    /**
     * Check if queue is closed.
     */
    [[nodiscard]] bool is_closed() const noexcept;

    /**
     * Get current size (approximate, thread-safe).
     */
    [[nodiscard]] std::size_t size() const noexcept;

    /**
     * Check if queue is empty (approximate, thread-safe).
     */
    [[nodiscard]] bool empty() const noexcept;

private:
    mutable std::mutex mutex_;
    std::condition_variable not_empty_;
    std::condition_variable not_full_;
    std::queue<std::string> queue_;
    std::size_t max_size_;
    bool closed_;

    /**
     * Internal helper to check if queue has space.
     */
    [[nodiscard]] bool has_space() const noexcept;
};

/**
 * Thread-safe ring buffer for zero-copy string_view operations.
 * Pre-allocates buffer chunks to minimize allocations.
 */
class ThreadSafeRingBuffer {
public:
    explicit ThreadSafeRingBuffer(std::size_t chunk_size = 64 * 1024, 
                                   std::size_t num_chunks = 16);
    ~ThreadSafeRingBuffer() = default;

    // Non-copyable, movable
    ThreadSafeRingBuffer(const ThreadSafeRingBuffer&) = delete;
    ThreadSafeRingBuffer& operator=(const ThreadSafeRingBuffer&) = delete;
    ThreadSafeRingBuffer(ThreadSafeRingBuffer&&) = delete;
    ThreadSafeRingBuffer& operator=(ThreadSafeRingBuffer&&) = delete;

    /**
     * Store a line and return a string_view to it.
     * Returns empty view if buffer is full.
     */
    std::string_view store_line(std::string_view line);

    /**
     * Get current chunk being written to (for producer).
     */
    std::string& current_chunk();

    /**
     * Advance to next chunk when current is full.
     */
    void advance_chunk();

private:
    mutable std::mutex mutex_;
    std::vector<std::string> chunks_;
    std::size_t chunk_size_;
    std::size_t write_chunk_idx_;
    std::size_t write_pos_;
    std::size_t read_chunk_idx_;
    std::size_t read_pos_;
};

} // namespace log_analyzer::utils
