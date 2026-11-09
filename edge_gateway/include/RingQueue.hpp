#pragma once
#include <vector>
#include <mutex>
#include <condition_variable>

template <typename T>
class RingQueue {
public:
    explicit RingQueue(size_t capacity) : capacity_(capacity), buffer_(capacity) {}

    // 生产者：入队
    void push(const T& item) {
        std::unique_lock<std::mutex> lock(mutex_);
        // 如果队列满了，就阻塞等待，直到消费者取走数据
        not_full_.wait(lock, [this]() { return size_ < capacity_; });
        
        buffer_[write_idx_] = item;
        write_idx_ = (write_idx_ + 1) % capacity_;
        size_++;
        
        // 唤醒可能在等待的消费者
        not_empty_.notify_one();
    }

    // 消费者：出队
    T pop() {
        std::unique_lock<std::mutex> lock(mutex_);
        // 如果队列空了，就阻塞等待，直到生产者放入数据
        not_empty_.wait(lock, [this]() { return size_ > 0; });
        
        T item = buffer_[read_idx_];
        read_idx_ = (read_idx_ + 1) % capacity_;
        size_--;
        
        // 唤醒可能在等待的生产者
        not_full_.notify_one();
        return item;
    }

private:
    size_t capacity_;
    std::vector<T> buffer_;
    size_t read_idx_ = 0;
    size_t write_idx_ = 0;
    size_t size_ = 0;
    
    std::mutex mutex_;
    std::condition_variable not_full_;
    std::condition_variable not_empty_;
};