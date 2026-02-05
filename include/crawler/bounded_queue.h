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

    /**
     * Pushes an item into the queue. Blocks if the queue is full.
     * @param item The item to push (can be lvalue/rvalue)
     * @param timeout Max time to wait, default is "infinite"
     * @return True if item was pushed, false if timeout/shutdown
     */
    bool push(T item, std::chrono::milliseconds timeout = std::chrono::milliseconds::max()) {
        std::unique_lock<std::mutex> lock(mutex_);

        auto deadline = std::chrono::steady_clock::now() + timeout;

        // Wait until there is space or shutdown
        bool success = not_full_.wait_until(lock, deadline, [this] {
            return queue_.size() < capacity_ || shutdown_.load(std::memory_order_acquire)
        })

        if (!success || shutdown_.load(std::memory_order_acquire)) {
            return false;
        }
        
        queue_.push(std::move(item));
        lock.unlock();
        not_empty_.notify_one();
        return true;
    }

    /**
     * Tries to push without blocking
     * @param item The item to push (can be lvalue/rvalue)
     * @return True if item was pushed, false if queue is full/shutdown
     */
    bool try_push(T item){
        std::lock_guard<std::mutex> lock(mutex_);

        if (shutdown_.load(std::memory_order_acquire) || queue_.size() >= capacity_) {
            return false;
        }

        queue_.push(std::move(item));
        not_empty_.notify_one();
        return true;
    }

    /**
     * Pop an item from the queue. Blocks if queue is empty.
     * @param timeout Maximum time to wait
     * @return The item, or nullopt if timeout or shutdown with empty queue
     */
    std::optional<T> pop(std::chrono::milliseconds timeout = std::chrono::milliseconds::max()) {
        std::unique_lock<std::mutex> lock(mutex_);
        
        auto deadline = std::chrono::steady_clock::now() + timeout;
        
        // Wait until there's an item or shutdown
        bool success = not_empty_.wait_until(lock, deadline, [this] {
            return !queue_.empty() || shutdown_.load(std::memory_order_acquire);
        });
        
        if (!success || (queue_.empty() && shutdown_.load(std::memory_order_acquire))) {
            return std::nullopt;
        }
        
        if (queue_.empty()) {
            return std::nullopt;
        }
        
        T item = std::move(queue_.front());
        queue_.pop();
        lock.unlock();
        not_full_.notify_one();
        return item;
    }

    /**
     * Try to pop without blocking.
     * @return The item, or nullopt if queue is empty
     */
    std::optional<T> try_pop() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (queue_.empty()) {
            return std::nullopt;
        }
        
        T item = std::move(queue_.front());
        queue_.pop();
        not_full_.notify_one();
        return item;
    }

    /**
    * Wakes all waiting threads
    * After shutdown, push() returns false and pop() drains remaining items
    */
    void shutdown() {
        shutdown_.store(true, std::memory_order_release);
        not_full_.notify_all();
        not_empty_.notify_all();
    }

    /**
    * Check if shutdown has been signaled
    */
    bool is_shutdown() const {
        return shutdown_.load(std::memory_order_acquire);
    }
    
    /**
    * Get current queue size
    */
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }
    
    /**
    * Check if queue is empty
    */
    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }
    
    /**
    * Get queue capacity
    */
    size_t capacity() const {
        return capacity_;
    }
};