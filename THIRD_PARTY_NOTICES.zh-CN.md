# 第三方许可说明

[English](THIRD_PARTY_NOTICES.md)

根目录 MIT 适用于项目自有内容，不替代以下许可。

## 仓库内第三方内容

| 组件 | 版本及位置 | 许可与归属 |
| --- | --- | --- |
| blueimp-md5 | 2.19.0，web/vendor | MIT，Sebastian Tschan；保留[原许可](web/vendor/blueimp-md5-LICENSE.txt) |
| Contributor Covenant 文本 | 2.1，CODE_OF_CONDUCT 文件 | CC BY 4.0，Contributor Covenant 贡献者；英文调整联系方式，中文为参考译文 |
| 原创图标与启动图 | web/icon.svg 及生成 PNG | 项目自有 MIT 图形，未使用外部图标包或图库图片 |

MD5 脚本原样保存，npm tarball 的 SHA-512 完整性及上游记录于
[blueimp-md5-source.json](web/vendor/blueimp-md5-source.json)，完整 MIT 声明也嵌入发布页面。
第三方注释保留原语言。

行为准则来自上游[明确采用 CC BY 4.0 的固定提交](https://github.com/EthicalSource/contributor_covenant/tree/91532d80ea5f39b6685aff4892bc6854bf2f6d6f)，
没有采用改版网站当前的源码许可。保留[CC BY 4.0 原文](LICENSES/CC-BY-4.0.txt)及归属，
项目只适配联系方式和语言切换链接。社区政策文本不改为 MIT。

## 固件与构建依赖

| 依赖 | 固定版本 | 相关许可 |
| --- | --- | --- |
| Arduino-ESP32 | 2.0.17 / PlatformIO 包 3.20017.241212 | 核心及库采用 LGPL-2.1-or-later，部分内置组件另有许可 |
| Arduino 包内 ESP-IDF SDK | 随该包提供的 4.4 系列 | ESP-IDF 源码 Apache-2.0；组件和预编译无线库可能另有条款 |
| Mbed TLS SHA-256/HMAC 头文件 | 随 SDK 提供 | Apache-2.0 OR GPL-2.0-or-later，本项目选择 Apache 选项 |
| PlatformIO Core / espressif32 平台 | 6.1.19 / 6.12.0 | Apache-2.0 |
| Python requests / pyserial | requirements-lock.txt | Apache-2.0 / BSD-3-Clause |
| 工具链、esptool 及其他传递工具 | platformio.ini / 锁定文件 | 各工具包自身许可，不属于项目 MIT |

仓库附[Arduino 原许可](LICENSES/LGPL-2.1.txt)及 [Apache 2.0 原文](LICENSES/Apache-2.0.txt)。
依据为 [Arduino-ESP32 2.0.17](https://github.com/espressif/arduino-esp32/tree/2.0.17)、
[ESP-IDF](https://github.com/espressif/esp-idf/tree/v4.4.7)及本地包声明。
本页说明使用组件，不表示所有 SDK 二进制或传递构建工具均为 MIT/Apache。

## 分发边界

仓库包含应用源码及原创射频样本，没有复制 TI 数据手册、内置 ESP32 SDK 或预编译固件；
外部资料仅提供链接。分发固件二进制前须保留适用声明，按 LGPL 提供相应库源码及重链接所需材料，
并核对 SDK 二进制义务。不要将整份固件称为“仅 MIT”。
当前 CI 只构建，不发布二进制产物，完整的二进制合规分发包尚未整理。

开发 npm 已检查 80 个安装包的许可元数据：76 个 MIT、2 个 ISC、1 个 BSD-2-Clause、
1 个 Python-2.0。它们是检查工具，不随网页运行时发布；Python 检查工具的传递版本
固定于 requirements-dev.txt。
