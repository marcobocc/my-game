#pragma once
#include <array>
#include <mutex>
#include <vector>

template<typename T>
class ConsoleBuffer {
    static constexpr size_t MAX_MESSAGES = 1024;

public:
    void push(T&& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (count_ < MAX_MESSAGES) {
            buffer_[count_++] = std::move(value);
            newMessages_++;
        }
    }

    void push(const T& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (count_ < MAX_MESSAGES) {
            buffer_[count_++] = value;
            newMessages_++;
        }
    }

    std::vector<T> consume() {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<T> out;
        out.reserve(count_);
        for (size_t i = 0; i < count_; ++i)
            out.push_back(std::move(buffer_[i]));
        count_ = 0;
        newMessages_ = 0;
        return out;
    }

    size_t newMessageCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return newMessages_;
    }

private:
    mutable std::mutex mutex_;
    std::array<T, MAX_MESSAGES> buffer_{};
    size_t count_ = 0;
    size_t newMessages_ = 0;
};
