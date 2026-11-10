#pragma once
#include <thread>
#include <atomic>
#include <memory>
#include "httplib.h"

class HttpServerManager {
public:
    HttpServerManager();
    ~HttpServerManager();
    void start();
    void stop();

private:
    std::atomic<bool> running{false};
    std::thread server_thread;
    std::shared_ptr<httplib::Server> svr;
};
