#include <iostream>
#include <spdlog/spdlog.h>
#include <yaml-cpp/yaml.h>
#include <thread>
#include <cstdio>
#include <csignal>   // 🚨 引入信号处理
#include <chrono>
#include "RingQueue.hpp"
#include "GlobalState.hpp"
#include "SerialManager.hpp"
#include "MqttManager.hpp"
#include "HttpServerManager.hpp"

// 🚨 全局原子变量，用于控制所有线程的生死
std::atomic<bool> g_running{true};

// 🚨 信号处理函数：捕获 Ctrl+C
void signalHandler(int signum) {
    spdlog::warn("捕获到终止信号 ({})，准备优雅退出...", signum);
    g_running = false;
}

int main() {
    // 🚨 注册信号捕获
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [thread %t] %v");
    spdlog::info("Edge Gateway 启动...");

    // 1. 解析配置
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

    // 2. 实例化各个模块
    RingQueue<std::string> data_queue(10);
    SerialManager serial(serial_port, data_queue);
    MqttManager mqtt(mqtt_ip, mqtt_port);
    HttpServerManager http;

    // 3. 启动模块
    mqtt.connectBroker();
    http.start();
    serial.start();

    // 4. 消费者线程（优化：在退出时处理完剩余数据）
    spdlog::info("消费者线程: 启动");
    std::thread consumer([&]() {
        while (g_running || data_queue.size() > 0) { // 🚨 即使 g_running 为 false，只要队列里还有数据就继续处理
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

    // 🚨 5. 主线程进入等待循环（替代原来的 join）
    while (g_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    // 🚨 6. 清理工作
    spdlog::info("正在停止各模块...");
    serial.stop();  // 停止生产者（关闭串口，唤醒阻塞的 read）
    http.stop();    // 停止 HTTP 服务

    // 给消费者 1 秒钟把队列里剩余的数据发出去
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // 塞入毒丸唤醒消费者线程（如果它卡在 pop 的话）
    data_queue.push(""); 
    consumer.join(); 

    spdlog::info("Edge Gateway 已安全退出。Bye!");
    return 0;
}