# 快速开始

[English](../en/getting-started.md) · [文档导航](index.md)

本指南用于构建并部署已验证的开发板配置。准备 Python 3.11、USB 数据线、接好的 ESP32/CC1101
及对应串口驱动。先阅读[硬件说明](hardware.md)。

<!-- section: tools -->
## 安装工具

在仓库根目录的 Windows PowerShell 中执行：

```powershell
python --version
python -m venv .venv
.\.venv\Scripts\python -m pip install -r requirements-lock.txt
.\.venv\Scripts\python -m platformio device list
```

Linux/macOS：

```sh
python3.11 -m venv .venv
.venv/bin/python -m pip install -r requirements-lock.txt
.venv/bin/python -m platformio device list
```

POSIX 系统将后续示例的解释器前缀改为 `.venv/bin/python`。PlatformIO Core 固定为 6.1.19，
espressif32 为 6.12.0，Arduino-ESP32 为 2.0.17。首次构建会下载固定工具链。
Node.js 仅供开发检查及表单登录测试使用，普通固件构建不需要它。

实测 CH343P 开发板出现在 COM4。请使用实际列出的端口；Linux 常见为 ttyUSB 设备。
只有供电功能的 USB 线不会出现串口。如果系统显示未知设备，安装开发板厂商提供的 CH343 驱动，
不要选择无关的主板串口。

<!-- section: spi -->
## 先确认 SPI 通信

核对接线并接好天线后执行：

```powershell
.\.venv\Scripts\python -m platformio run --environment esp32dev
.\.venv\Scripts\python -m platformio run --environment esp32dev --target upload --upload-port COM4
.\.venv\Scripts\python -m platformio device monitor --port COM4 --baud 115200
```

将 COM4 换为实际端口。此诊断固件不发射。需要连续十次合法身份读数并看到最后的 PASS 行：
PARTNUM=0x00、VERSION=0x04 或 0x14、就绪位清零、MARCSTATE=0x01（IDLE）。
全零或全 0xFF 均失败，只有 PARTNUM=0 不能证明成功。其他工具打开端口前，先用 Ctrl+C 关闭监视器。
异常见[故障排查](troubleshooting.md)。

<!-- section: deploy -->
## 部署 Web 固件

仓库包含生成后的头文件。修改源码或页面后，先重新生成再编译：

```powershell
.\.venv\Scripts\python scripts/generate_replay_frames.py
.\.venv\Scripts\python scripts/embed_web.py
.\.venv\Scripts\python -m platformio run --environment phase4
.\.venv\Scripts\python -m platformio run --environment phase4 --target upload --upload-port COM4
.\.venv\Scripts\python scripts/check_phase4_serial.py --port COM4
```

最后一条读取真实状态及私密访问资料，写入已忽略的 `build/private/`。
终端输出包含设备密码，不要将它附到公开 Issue。
Web 固件烧录速率为 460800 baud，串口仍为 115200。如果烧录不稳定，在烧录命令增加
`--upload-speed 115200`。

<!-- section: setup -->
## 配置家庭网络

1. 手机连接 `Lamp-xxxxxx` 热点，密码来自串口 `access` 命令或私密串口检查输出。
2. 打开串口 `setup_ap` 记录打印的配网 IP，以 `admin` 和相同密码登录。
3. 输入家庭 2.4 GHz Wi-Fi 名称和密码，等待页面显示分配到的局域网地址。
4. 手机切回家庭网络，再打开该地址。

家庭网络连接成功三分钟后热点关闭。Wi-Fi 及随机访问密码跨复位保留；通过物理 USB 发送
`setup` 可重新配网。家庭凭据不要写入源码。发送之前先读[使用指南](user-guide.md)：
内置配置未必适用于你的设备。

<!-- section: power -->
## 脱离电脑运行

保持 CC1101 接 ESP32 的 3V3 与 GND，只将 ESP32 的 USB 线从电脑改接 5 V / 1 A USB 电源。
固件不等待串口监视器，会自动连接已保存的 Wi-Fi。更换电源后仍需验证一次真实冷启动；
此前复位测试不能证明该电源稳定。重启后 DHCP 可能分配新 IP；可在路由器保留地址以稳定主屏幕入口。
