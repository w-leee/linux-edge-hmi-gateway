#include "GlobalState.hpp"

std::atomic<double> g_latest_temp{0.0};
std::atomic<double> g_latest_hum{0.0};
std::atomic<int> g_msg_count{0};
