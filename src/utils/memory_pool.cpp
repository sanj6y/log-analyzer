#include "utils/memory_pool.hpp"
#include <algorithm>
#include <cassert>

namespace log_analyzer::utils {

MemoryPool::MemoryPool(std::size_t block_size, std::size_t initial_blocks)
    : block_size_(block_size)
    , allocated_count_(0)
{
    free_list_.reserve(initial_blocks * 2);
    for (std::size_t i = 0; i < initial_blocks; ++i) {
        grow_pool();
    }
}

void MemoryPool::grow_pool() {
    auto block = std::make_unique<std::byte[]>(block_size_);
    void* ptr = block.get();
    blocks_.push_back(std::move(block));
    free_list_.push_back(ptr);
}

void* MemoryPool::allocate() {
    if (free_list_.empty()) {
        grow_pool();
    }

    void* ptr = free_list_.back();
    free_list_.pop_back();
    ++allocated_count_;
    return ptr;
}

void MemoryPool::deallocate(void* ptr) {
    if (ptr == nullptr) {
        return;
    }

    // Verify the pointer belongs to this pool
    auto it = std::find_if(blocks_.begin(), blocks_.end(),
        [ptr](const auto& block) {
            return block.get() == ptr;
        });

    if (it != blocks_.end()) {
        free_list_.push_back(ptr);
        --allocated_count_;
    }
}

} // namespace log_analyzer::utils
