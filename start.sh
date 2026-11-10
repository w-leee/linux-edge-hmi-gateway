#!/bin/bash

# 定义终端颜色
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' 

ROOT_DIR=$(cd "$(dirname "$0")" && pwd)
cd "$ROOT_DIR"

# 🚨 定义退出清理函数：不管怎么退出，都把后台的 mqtt 和 docker 停掉
cleanup() {
    echo -e "\n${YELLOW}🛑 捕获到退出信号，正在清理后台服务...${NC}"
    docker stop my_gateway 2>/dev/null
    sudo pkill -9 mosquitto 2>/dev/null
    echo -e "${GREEN}👋 系统已完全退出，环境已清理。${NC}"
    exit 0
}
trap cleanup INT TERM

echo -e "${GREEN}=========================================${NC}"
echo -e "${GREEN}   Edge Gateway 全栈一键启动脚本 v3.0   ${NC}"
echo -e "${GREEN}=========================================${NC}"

# 1. 检查物理串口是否已挂载
echo -e "${YELLOW}[1/4] 正在检查物理串口...${NC}"
SERIAL_DEV=$(ls /dev/ttyUSB* 2>/dev/null | head -n 1)
if [ -z "$SERIAL_DEV" ]; then
    echo -e "${RED}❌ 错误: 未找到任何 /dev/ttyUSB* 设备！${NC}"
    echo -e "请先在 Windows PowerShell (管理员) 中执行:"
    echo -e "usbipd attach --wsl --busid 2-2"
    exit 1
fi
echo -e "${GREEN}✅ 发现串口: $SERIAL_DEV${NC}"

# 2. 启动 MQTT Broker (后台静默运行)
echo -e "${YELLOW}[2/4] 正在启动 MQTT Broker...${NC}"
sudo pkill -9 mosquitto 2>/dev/null
echo -e "listener 1883 0.0.0.0\nallow_anonymous true" > /tmp/mosq.conf
mosquitto -c /tmp/mosq.conf -d # -d 代表后台运行
sleep 1
echo -e "${GREEN}✅ MQTT Broker 已在 1883 端口运行${NC}"

# 3. 启动 Docker 容器
echo -e "${YELLOW}[3/4] 正在启动 Docker 网关容器...${NC}"
# 检查容器是否存在，如果不存在则提示构建
if ! docker ps -a --format '{{.Names}}' | grep -q '^my_gateway$'; then
    echo -e "${RED}❌ 未找到 my_gateway 容器，请先执行构建命令！${NC}"
    exit 1
fi
docker start my_gateway > /dev/null
echo -e "${GREEN}✅ Docker 网关已启动 (后台运行)${NC}"

# 4. 启动 Qt 监控大屏 (前台运行)
echo -e "${YELLOW}[4/4] 正在启动 Qt 监控大屏...${NC}"
cd "$ROOT_DIR/qt_dashboard"
./build/qt_dashboard

# 当用户关闭 Qt 窗口时，触发 cleanup
cleanup
