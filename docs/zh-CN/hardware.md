# 硬件与接线

[English](../en/hardware.md) · [文档导航](index.md)

本页记录实际搭建的组合，不代表所有 ESP32 或 CC1101 板型均兼容。

<!-- section: equipment -->
## 已验证硬件

| 项目 | 实际配置 |
| --- | --- |
| ESP32 开发板 | 微雪 ESP32-DEV-KIT-WROOM-32E-N4-M |
| 模组与 Flash | ESP32-WROOM-32E / 4 MB |
| USB | Type-C，CH343P USB 转串口 |
| 射频模块 | E07-M1101D 类规格 CC1101 SPI 模块，433 MHz 弹簧天线；确切厂商未确认 |
| PlatformIO 板型 | esp32dev，DIO，Flash 40 MHz |
| 开发供电 | 电脑 USB |
| 计划独立供电 | 5 V / 1 A USB 电源给 ESP32；更换电源后的冷启动未专项验证 |

ESP32 晶振与假定为 **26 MHz 的 CC1101 模块晶振**是两个时钟。
CC1101 实测 PARTNUM=0x00、VERSION=0x14，不能据此确定模块厂商。

<!-- section: pins -->
## 引脚表

已与 [include/config.h](../../include/config.h) 逐项核对：

| CC1101 模块脚位 | 信号 | 杜邦线 | ESP32 |
| --- | --- | --- | --- |
| 1 | GND | 白色 | G / GND |
| 2 | VCC | 黄色 | 3V3 |
| 3 | GDO0 | 灰色 | GPIO26 |
| 4 | CSN | 蓝色 | GPIO5 |
| 5 | SCK | 绿色 | GPIO18 |
| 6 | MOSI / SI | 紫色 | GPIO23 |
| 7 | MISO / SO / CHIP_RDYn | 黑色 | GPIO19 |
| 8 | GDO2 | 橙色 | GPIO27 |

脚位编号和线色仅对应本次模块。其他 CC1101 模块的排针布局可能不同，
必须先根据丝印确认信号及方向，再接线。

<!-- section: electrical -->
## 电气要求

- 修改接线前断开 USB，首次连接先确认排针方向及共地。
- CC1101 使用 **3.3 V**，VCC 和信号输入不可直接接 5 V。
- GPIO5 是 ESP32 启动配置脚，避免外部电路将其强拉到不兼容的启动电平。
- 原搭建中杜邦线接触问题曾导致 SPI 失败；重新插紧 MISO 仍失败，更换杜邦线后身份读数稳定。
  模块实际供电电压未测量。
- 有意发射时接好天线；不要仅依据模块标称说明改变功率或频率。
- 诊断时两个 GDO 均为 ESP32 输入。RX 时 GDO0 由 CC1101 输出数据。
  TX 先使模块 GDO0 三态，再由 ESP32/RMT 驱动；GDO2 保持输入。

驱动采用 VSPI、模式 0、MSB first、1 MHz，并以 10ms 超时等待 SO/CHIP_RDYn。
状态寄存器逐个置 READ|BURST：PARTNUM 0xF0、VERSION 0xF1、MARCSTATE 0xF5。
此处 BURST 用于区分状态读取与命令，并不是连续读取多个状态寄存器。

<!-- section: radio -->
## 无线电使用要求

433 MHz 合法频段、功率、占空比与使用限制因地区而异，发射前核对所在地规定。
项目本身不构成合规认证。当前 PA 值对应 TI 表中 433 MHz 典型 -10 dBm，
不等于实测天线输出或 EIRP。只测试自己拥有或已获授权的设备。

参考：[TI CC1101 数据手册](https://www.ti.com/lit/ds/symlink/cc1101.pdf)、
[PlatformIO esp32dev](https://docs.platformio.org/en/stable/boards/espressif32/esp32dev.html)、
[微雪开发板文档](https://docs.waveshare.net/ESP32-DEV-KIT-XX/)。
