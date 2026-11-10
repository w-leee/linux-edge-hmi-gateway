#pragma once
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <string>
#include <thread>
#include <atomic>

class EpollServer {
public:
    EpollServer(int port);
    ~EpollServer();
    void start();
    void stop();

private:
    void run();
    void setNonBlocking(int fd);
    void handleNewConnection();
    void handleRead(int fd);

    int port_;
    int server_fd_;
    int epoll_fd_;
    std::atomic<bool> running_{false};
    std::thread worker_thread_;
};