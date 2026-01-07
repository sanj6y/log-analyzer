#pragma once

#include <cstddef>
#include <memory>
#include <vector>

namespace log_analyzer::utils {

/**
 * Memory pool allocator for zero-copy operations.
 * Pre-allocates fixed-size blocks to eliminate heap allocations in hot paths.
 */
class MemoryPool {
public:
    explicit MemoryPool(std::size_t block_size = 64 * 1024, std::size_t initial_blocks = 4);
    ~MemoryPool() = default;

    // Non-copyable, movable
    MemoryPool(const MemoryPool&) = delete;
    MemoryPool& operator=(const MemoryPool&) = delete;
    MemoryPool(MemoryPool&&) noexcept = default;
    MemoryPool& operator=(MemoryPool&&) noexcept = default;

    /**
     * Allocate a block from the pool.
     * Returns nullptr if pool is exhausted (should not happen in normal operation).
     */
    void* allocate();

    /**
     * Return a block to the pool.
     */
    void deallocate(void* ptr);

    /**
     * Get the block size.
     */
    [[nodiscard]] std::size_t block_size() const noexcept { return block_size_; }

    /**
     * Get current pool statistics.
     */
    [[nodiscard]] std::size_t allocated_blocks() const noexcept { return allocated_count_; }
    [[nodiscard]] std::size_t total_blocks() const noexcept { return blocks_.size(); }

private:
    std::size_t block_size_;
    std::vector<std::unique_ptr<std::byte[]>> blocks_;
    std::vector<void*> free_list_;
    std::size_t allocated_count_;

    void grow_pool();
};

} // namespace log_analyzer::utils
