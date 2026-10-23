# lamp-rf-bridge

[English](README.md) · [中文文档](docs/zh-CN/index.md)

使用 ESP32 与 CC1101 学习射频采集、分析脉冲编码，并通过局域网浏览器控制兼容设备。
内置配置来自**一台实际验证的落地灯**。其他 433 MHz 设备需要独立采集、分析及验证；
频率相同并不代表兼容。

## 已实现功能

- 带就绪超时、连续样本校验的 SPI 芯片身份诊断。
- 异步 OOK 接收、GPIO 边沿时间戳、CSV 导出及离线 PWM 候选帧分析。
- 使用硬件 RMT 重放七类实测波形。电源、亮度增减、色温增减与夜灯已观察到实际效果；
  10 分钟定时已采集，但未验证灯具效果。
- 带密码认证的局域网控制、2.4 GHz Wi-Fi 配网与重连、设备信任及 iPhone 主屏幕入口。
  当前设备界面和串口提示为中文。
- 分离的诊断、采集、重放及 Web 固件环境，构建依赖固定版本。

## 硬件要求

已验证组合为微雪 **ESP32-DEV-KIT-WROOM-32E-N4-M**（4 MB Flash）及
**E07-M1101D 类规格的 CC1101 433 MHz SPI 模块**，使用弹簧天线；模块确切厂商尚未确认。
CC1101 必须接 **3.3 V**，不可直接接 5 V。接线前先阅读[引脚表及电气说明](docs/zh-CN/hardware.md)。

## 快速开始

使用 Python 3.11，在仓库根目录的 Windows PowerShell 中执行：

```powershell
python -m venv .venv
.\.venv\Scripts\python -m pip install -r requirements-lock.txt
.\.venv\Scripts\python -m platformio run --environment phase4
```

以上命令无需硬件即可构建。USB 驱动、首次 SPI 检测、烧录、Linux/macOS 命令和首次配网见
[快速开始](docs/zh-CN/getting-started.md)。不要假定内置命令适用于你的灯。

## 文档导航

| 主题 | 文档 |
| --- | --- |
| 首次构建、SPI 检测与部署 | [快速开始](docs/zh-CN/getting-started.md) |
| 接线与供电 | [硬件说明](docs/zh-CN/hardware.md) |
| 日常控制、登录与主屏幕 | [使用指南](docs/zh-CN/user-guide.md) |
| 集中配置与生成资源 | [配置说明](docs/zh-CN/configuration.md) |
| 采集、时序、重放与兼容性 | [射频协议](docs/zh-CN/rf-protocol.md) |
| 模块与数据流 | [架构设计](docs/zh-CN/architecture.md) |
| HTTP 接口 | [API 参考](docs/zh-CN/api.md) |
| 构建、测试及 CI | [开发指南](docs/zh-CN/development.md) · [测试与验证](docs/zh-CN/testing.md) |
| 异常与排查 | [故障排查](docs/zh-CN/troubleshooting.md) |
| 发布准备 | [发布检查表](docs/zh-CN/release-checklist.md) |

## 支持范围及限制

Web 固件只开放固定命令白名单，不提供通用射频学习界面。采集使用独立固件，期间不启用 Wi-Fi。
电源是**切换键**；没有经过验证的灯具状态查询、独立绝对开关指令、亮度百分比或 Kelvin 回读。

主屏幕入口是**局域网在线 Web App**。当前 HTTP 部署没有 Service Worker、离线控制、TLS、
云服务或 OTA。手机必须能访问网关网络。项目不承诺射频距离、实测辐射功率或广泛设备兼容性。
详见[测试边界](docs/zh-CN/testing.md)及[安全政策](SECURITY.zh-CN.md)。

仓库未提供硬件照片、控制页截图或演示视频。`web/` 中的 SVG/PNG 是应用图形资源，不是硬件工作证据。

## 贡献方式

请阅读[贡献指南](CONTRIBUTING.zh-CN.md)及[行为准则](CODE_OF_CONDUCT.zh-CN.md)。
有意义的一方代码注释须中英双语，修改文档时同步两种语言，并将软件检查与实体观察分开。
漏洞请按[安全政策](SECURITY.zh-CN.md)私密报告。

## License

项目自有源码、文档及图形资源采用 [MIT License](LICENSE)，
Copyright (c) 2026 pachi-qwq。第三方组件保留各自许可，包括 Arduino-ESP32 的 LGPL 条款，
详见[第三方许可说明](THIRD_PARTY_NOTICES.zh-CN.md)。许可证以英文 LICENSE 原文为准。
