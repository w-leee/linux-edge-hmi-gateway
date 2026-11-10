#include "MqttManager.hpp"
#include <spdlog/spdlog.h>

MqttManager::MqttManager(const std::string& ip, int port) : broker_ip(ip), broker_port(port) {
    mosquitto_lib_init();
    mosq = mosquitto_new("gateway_client_01", true, NULL);
}

MqttManager::~MqttManager() {
    if (mosq) {
        mosquitto_disconnect(mosq);
        mosquitto_destroy(mosq);
    }
    mosquitto_lib_cleanup();
}

void MqttManager::connectBroker() {
    if (!mosq) return;
    int rc = mosquitto_connect(mosq, broker_ip.c_str(), broker_port, 60);
    if (rc != MOSQ_ERR_SUCCESS) {
        spdlog::error("MQTT 连接失败: {}", mosquitto_strerror(rc));
    } else {
        spdlog::info("【MQTT】成功连接 Broker {}:{}", broker_ip, broker_port);
    }
    mosquitto_loop_start(mosq);
}

bool MqttManager::publish(const std::string& topic, const std::string& payload) {
    if (!mosq) return false;
    int rc = mosquitto_publish(mosq, NULL, topic.c_str(), payload.length(), payload.c_str(), 0, false);
    if (rc == MOSQ_ERR_SUCCESS) {
        spdlog::info("【MQTT】上云成功: {}", payload);
        return true;
    }
    return false;
}
