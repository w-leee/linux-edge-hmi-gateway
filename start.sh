#!/bin/bash

# 🚨 记录脚本所在的绝对路径，防止后面 cd 跑偏
ROOT_DIR=$(cd "$(dirname "$0")" && pwd)
cd "$ROOT_DIR"

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' 

cleanup() {
    cd "$ROOT_DIR" # 🚨 强制回到根目录
    
    echo -e "\n${YELLOW}🛑 捕获到退出信号，正在通知网关优雅退出...${NC}"
    pkill -TERM gateway 2>/dev/null
    sleep 1.5
    
    echo -e "${YELLOW}--- 网关退出日志 ---${NC}"
    if [ -f "gateway.log" ]; then
        tail -n 8 gateway.log
    else
        echo "未找到 gateway.log 文件。"
    fi
    
    pkill -9 gateway 2>/dev/null
    echo -e "${GREEN}👋 系统已完全退出。${NC}"
    exit 0
}

trap cleanup INT TERM

echo -e "${GREEN}=========================================${NC}"
echo -e "${GREEN}   Edge Gateway & HMI 一键启动脚本 v2.4  ${NC}"
echo -e "${GREEN}=========================================${NC}"

echo -e "${YELLOW}[1/5] 正在检查物理串口...${NC}"
SERIAL_DEV=$(ls /dev/ttyUSB* 2>/dev/null | head -n 1)

if [ -z "$SERIAL_DEV" ]; then
    echo -e "${RED}❌ 错误: 未找到任何 /dev/ttyUSB* 设备！${NC}"
    echo -e "请先在 Windows PowerShell (管理员) 中执行: usbipd attach --wsl --busid 2-2"
    exit 1
fi
echo -e "${GREEN}✅ 发现串口设备: $SERIAL_DEV${NC}"

echo -e "${YELLOW}[2/5] 正在修复串口权限...${NC}"
sudo chmod 666 $SERIAL_DEV

echo -e "${YELLOW}[3/5] 正在编译项目代码...${NC}"
cd "$ROOT_DIR/edge_gateway"
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug > /dev/null
cmake --build build -j4 > /dev/null
cd "$ROOT_DIR/qt_dashboard"
cmake -B build -S . > /dev/null
cmake --build build -j4 > /dev/null
cd "$ROOT_DIR"
echo -e "${GREEN}✅ 编译成功${NC}"

echo -e "${YELLOW}[4/5] 正在后台启动 C++ 边缘网关...${NC}"
cd "$ROOT_DIR/edge_gateway"
./build/gateway > "$ROOT_DIR/gateway.log" 2>&1 &
cd "$ROOT_DIR"
sleep 1

echo -e "${YELLOW}[5/5] 正在启动 Qt 监控大屏...${NC}"
cd "$ROOT_DIR/qt_dashboard"
./build/qt_dashboard

cleanup
