#pragma once
#include <string>
#include <atomic>
#include <thread>
#include "RingQueue.hpp"

class SerialManager {
public:
    SerialManager(const std::string& port, RingQueue<std::string>& queue);
    ~SerialManager();
    void start();
    void stop(); // 🚨 停止并唤醒阻塞

private:
    void readLoop();
    std::string port_name;
    RingQueue<std::string>& data_queue;
    std::atomic<bool> running{false};
    std::thread worker_thread;
    std::atomic<int> fd{-1}; // 🚨 改为原子操作，方便其他线程强制关闭，打断阻塞的 read
};