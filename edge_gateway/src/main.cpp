#include <iostream>
#include <spdlog/spdlog.h>
#include <yaml-cpp/yaml.h>
#include <thread>
#include <cstdio>
#include "RingQueue.hpp"
#include "GlobalState.hpp"
#include "SerialManager.hpp"
#include "MqttManager.hpp"
#include "HttpServerManager.hpp"

int main() {
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

    // 4. 消费者线程
    spdlog::info("消费者线程: 启动");
    std::thread consumer([&]() {
        while (true) {
            std::string data = data_queue.pop();
            if (data.empty()) continue;
            
            spdlog::info("[处理业务] 收到数据: {}", data);
            
            // 解析温湿度
            double t, h;
            if (sscanf(data.c_str(), "temp: %lf, hum: %lf", &t, &h) == 2) {
                g_latest_temp.store(t);
                g_latest_hum.store(h);
            }
            
            // 发 MQTT
            std::string json = "{\"device\":\"gateway_01\", \"data\":\"" + data + "\"}";
            if (mqtt.publish(mqtt_topic, json)) {
                g_msg_count++;
            }
        }
    });

    consumer.join(); // 阻塞主线程
    return 0;
}
