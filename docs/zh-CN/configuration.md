# 配置参考

[English](../en/configuration.md) · [文档导航](index.md)

硬件与协议采用编译期常量，网络凭据在运行时配网。项目没有任意射频数据、频率或功率的公开 API。

<!-- section: files -->
## 配置文件

| 文件 | 职责 |
| --- | --- |
| [config.h](../../include/config.h) | GPIO、SPI/串口时钟、复位与就绪时间、身份连续次数 |
| [radio_config.h](../../include/radio_config.h) | 假定晶振、名义接收频率及范围、解调配置 |
| [replay_config.h](../../include/replay_config.h) | 发射功率表项、重复数、RMT 容量及冷却 |
| [web_config.h](../../include/web_config.h) | HTTP 端口、用户名、重连/热点/预连接时间 |
| [platformio.ini](../../platformio.ini) | 板型、固定工具链、固件源码选择及警告 |
| [captures/index.json](../../captures/index.json) | 采集命令标定及所选帧来源 |

帧查找头文件及网页头文件由工具生成，应修改输入，不手改字节数组。

<!-- section: defaults -->
## 当前默认值

| 参数 | 数值 |
| --- | --- |
| 串口 / SPI | 115200 baud / 1 MHz，模式 0 |
| CSN / SCK / MOSI / MISO | GPIO5 / GPIO18 / GPIO23 / GPIO19 |
| GDO0 / GDO2 | GPIO26 / GPIO27 |
| 身份连续次数 / 就绪超时 | 10 次 / 10000µs |
| 名义频率 / 假定 RF 晶振 | 433920000 Hz / 26000000 Hz |
| 默认接收带宽 / 配置速率 | 203125 Hz / 4797.935 baud |
| 仅接收调谐范围 | 430..437 MHz，不是发射 API |
| 发射 PATABLE / FREND0 | [0x00, 0x34, 0, 0, 0, 0, 0, 0] / 0x11 |
| 调节键 / 模式键重复数 | 2 / 3 帧，上限 3 帧 |
| 额外起始 / 结束低电平 | 14000 / 3000µs |
| RMT | 通道 0，1µs tick，2 块（128 项），不循环 |
| 计划时长 / 软件等待上限 | 200000µs / 300ms |
| 尝试结束后最短冷却 | 20ms，失败也执行 |
| HTTP / Wi-Fi 重连 / 回退热点 | 端口 80 / 15s / 60s |
| 热点保留 / 频道 / 客户端数 | 180s / 6 / 2 |
| 竞争时空闲预连接等待门限 | 200ms |
| 请求结果缓存 | 8 个编号，仅 RAM |

接收实验还提供 wide（406250 Hz）、narrow（81250 Hz）、fast（203125 Hz / 9595.871 baud）。
这些速率是 CC1101 解调配置，不是已经确认的遥控器位速率。
新采样提供依据前，保留已验证的默认值。

<!-- section: storage -->
## 运行时存储

NVS 的 `lamp` 命名空间保存 `network`（SSID/密码）、`access`（随机访问及热点密码）
和 `trusted`（最多四个凭证哈希）。项目未启用 NVS/Flash 加密。
固件关闭框架自动 Wi-Fi 持久化，自行完整写入凭据记录。

修改用户名常量不会轮换已签发的信任凭证。当前没有修改密码、恢复出厂或撤销全部凭证的 API。
物理擦除 Flash 属于破坏性操作，会丢失 Wi-Fi、密码及设备信任，并需重新安装，
不要将擦除当作日常部署步骤。

<!-- section: generated -->
## 重新生成资源

```powershell
.\.venv\Scripts\python scripts/generate_replay_frames.py
.\.venv\Scripts\python scripts/embed_web.py
.\.venv\Scripts\python scripts/generate_replay_frames.py --check
.\.venv\Scripts\python scripts/embed_web.py --check
```

CSV 来源哈希依赖原始字节，因此 Git 属性禁止转换采样换行。
网页打包固定 gzip 时间及 OS 元数据，并生成精确 CSP 哈希；HTML 的脚本注释包含第三方 MD5 完整许可。
PNG 已纳入仓库；只有通过 `scripts/export_icon.py` 重新导出时需要 Pillow，它不是固件构建依赖。
