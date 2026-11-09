cmake_minimum_required(VERSION 3.16)
project(QtDashboard LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 COMPONENTS Quick Network REQUIRED)

add_executable(qt_dashboard main.cpp backend.h qml.qrc)
target_link_libraries(qt_dashboard PRIVATE Qt5::Quick Qt5::Network)
