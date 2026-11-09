# Linux Edge Gateway & Qt HMI Monitoring System

基于 C++17、Linux 系统编程与 Qt/QML 开发的边缘计算网关系统。通过物理硬件采集环境数据，经过多线程并发处理后上云，并实时展示在本地 HMI 监控大屏上。

<img width="1440" height="1920" alt="Camera_1040g3qo327bagkcamgc05nd1ogcg85ojou1p3bg" src="https://github.com/user-attachments/assets/9271e3c9-19c6-4cf8-8d02-0199340df0dd" />

## 🏗️ 系统架构图

```text
[ESP32 + DHT11] ──(物理温湿度数据)
       │ USB
       ▼
[Windows USB] ──(usbipd 硬件穿透)
       ▼
[WSL2 /dev/ttyUSB0] ──(termios 波特率 115200)
       ▼
[生产者线程 (read)] ──(按 \n 拆包解决粘包)
       ▼
[线程安全环形队列 (RingQueue)]
       ▼
[消费者线程 (解析数据)]
       ├──► [MQTT Client] ──(JSON 格式上云)
       └──► [HTTP Server] ──(暴露 /api/data 接口)
                ▲
                │ (每 5 秒异步请求)
                │
[Qt/QML HMI 监控大屏] ──(信号槽机制实时刷新 UI)
```

## ⚡ 一键启动与自动化脚本

为了提升开发与测试效率，本项目提供了一个全自动化的启动脚本 `start.sh`。该脚本会自动完成串口权限修复、依赖检查、项目编译，并在后台拉起网关服务与前端监控面板。


**1. 物理设备挂载 (Windows PowerShell - 管理员)**
将物理串口控制权从 Windows 移交给 WSL2：
usbipd list
usbipd attach --wsl --busid <你的BUSID>

**2.linux环境下启动**
cd ~/workspace/Edge-Gateway-System
./start.sh
