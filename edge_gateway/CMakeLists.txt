cmake_minimum_required(VERSION 3.16)
project(EdgeGateway LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Threads REQUIRED)
find_package(spdlog REQUIRED)
find_package(yaml-cpp REQUIRED)

add_executable(gateway src/main.cpp)
target_include_directories(gateway PRIVATE include)

# 🚨 这里加上了双冒号
target_link_libraries(gateway PRIVATE Threads::Threads spdlog::spdlog mosquitto yaml-cpp::yaml-cpp)