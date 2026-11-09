#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>    // 🚨 新增：用于配置真实物理串口
#include <spdlog/spdlog.h>
#include <string>
#include <thread>
#include <atomic>
#include <chrono>
#include <cstdio>       // 🚨 新增：用于 sscanf 解析
#include <mosquitto.h>
#include "RingQueue.hpp"
#include <yaml-cpp/yaml.h>
#include "httplib.h"

// 全局共享变量
RingQueue<std::string> data_queue(10);
std::atomic<bool> running(true); 
std::atomic<int> msg_count{0};   // 记录处理的消息总数

// 🚨 全局变量存储最新的温湿度 (使用 atomic 保证多线程安全)
std::atomic<double> latest_temp{0.0};
std::atomic<double> latest_hum{0.0};

struct mosquitto *mosq = nullptr;

// 初始化并连接 MQTT Broker
void init_mqtt(const std::string& ip, int port) {
    mosquitto_lib_init();
    mosq = mosquitto_new("gateway_client_01", true, NULL);
    if (!mosq) {
        spdlog::error("MQTT 客户端创建失败");
        return;
    }
    
    int rc = mosquitto_connect(mosq, ip.c_str(), port, 60);
    if (rc != MOSQ_ERR_SUCCESS) {
        spdlog::error("MQTT 连接失败，请确认 mosquitto 服务已启动: {}", mosquitto_strerror(rc));
    } else {
        spdlog::info("【MQTT】成功连接云平台 Broker {}:{}", ip, port);
    }
    mosquitto_loop_start(mosq);
}

// 生产者：读串口（包含断线重连、粘包处理、以及真实串口波特率配置）
void serial_reader_task(const std::string& port_name) {
    spdlog::info("生产者线程: 开始监听串口 {}", port_name);
    
    while (running) {
        int fd = open(port_name.c_str(), O_RDWR | O_NOCTTY); 
        if (fd == -1) {
            spdlog::warn("串口 {} 不可用，3秒后重试...", port_name);
            std::this_thread::sleep_for(std::chrono::seconds(3));
            continue;
        }
        spdlog::info("生产者线程: 成功打开串口 {}", port_name);

        // 🚨 配置物理串口参数（波特率 115200, 8N1）
        struct termios tty;
        if (tcgetattr(fd, &tty) == 0) {
            cfsetospeed(&tty, B115200); // 设置输出波特率 115200
            cfsetispeed(&tty, B115200); // 设置输入波特率 115200
            tty.c_cflag &= ~PARENB;     // 无校验位
            tty.c_cflag &= ~CSTOPB;     // 1位停止位
            tty.c_cflag &= ~CSIZE;      // 清除数据位设置
            tty.c_cflag |= CS8;         // 8位数据位
            tty.c_cflag &= ~CRTSCTS;    // 无硬件流控
            tty.c_cflag |= CREAD | CLOCAL; // 打开接收器，忽略调制解调器控制线
            tcsetattr(fd, TCSANOW, &tty);
            spdlog::info("串口参数配置成功: 波特率 115200, 8N1");
        } else {
            spdlog::error("串口参数配置失败");
        }

        char buffer[256];
        std::string remain_data = "";

        while (running) {
            int n = read(fd, buffer, sizeof(buffer) - 1);
            if (n > 0) {
                buffer[n] = '\0';
                remain_data += buffer; 
                
                // 按换行符拆包
                size_t pos = 0;
                while ((pos = remain_data.find('\n')) != std::string::npos) {
                    std::string complete_line = remain_data.substr(0, pos);
                    if (!complete_line.empty() && complete_line.back() == '\r') {
                        complete_line.pop_back();
                    }
                    if (!complete_line.empty()) {
                        data_queue.push(complete_line); 
                    }
                    remain_data.erase(0, pos + 1);
                }
            } else if (n < 0) {
                break; 
            }
        }
        close(fd);
        spdlog::warn("串口 {} 连接断开", port_name);
    }
}

// 消费者：处理并上云，同时解析温湿度
void data_process_task(const std::string& topic) {
    spdlog::info("消费者线程: 启动，等待处理数据...");
    while (running) {
        std::string data = data_queue.pop();

        while (!data.empty() && (data.back() == '\n' || data.back() == '\r')) {
            data.pop_back();
        }

        if (data.empty()) continue;
        
        spdlog::info("[处理业务] 收到数据: {}", data);

        // 🚨 解析温湿度，格式为 "temp: 22.20, hum: 43.00"
        double t, h;
        if (sscanf(data.c_str(), "temp: %lf, hum: %lf", &t, &h) == 2) {
            latest_temp.store(t);
            latest_hum.store(h);
        }
        
        // 组装 JSON
        std::string json_payload = "{\"device\":\"gateway_01\", \"data\":\"" + data + "\"}";
        
        // 发布到 MQTT 主题
        if (mosq) {
            int rc = mosquitto_publish(mosq, NULL, topic.c_str(), json_payload.length(), json_payload.c_str(), 0, false);
            if (rc == MOSQ_ERR_SUCCESS) {
                msg_count++; 
                spdlog::info("【MQTT】上云成功: {}", json_payload);
            } else {
                spdlog::error("【MQTT】上云失败");
            }
        }
    }
}

// HTTP 服务线程（提供监控接口）
void http_server_task() {
    httplib::Server svr;
    
    // 接口 1：健康检查
    svr.Get("/health", [](const httplib::Request&, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_content("OK", "text/plain");
    });
    
    // 接口 2：监控指标 (保留作为后台监控)
    svr.Get("/metrics", [](const httplib::Request&, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        std::string metrics = "gateway_msg_total " + std::to_string(msg_count.load()) + "\n";
        res.set_content(metrics, "text/plain");
    });

    // 🚨 接口 3：温湿度数据接口 (专供 Qt 前端使用)
    svr.Get("/api/data", [](const httplib::Request&, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        char buf[64];
        // 格式化为 "22.2,43.0" 返回
        snprintf(buf, sizeof(buf), "%.1f,%.1f", latest_temp.load(), latest_hum.load());
        res.set_content(buf, "text/plain");
    });
    
    spdlog::info("【HTTP】监控服务已启动，监听端口 8080");
    svr.listen("0.0.0.0", 8080); 
}

int main() {
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [thread %t] %v");
    spdlog::info("Edge Gateway 启动...");

    std::string serial_port;
    std::string mqtt_ip;
    int mqtt_port;
    std::string mqtt_topic;

    try {
        YAML::Node config = YAML::LoadFile("config.yaml");
        serial_port = config["gateway"]["serial_port"].as<std::string>();
        mqtt_ip = config["gateway"]["mqtt"]["broker_ip"].as<std::string>();
        mqtt_port = config["gateway"]["mqtt"]["broker_port"].as<int>();
        mqtt_topic = config["gateway"]["mqtt"]["topic"].as<std::string>();
        spdlog::info("成功加载配置: 串口={}, MQTT={}:{}, Topic={}", serial_port, mqtt_ip, mqtt_port, mqtt_topic);
    } catch (const std::exception& e) {
        spdlog::error("读取配置文件失败: {}", e.what());
        return -1;
    }

    init_mqtt(mqtt_ip, mqtt_port);

    std::thread consumer(data_process_task, mqtt_topic);
    std::thread producer(serial_reader_task, serial_port);
    std::thread http_thread(http_server_task); 

    producer.join();
    
    running = false;
    data_queue.push(""); 
    consumer.join();

    if (mosq) {
        mosquitto_disconnect(mosq);
        mosquitto_destroy(mosq);
    }
    mosquitto_lib_cleanup();
    
    http_thread.detach(); 

    return 0;
}