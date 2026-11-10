#pragma once
#include <atomic>

extern std::atomic<double> g_latest_temp;
extern std::atomic<double> g_latest_hum;
extern std::atomic<int> g_msg_count;
