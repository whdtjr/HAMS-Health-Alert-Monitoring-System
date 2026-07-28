#pragma once

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace hams {

template <typename T>
class BlockingQueue {
public:
    explicit BlockingQueue(std::size_t capacity) : capacity_{capacity} {
        if (capacity == 0) {
            throw std::invalid_argument{"queue capacity must be greater than zero"};
        }
    }

    BlockingQueue(const BlockingQueue&) = delete;
    BlockingQueue& operator=(const BlockingQueue&) = delete;

    bool push(T value) {
        std::unique_lock lock{mutex_};
        notFull_.wait(lock, [this] {
            return closed_ || queue_.size() < capacity_;
        });

        if (closed_) {
            return false;
        }

        queue_.push_back(std::move(value));
        lock.unlock();
        notEmpty_.notify_one();
        return true;
    }

    bool pushOverwrite(T value) {
        std::lock_guard lock{mutex_};
        if (closed_) {
            return false;
        }

        if (queue_.size() == capacity_) {
            queue_.pop_front();
        }
        queue_.push_back(std::move(value));
        notEmpty_.notify_one();
        return true;
    }

    std::optional<T> pop() {
        std::unique_lock lock{mutex_};
        notEmpty_.wait(lock, [this] {
            return closed_ || !queue_.empty();
        });

        if (queue_.empty()) {
            return std::nullopt;
        }

        T value = std::move(queue_.front());
        queue_.pop_front();
        lock.unlock();
        notFull_.notify_one();
        return value;
    }

    [[nodiscard]] std::vector<T> snapshot() const {
        std::lock_guard lock{mutex_};
        return {queue_.begin(), queue_.end()};
    }

    [[nodiscard]] std::size_t size() const {
        std::lock_guard lock{mutex_};
        return queue_.size();
    }

    void close() {
        {
            std::lock_guard lock{mutex_};
            closed_ = true;
        }
        notEmpty_.notify_all();
        notFull_.notify_all();
    }

private:
    const std::size_t capacity_;
    mutable std::mutex mutex_;
    std::condition_variable notEmpty_;
    std::condition_variable notFull_;
    std::deque<T> queue_;
    bool closed_{false};
};

}  // namespace hams
