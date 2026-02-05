# pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>
#include <chrono>
#include <atomic>

template<typename T>
class BoundedQueue {
    std::queue<T> queue_;
    mutable std::mutex mutex_;
    std::condition_variable not_full_;
    std::condition_variable not_empty_;
    const size_t capacity_;
    std::atomic<bool> shutdown_{false};

public:
    explicit BoundedQueue(size_t capacity)
        : capacity_(capacity) {}
    
    // Restrict copy/movement since this class contains moutex
    BoundedQueue(const BoundedQueue&) = delete;
    BoundedQueue& operator=(const BoundedQueue&) = delete;
    BoundedQueue(BoundedQueue&&) = delete;
    BoundedQueue& operator=(BoundedQueue&&) = delete;

    // Wait for max timeout milliseconds until there's space or shutdown
    bool push(T item, std::chrono::milliseconds timeout = std::chrono::milliseconds::max()) {
        std::unique_lock<std::mutex> lock(mutex_);
        auto deadline = std::chrono::steady_clock::now() + timeout;
        bool success = not_full_.wait_until(lock, deadline, [this] {
            return queue_.size() < capacity_ || shutdown_.load(std::memory_order_acquire)
        })
        if (!success || shutdown_.load(std::memory_order_acquire)) {
            return false;
        }
        
        queue.push(std::move(item));
        lock.unlock();
        not_empty_.notify_one();
        return true;
    }

    
};