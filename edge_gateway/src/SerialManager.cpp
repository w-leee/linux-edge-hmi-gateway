#include "SerialManager.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <spdlog/spdlog.h>
#include <chrono>
#include <cstdio>
#include <cstring>

SerialManager::SerialManager(const std::string& port, RingQueue<std::string>& queue)
    : port_name(port), data_queue(queue) {}

SerialManager::~SerialManager() { 
    stop(); 
}

void SerialManager::start() {
    running = true;
    worker_thread = std::thread(&SerialManager::readLoop, this);
}

void SerialManager::stop() {
    if (!running.exchange(false)) return; // 如果已经停止了，直接返回
    
    // 🚨 核心：强行关闭文件描述符，这将立刻打断阻塞中的 read()
    int current_fd = fd.exchange(-1);
    if (current_fd != -1) {
        close(current_fd);
    }
    
    if (worker_thread.joinable()) {
        worker_thread.join();
    }
    spdlog::info("SerialManager: 已优雅停止");
}

void SerialManager::readLoop() {
    spdlog::info("SerialManager: 开始监听串口 {}", port_name);
    
    std::string last_valid_data = ""; 
    auto last_time = std::chrono::steady_clock::now();

    while (running) {
        int local_fd = open(port_name.c_str(), O_RDWR | O_NOCTTY); 
        if (local_fd == -1) {
            spdlog::warn("串口 {} 不可用，3秒后重试...", port_name);
            std::this_thread::sleep_for(std::chrono::seconds(3));
            continue;
        }
        
        fd.store(local_fd); // 保存到原子变量中
        
        struct termios tty;
        if (tcgetattr(local_fd, &tty) == 0) {
            cfsetospeed(&tty, B115200);
            cfsetispeed(&tty, B115200);
            tty.c_cflag &= ~PARENB; tty.c_cflag &= ~CSTOPB;
            tty.c_cflag &= ~CSIZE; tty.c_cflag |= CS8;
            tty.c_cflag &= ~CRTSCTS; tty.c_cflag |= CREAD | CLOCAL;
            tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
            tty.c_iflag &= ~(IXON | IXOFF | IXANY);
            tcsetattr(local_fd, TCSANOW, &tty);
            tcflush(local_fd, TCIFLUSH); 
        }

        char buffer[256];
        std::string remain_data = "";

        while (running) {
            int n = read(local_fd, buffer, sizeof(buffer) - 1);
            if (n > 0) {
                buffer[n] = '\0';
                remain_data += buffer; 
                
                size_t pos = 0;
                while ((pos = remain_data.find('\n')) != std::string::npos) {
                    std::string complete_line = remain_data.substr(0, pos);
                    if (!complete_line.empty() && complete_line.back() == '\r') complete_line.pop_back();
                    
                    double t, h;
                    if (sscanf(complete_line.c_str(), "temp: %lf, hum: %lf", &t, &h) == 2) {
                        auto now = std::chrono::steady_clock::now();
                        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_time).count();
                        
                        if (!(complete_line == last_valid_data && duration < 500)) {
                            data_queue.push(complete_line); 
                            last_valid_data = complete_line;
                            last_time = now;
                        }
                    }
                    remain_data.erase(0, pos + 1);
                }
            } else if (n < 0) {
                break; // 被 stop() 强行打断，退出内层循环
            }
        }
        
        if (fd.exchange(-1) != -1) { 
            close(local_fd); 
        }
        
        if(running) {
            spdlog::warn("串口连接断开，尝试重连...");
            std::this_thread::sleep_for(std::chrono::seconds(3));
        }
    }
}