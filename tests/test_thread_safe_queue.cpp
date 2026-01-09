#include <gtest/gtest.h>
#include "utils/thread_safe_queue.hpp"
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>

using namespace log_analyzer::utils;

class ThreadSafeQueueTest : public ::testing::Test {
protected:
    void SetUp() override {
        queue_ = std::make_unique<ThreadSafeQueue>(100);
    }

    void TearDown() override {
        queue_.reset();
    }

    std::unique_ptr<ThreadSafeQueue> queue_;
};

// Test basic push and pop
TEST_F(ThreadSafeQueueTest, BasicPushPop) {
    queue_->push("Line 1");
    queue_->push("Line 2");
    queue_->push("Line 3");
    
    EXPECT_EQ(queue_->size(), 3);
    
    auto line1 = queue_->pop();
    EXPECT_EQ(line1, "Line 1");
    
    auto line2 = queue_->pop();
    EXPECT_EQ(line2, "Line 2");
    
    auto line3 = queue_->pop();
    EXPECT_EQ(line3, "Line 3");
    
    EXPECT_EQ(queue_->size(), 0);
    EXPECT_TRUE(queue_->empty());
}

// Test try_push and try_pop
TEST_F(ThreadSafeQueueTest, TryPushPop) {
    bool success1 = queue_->try_push("Line 1");
    EXPECT_TRUE(success1);
    
    bool success2 = queue_->try_push("Line 2");
    EXPECT_TRUE(success2);
    
    std::string line;
    bool pop_success1 = queue_->try_pop(line);
    EXPECT_TRUE(pop_success1);
    EXPECT_EQ(line, "Line 1");
    
    bool pop_success2 = queue_->try_pop(line);
    EXPECT_TRUE(pop_success2);
    EXPECT_EQ(line, "Line 2");
    
    bool pop_success3 = queue_->try_pop(line);
    EXPECT_FALSE(pop_success3);
}

// Test close functionality
TEST_F(ThreadSafeQueueTest, Close) {
    queue_->push("Line 1");
    queue_->push("Line 2");
    
    EXPECT_FALSE(queue_->is_closed());
    
    queue_->close();
    
    EXPECT_TRUE(queue_->is_closed());
    
    // Should still be able to pop existing items
    auto line1 = queue_->pop();
    EXPECT_EQ(line1, "Line 1");
    
    auto line2 = queue_->pop();
    EXPECT_EQ(line2, "Line 2");
    
    // After queue is empty and closed, pop should return empty string
    auto line3 = queue_->pop();
    EXPECT_TRUE(line3.empty());
}

// Test producer-consumer pattern
TEST_F(ThreadSafeQueueTest, ProducerConsumer) {
    const int num_items = 100;
    std::atomic<int> produced{0};
    std::atomic<int> consumed{0};
    
    // Producer thread
    std::thread producer([&]() {
        for (int i = 0; i < num_items; ++i) {
            queue_->push("Line " + std::to_string(i));
            produced++;
        }
        queue_->close();
    });
    
    // Consumer thread
    std::thread consumer([&]() {
        while (true) {
            std::string line = queue_->pop();
            if (line.empty() && queue_->is_closed()) {
                break;
            }
            if (!line.empty()) {
                consumed++;
            }
        }
    });
    
    producer.join();
    consumer.join();
    
    EXPECT_EQ(produced.load(), num_items);
    EXPECT_EQ(consumed.load(), num_items);
}

// Test multiple consumers
TEST_F(ThreadSafeQueueTest, MultipleConsumers) {
    const int num_items = 1000;
    const int num_consumers = 4;
    std::atomic<int> produced{0};
    std::atomic<int> consumed{0};
    
    // Producer thread
    std::thread producer([&]() {
        for (int i = 0; i < num_items; ++i) {
            queue_->push("Line " + std::to_string(i));
            produced++;
        }
        queue_->close();
    });
    
    // Multiple consumer threads
    std::vector<std::thread> consumers;
    for (int i = 0; i < num_consumers; ++i) {
        consumers.emplace_back([&]() {
            while (true) {
                std::string line = queue_->pop();
                if (line.empty() && queue_->is_closed()) {
                    break;
                }
                if (!line.empty()) {
                    consumed++;
                }
            }
        });
    }
    
    producer.join();
    for (auto& consumer : consumers) {
        consumer.join();
    }
    
    EXPECT_EQ(produced.load(), num_items);
    EXPECT_EQ(consumed.load(), num_items);
}

// Test queue size limit
TEST_F(ThreadSafeQueueTest, QueueSizeLimit) {
    ThreadSafeQueue small_queue(2); // Very small queue
    
    bool success1 = small_queue.try_push("Line 1");
    EXPECT_TRUE(success1);
    
    bool success2 = small_queue.try_push("Line 2");
    EXPECT_TRUE(success2);
    
    bool success3 = small_queue.try_push("Line 3");
    EXPECT_FALSE(success3); // Should fail - queue is full
    
    EXPECT_EQ(small_queue.size(), 2);
}

// Test empty check
TEST_F(ThreadSafeQueueTest, EmptyCheck) {
    EXPECT_TRUE(queue_->empty());
    
    queue_->push("Line 1");
    EXPECT_FALSE(queue_->empty());
    
    queue_->pop();
    EXPECT_TRUE(queue_->empty());
}

// Test size tracking
TEST_F(ThreadSafeQueueTest, SizeTracking) {
    EXPECT_EQ(queue_->size(), 0);
    
    queue_->push("Line 1");
    EXPECT_EQ(queue_->size(), 1);
    
    queue_->push("Line 2");
    EXPECT_EQ(queue_->size(), 2);
    
    queue_->pop();
    EXPECT_EQ(queue_->size(), 1);
    
    queue_->pop();
    EXPECT_EQ(queue_->size(), 0);
}
