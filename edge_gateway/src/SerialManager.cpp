#include "SerialManager.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <spdlog/spdlog.h>
#include <chrono>
#include <cstdio>       // 🚨 用于 sscanf 解析
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
    running = false;
    if (fd != -1) {
        close(fd);
        fd = -1;
    }
    if (worker_thread.joinable()) {
        worker_thread.join();
    }
}

void SerialManager::readLoop() {
    spdlog::info("SerialManager: 开始监听串口 {}", port_name);
    
    // 🚨 用于去重的变量
    std::string last_valid_data = ""; 
    auto last_time = std::chrono::steady_clock::now();

    while (running) {
        fd = open(port_name.c_str(), O_RDWR | O_NOCTTY); 
        if (fd == -1) {
            spdlog::warn("串口 {} 不可用，3秒后重试...", port_name);
            std::this_thread::sleep_for(std::chrono::seconds(3));
            continue;
        }
        
        // 配置物理串口参数 (115200, 8N1)
        struct termios tty;
        if (tcgetattr(fd, &tty) == 0) {
            cfsetospeed(&tty, B115200);
            cfsetispeed(&tty, B115200);
            tty.c_cflag &= ~PARENB; // 无校验
            tty.c_cflag &= ~CSTOPB; // 1位停止位
            tty.c_cflag &= ~CSIZE;  // 清除数据位设置
            tty.c_cflag |= CS8;     // 8位数据位
            tty.c_cflag &= ~CRTSCTS;// 无硬件流控
            tty.c_cflag |= CREAD | CLOCAL; // 打开接收器
            
            // 忽略控制字符，防止特殊字符导致终端卡死
            tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
            tty.c_iflag &= ~(IXON | IXOFF | IXANY);
            
            tcsetattr(fd, TCSANOW, &tty);
            
            // 🚨 核心1：清空底层缓冲区里的历史乱码（ESP32重启时的Bootloader日志）
            tcflush(fd, TCIFLUSH); 
        }

        char buffer[256];
        std::string remain_data = "";

        while (running) {
            int n = read(fd, buffer, sizeof(buffer) - 1);
            if (n > 0) {
                buffer[n] = '\0';
                remain_data += buffer; 
                
                size_t pos = 0;
                while ((pos = remain_data.find('\n')) != std::string::npos) {
                    std::string complete_line = remain_data.substr(0, pos);
                    if (!complete_line.empty() && complete_line.back() == '\r') complete_line.pop_back();
                    
                    // 🚨 核心2：强校验！必须完美匹配 "temp: %lf, hum: %lf" 才放行，乱码直接被丢弃
                    double t, h;
                    if (sscanf(complete_line.c_str(), "temp: %lf, hum: %lf", &t, &h) == 2) {
                        
                        // 🚨 核心3：去重！防止 WSL2 底层重传导致一次数据进两次
                        auto now = std::chrono::steady_clock::now();
                        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_time).count();
                        
                        if (complete_line == last_valid_data && duration < 500) {
                            // 重复数据，直接丢弃 (忽略)
                        } else {
                            // 有效且不重复的数据，放入环形队列
                            data_queue.push(complete_line); 
                            last_valid_data = complete_line;
                            last_time = now;
                        }
                    }
                    
                    // 把已处理的数据从缓冲区删掉
                    remain_data.erase(0, pos + 1);
                }
            } else if (n < 0) {
                break; 
            }
        }
        
        if (fd != -1) { 
            close(fd); 
            fd = -1; 
        }
        if(running) {
            spdlog::warn("串口 {} 连接断开，尝试重连...", port_name);
        }
    }
}