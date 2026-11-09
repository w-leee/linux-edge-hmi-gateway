#!/bin/bash

# 1. 清理之前可能残留的进程
pkill socat 2>/dev/null
pkill mosquitto_sub 2>/dev/null
# 如果你之前后台跑了 systemd 服务，顺手停掉它，防止端口冲突
sudo systemctl stop edge-gateway 2>/dev/null

echo ">>> 正在启动虚拟硬件..."

# 2. 制造固定路径的虚拟串口（路径已与 config.yaml 绝对对齐）
socat -d -d pty,raw,echo=0,link=/home/wangle/workspace/edge_gateway/gateway_serial pty,raw,echo=0,link=/home/wangle/workspace/edge_gateway/sensor_serial > /dev/null 2>&1 &
sleep 1 # 给系统1秒钟时间生成串口

# 3. 启动云端监控大屏（在后台跑，日志输出到当前屏幕）
echo ">>> 正在连接云端大屏..."
mosquitto_sub -t "edge/device/data" -v &

# 4. 启动你的边缘网关
echo ">>> 正在启动边缘网关核心程序..."
./build/gateway

# 5. 退出清理逻辑
trap "echo '正在清理后台进程...'; pkill socat; pkill mosquitto_sub; exit" INT