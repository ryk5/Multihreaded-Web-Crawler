#include "include/crawler/url_frontier.h"

bool URLFrontier::try_add(const std::string& url, std::chrono::milliseconds timeout){
    // Need to add check for validity and normality first, will do later in utils

    // Check visited set with read lock first
    {
        std::shared_lock<std::shared_mutex> read_lock(visited_mutex_);
        if (visited_.count(url) > 0) {
            duplicates_skipped_++;
            return false;
        }
    } // shared_lock goes out of scope here, destructor called, unlocked

    // Not visited
    {
        std::unique_lock<std::shared_mutex> write_lock(visited_mutex_);
        auto [it, inserted] = visited_.insert(url);
        if (!inserted){
            duplicates_skipped_++;
            return false;
        }
    }

    // Successfully marked as visited, add to Bounded Queue
    if (queue_.push(url, timeout)){
        urls_added_++;
        return true;
    }
    // If unsuccessful, return false
    return false;
}

bool URLFrontier::try_add_nowait(const std::string& url){
    // Need to add check for validity and normality first, will do later in utils

    // Check visited set with read lock first
    {
        std::shared_lock<std::shared_mutex> read_lock(visited_mutex_);
        if (visited_.count(url) > 0) {
            duplicates_skipped_++;
            return false;
        }
    } // shared_lock goes out of scope here, destructor called, unlocked

    // Not visited
    {
        std::unique_lock<std::shared_mutex> write_lock(visited_mutex_);
        auto [it, inserted] = visited_.insert(url);
        if (!inserted){
            duplicates_skipped_++;
            return false;
        }
    }

    // Successfully marked as visited, add to Bounded Queue with no 
    if (queue_.try_push(url)){
        urls_added_++;
        return true;
    }
    // If unsuccessful, return false
    return false;
}

size_t URLFrontier::add_batch(const std::vector<std::string>& urls){
    size_t added = 0;
    for(const auto& url : urls) {
        if(try_add_nowait(url)) added++;
    }
    return added;
}

std::optional<std::string> URLFrontier::pop(std::chrono::milliseconds timeout){
    return queue_.pop(timeout);
}

bool URLFrontier::is_visited(const std::string& url) const {
    std::shared_lock<std::shared_mutex> lock(visited_mutex_);
    return visited_.count(url) > 0;
}

void URLFrontier::mark_visited(const std::string& url) {
    std::unique_lock<std::shared_mutex> lock(visited_mutex_);
    visited_.insert(url);
}

void URLFrontier::shutdown() {
    queue_.shutdown();
}

size_t URLFrontier::visited_count() const {
    std::shared_lock<std::shared_mutex> lock(visited_mutex_);
    return visited_.size();
}

bool URLFrontier::is_visited_internal(const std::string& normalized_url) const {
    return visited_.count(normalized_url) > 0;
}

bool URLFrontier::add_to_visited(const std::string& normalized_url) {
    auto [it, inserted] = visited_.insert(normalized_url);
    return inserted;
}