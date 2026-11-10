#!/bin/bash

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

echo -e "${GREEN}=========================================${NC}"
echo -e "${GREEN}   Edge Gateway & HMI 一键启动脚本 v2.0  ${NC}"
echo -e "${GREEN}=========================================${NC}"

echo -e "${YELLOW}[1/6] 正在检查物理串口...${NC}"
SERIAL_DEV=$(ls /dev/ttyUSB* 2>/dev/null | head -n 1)

if [ -z "$SERIAL_DEV" ]; then
    echo -e "${RED}❌ 错误: 未找到任何 /dev/ttyUSB* 设备！${NC}"
    echo -e "请先在 Windows PowerShell (管理员) 中执行: usbipd attach --wsl --busid 2-2"
    exit 1
fi
echo -e "${GREEN}✅ 发现串口设备: $SERIAL_DEV${NC}"

echo -e "${YELLOW}[2/6] 正在修复串口权限...${NC}"
sudo chmod 666 $SERIAL_DEV
echo -e "${GREEN}✅ 串口权限已修复${NC}"

echo -e "${YELLOW}[3/6] 清理旧进程...${NC}"
pkill -9 gateway 2>/dev/null
pkill -9 qt_dashboard 2>/dev/null
sleep 0.5

echo -e "${YELLOW}[4/6] 正在编译项目代码...${NC}"
cd edge_gateway
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug > /dev/null
cmake --build build -j4 > /dev/null
echo -e "${GREEN}✅ C++ 网关编译成功${NC}"
cd ..

cd qt_dashboard
cmake -B build -S . > /dev/null
cmake --build build -j4 > /dev/null
echo -e "${GREEN}✅ Qt 大屏编译成功${NC}"
cd ..

echo -e "${YELLOW}[5/6] 正在启动 C++ 边缘网关...${NC}"
cd edge_gateway
./build/gateway > ../gateway.log 2>&1 &
GATEWAY_PID=$!
cd ..
sleep 1

if ! ps -p $GATEWAY_PID > /dev/null; then
    echo -e "${RED}❌ 错误: 网关启动失败，请查看 gateway.log 日志！${NC}"
    exit 1
fi
echo -e "${GREEN}✅ 网关启动成功 (PID: $GATEWAY_PID)${NC}"

echo -e "${YELLOW}[6/6] 正在启动 Qt 监控大屏...${NC}"
cd qt_dashboard
./build/qt_dashboard

echo -e "${YELLOW}🛑 检测到大屏已关闭，正在清理后台网关进程...${NC}"
kill -9 $GATEWAY_PID 2>/dev/null
echo -e "${GREEN}👋 系统已完全退出。${NC}"
