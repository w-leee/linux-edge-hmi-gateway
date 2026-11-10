#include "EpollServer.hpp"
#include <spdlog/spdlog.h>
#include <cstring>
#include <cstdio>
#include <cerrno>
#include <chrono>
#include <thread>
#include "GlobalState.hpp"

EpollServer::EpollServer(int port) : port_(port), server_fd_(-1), epoll_fd_(-1) {}

EpollServer::~EpollServer() { stop(); }

void EpollServer::start() {
    running_ = true;
    worker_thread_ = std::thread(&EpollServer::run, this);
}

void EpollServer::stop() {
    running_ = false;
    if (server_fd_ != -1) close(server_fd_);
    if (epoll_fd_ != -1) close(epoll_fd_);
    if (worker_thread_.joinable()) worker_thread_.join();
}

void EpollServer::setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void EpollServer::run() {
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    setNonBlocking(server_fd_);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port_);

    if (bind(server_fd_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        spdlog::error("EpollServer: Bind failed on port {}", port_);
        return;
    }
    listen(server_fd_, SOMAXCONN);

    epoll_fd_ = epoll_create1(0);
    struct epoll_event ev, events[10];
    ev.events = EPOLLIN | EPOLLET; // 边缘触发 (ET 模式)
    ev.data.fd = server_fd_;
    epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, server_fd_, &ev);

    spdlog::info("【EpollServer】高并发网络服务已启动，监听端口 {}", port_);

    while (running_) {
        int nfds = epoll_wait(epoll_fd_, events, 10, 100);
        if (nfds < 0) {
            if (errno != EINTR) spdlog::error("epoll_wait error");
            break;
        }

        for (int i = 0; i < nfds; ++i) {
            if (events[i].data.fd == server_fd_) {
                handleNewConnection();
            } else if (events[i].events & EPOLLIN) {
                handleRead(events[i].data.fd);
            }
        }
    }
    spdlog::info("【EpollServer】服务已停止");
}

void EpollServer::handleNewConnection() {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    while (true) {
        int client_fd = accept(server_fd_, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) break; 
        
        setNonBlocking(client_fd);
        struct epoll_event ev;
        ev.events = EPOLLIN | EPOLLET;
        ev.data.fd = client_fd;
        epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, client_fd, &ev);
        spdlog::info("EpollServer: 新客户端连接, fd={}", client_fd);
    }
}

void EpollServer::handleRead(int fd) {
    char buffer[1024];
    while (true) {
        int n = read(fd, buffer, sizeof(buffer) - 1);
        if (n > 0) {
            char response[256];
            snprintf(response, sizeof(response), 
                     "HTTP/1.1 200 OK\r\n"
                     "Content-Type: text/plain\r\n"
                     "Access-Control-Allow-Origin: *\r\n"
                     "Connection: close\r\n"
                     "\r\n"
                     "%.1f,%.1f", 
                     g_latest_temp.load(), g_latest_hum.load());
            
            size_t total = strlen(response);
            size_t written = 0;
            while (written < total) {
                int ret = write(fd, response + written, total - written);
                if (ret > 0) {
                    written += ret;
                } else if (ret < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    continue;
                } else {
                    break;
                }
            }
            
            epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr);
            close(fd);
            spdlog::info("EpollServer: 请求处理完成，主动关闭 fd={}", fd);
            break;
        } else if (n == 0) {
            epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr);
            close(fd);
            break;
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            else break;
        }
    }
}