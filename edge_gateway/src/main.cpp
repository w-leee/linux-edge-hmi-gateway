#include <iostream>
#include <spdlog/spdlog.h>
#include <yaml-cpp/yaml.h>
#include <thread>
#include <cstdio>
#include <csignal>
#include <chrono>
#include "RingQueue.hpp"
#include "GlobalState.hpp"
#include "SerialManager.hpp"
#include "MqttManager.hpp"
#include "EpollServer.hpp" // 🚨 替换头文件

std::atomic<bool> g_running{true};

void signalHandler(int signum) {
    spdlog::warn("接收到退出信号 ({}), 准备优雅退出...", signum);
    g_running = false;
}

int main() {
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [thread %t] %v");
    spdlog::info("Edge Gateway 启动...");

    std::string serial_port, mqtt_ip, mqtt_topic;
    int mqtt_port;
    try {
        YAML::Node config = YAML::LoadFile("config.yaml");
        serial_port = config["gateway"]["serial_port"].as<std::string>();
        mqtt_ip = config["gateway"]["mqtt"]["broker_ip"].as<std::string>();
        mqtt_port = config["gateway"]["mqtt"]["broker_port"].as<int>();
        mqtt_topic = config["gateway"]["mqtt"]["topic"].as<std::string>();
    } catch (const std::exception& e) {
        spdlog::error("读取配置文件失败: {}", e.what());
        return -1;
    }

    RingQueue<std::string> data_queue(10);
    SerialManager serial(serial_port, data_queue);
    MqttManager mqtt(mqtt_ip, mqtt_port);
    EpollServer epoll_server(8080); // 🚨 实例化 EpollServer

    mqtt.connectBroker();
    epoll_server.start(); // 🚨 启动 Epoll 网络服务
    serial.start();

    spdlog::info("消费者线程: 启动");
    std::thread consumer([&]() {
        while (g_running || data_queue.size() > 0) { 
            try {
                std::string data = data_queue.pop();
                if (data.empty()) continue;
                
                spdlog::info("[处理业务] 收到数据: {}", data);
                
                double t, h;
                if (sscanf(data.c_str(), "temp: %lf, hum: %lf", &t, &h) == 2) {
                    g_latest_temp.store(t);
                    g_latest_hum.store(h);
                }
                
                std::string json = "{\"device\":\"gateway_01\", \"data\":\"" + data + "\"}";
                if (mqtt.publish(mqtt_topic, json)) {
                    g_msg_count++;
                }
            } catch (...) {
                break;
            }
        }
        spdlog::info("消费者线程: 已退出");
    });

    while (g_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    spdlog::info("【1/3】正在停止串口接收...");
    serial.stop(); 
    
    spdlog::info("【2/3】正在停止 Epoll 网络服务...");
    epoll_server.stop();

    data_queue.push(""); 
    
    if (consumer.joinable()) {
        consumer.join(); 
    }

    spdlog::info("【3/3】资源清理完毕。");
    spdlog::info("Edge Gateway 已安全退出。Bye! 👋");
    return 0;
}
