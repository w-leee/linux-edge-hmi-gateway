#pragma once
#include <string>
#include <mosquitto.h>

class MqttManager {
public:
    MqttManager(const std::string& ip, int port);
    ~MqttManager();
    void connectBroker();
    bool publish(const std::string& topic, const std::string& payload);

private:
    std::string broker_ip;
    int broker_port;
    struct mosquitto* mosq{nullptr};
};
