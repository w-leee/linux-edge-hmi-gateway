cmake_minimum_required(VERSION 3.16)
project(EdgeGateway LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Threads REQUIRED)
find_package(spdlog REQUIRED)
find_package(yaml-cpp REQUIRED)

add_executable(gateway 
    src/main.cpp
    src/GlobalState.cpp
    src/SerialManager.cpp
    src/MqttManager.cpp
    src/EpollServer.cpp  # 🚨 替换为了 EpollServer
)

target_include_directories(gateway PRIVATE include)
target_link_libraries(gateway PRIVATE Threads::Threads spdlog::spdlog mosquitto yaml-cpp)

# 单元测试
enable_testing()
find_package(GTest REQUIRED)
add_executable(test_ring_queue tests/test_ring_queue.cpp)
target_include_directories(test_ring_queue PRIVATE include)
target_link_libraries(test_ring_queue PRIVATE Threads::Threads GTest::gtest GTest::gtest_main)
