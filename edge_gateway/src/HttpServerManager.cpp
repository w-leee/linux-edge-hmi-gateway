#include "HttpServerManager.hpp"
#include "GlobalState.hpp"
#include <spdlog/spdlog.h>
#include <cstdio>

HttpServerManager::HttpServerManager() { svr = std::make_shared<httplib::Server>(); }
HttpServerManager::~HttpServerManager() { stop(); }

void HttpServerManager::start() {
    svr->Get("/health", [](const httplib::Request&, httplib::Response& res) {
        res.set_content("OK", "text/plain");
    });
    
    svr->Get("/metrics", [](const httplib::Request&, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        std::string metrics = "gateway_msg_total " + std::to_string(g_msg_count.load()) + "\n";
        res.set_content(metrics, "text/plain");
    });

    svr->Get("/api/data", [](const httplib::Request&, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        char buf[64];
        snprintf(buf, sizeof(buf), "%.1f,%.1f", g_latest_temp.load(), g_latest_hum.load());
        res.set_content(buf, "text/plain");
    });

    running = true;
    server_thread = std::thread([this]() {
        spdlog::info("【HTTP】监控服务已启动，监听端口 8080");
        svr->listen("0.0.0.0", 8080);
    });
}

void HttpServerManager::stop() {
    if (running) {
        svr->stop();
        if (server_thread.joinable()) {
            server_thread.join();
        }
        running = false;
    }
}
