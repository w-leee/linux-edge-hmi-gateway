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
    void stop();

private:
    void readLoop();
    std::string port_name;
    RingQueue<std::string>& data_queue;
    std::atomic<bool> running{false};
    std::thread worker_thread;
    int fd{-1};
};
