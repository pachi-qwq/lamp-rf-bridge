# 射频协议、采集与重放

[English](../en/rf-protocol.md) · [文档导航](index.md)

本页区分实测数据与尚未确认的协议假设。只有一组未具名落地灯及遥控器经过测试。
此前报告的 PCB 标记 `NS-021-RF01` 尚未确认，不作为兼容型号。

<!-- section: scope -->
## 芯片能力与项目支持

TI CC1101 芯片可配置的能力多于本项目实现。当前模块为 433 MHz 版本，
实现路径为**异步 ASK/OOK**，名义频率 **433.92 MHz**；没有其他调制或频段的支持配置。

当前接收设置能采到遥控器，重放也能控制这台灯，但不等于测量了载波中心、晶振误差、
调制深度、辐射功率或距离。频率、调制或 24 位长度相同，都不足以宣称其他设备兼容。
尚未实现滚动码、配对、校验规则或通用协议学习。

<!-- section: receive -->
## 接收配置与采集

`SpiDevice::configureReceive()` 使用 PKTCTRL0=0x30（异步串行，无包 CRC/白化）、
MDMCFG2=0x30（ASK/OOK，关闭 Manchester/同步字检测）、IOCFG0=0x0D（异步数据）、
IOCFG2=0x0E（载波检测）。假定模块晶振 26 MHz，默认带宽 203125 Hz、接收配置速率
4797.935 baud。FOCCFG 的 FOC_LIMIT=0，AGC 与配置常量见
[radio_config.h](../../include/radio_config.h)，写入均回读。

采集使用独立 `phase2` 固件。GPIO26 双边沿中断调用 `esp_timer_get_time()`，
保存 1024 段预触发及 16384 段触发后脉冲。ISR 不申请内存、不打印、不访问 SPI，
主循环轮询 RSSI 并触发记录。背景测试中 GDO2 载波标志也经常为高，因此不是唯一触发依据。

停止后才通过串口导出打包脉冲，第零位为电平，其余位为微秒。
CSV 还原为 `index,level,duration_us,segment`。元数据记录溢出、预触发截断、
同电平歧义回调、零时长、RSSI 及可能不完整的边界，未完成尾段另存电平与时长。
歧义回调不能换算成丢包数，尚未用独立设备校准时间或绝对 RSSI。

<!-- section: workflow -->
## 采集与分析流程

先关闭其他串口监视器，COM4 替换为实际端口：

```powershell
.\.venv\Scripts\python -m platformio run --environment phase2 --target upload --upload-port COM4
.\.venv\Scripts\python scripts/listen_rf.py --port COM4 --label power --seconds 1 --trigger -80
```

看到 ARMED 后，只短按一个指定实体键。监听器默认最多六个窗口，
每窗等待最多 60 秒，触发后记录一秒；未触发窗口同样保存。
`collect_rf.py` 还提供 `--immediate`、`--wait`、`--frequency-khz`、`--profile`、
`--output-dir`。单次记录限制 0.1..10s，等待 0..60s，RSSI 门限 -120..0dBm；
430000..437000kHz 调谐只影响接收。

分析已有真实样本：

```powershell
.\.venv\Scripts\python scripts/analyze_rf.py captures/20261021_225552_366188_power_on_clean_01_wait1.csv --gap-us 5000
.\.venv\Scripts\python tests/test_analysis.py
```

分析器估计互补长短脉冲簇，从候选间隔向前识别，不补齐 24 位、不修补缺失边沿。
`candidate_only=true` 与 `protocol_confirmed=false` 刻意保留分析边界。
对同一实体键重复采集后，再赋予命令标签。

<!-- section: observations -->
## 已观察编码及命令

本项目分析约定：高长低短为 1，高短低长为 0，先出现的位作为高位。
完整候选包含 24 对脉冲，随后为短高及长低分隔。独立前导码、地址字段及校验规则仍未确认。
早期未标定调试样本出现过 FFFF08，未加入支持按键。

| 命令名 | 显示用十六进制 | 一致完整帧数 | 已选实测帧 |
| --- | --- | --- | --- |
| power | FFFF02 | 5 | [power.csv](../../captures/frames/power.csv) |
| brightness_up | FFFF0A | 9 | [brightness_up.csv](../../captures/frames/brightness_up.csv) |
| brightness_down | FFFF0C | 12 | [brightness_down.csv](../../captures/frames/brightness_down.csv) |
| cct_up | FFFF12 | 9 | [cct_up.csv](../../captures/frames/cct_up.csv) |
| cct_down | FFFF10 | 9 | [cct_down.csv](../../captures/frames/cct_down.csv) |
| night_mode | FFFF04 | 6 | [night_mode.csv](../../captures/frames/night_mode.csv) |
| timer_10min | FFFF06 | 6 | [timer_10min.csv](../../captures/frames/timer_10min.csv) |

37 组正常分隔完整帧的统计：短脉冲中位数 408µs（374..490），长脉冲 1190.5µs（1113..1226），
低分隔 13817µs（13771..13848），整帧 52678µs。这些是测量簇，不是理想化发射常量。
[采样索引](../../captures/index.json)记录每个已选 50 段帧的来源范围，
生成器逐项核对原始 CSV，保留每段实测时长。

电源是同一个实体切换键，不是分开的开与关。六类命令已观察到控灯效果。
定时键被确认为标注十分钟，但到期效果未测；没有经过验证的灯具确认或状态查询协议。

<!-- section: replay -->
## 重放与限制

```powershell
.\.venv\Scripts\python -m platformio run --environment phase3 --target upload --upload-port COM4
.\.venv\Scripts\python scripts/replay_power.py --port COM4
.\.venv\Scripts\python scripts/replay_power.py --port COM4 --send --button power
```

第一次脚本调用仅检查健康状态，**--send 会有意发射一次命令**，未经设备授权不要运行。
超时后不自动重试。`--help` 列出相同七个白名单名称；结束后恢复 `phase4` 才能使用 Web。

TX 先使模块 GDO0 三态，再由 ESP32 驱动。PATABLE 的低值为 0x00，高值为 0x34，
FREND0=0x11，对应 TI 433 MHz 典型 -10dBm 表项。RMT 使用确认的 1MHz 时钟、两块内存、
关闭循环。调节键重复实测帧两次，电源/夜灯/定时三次，前加 14ms 低电平，后加 3ms。
最大三帧计划为 152 段、76 项 RMT，全部装入硬件。

计划校验要求恰好 50 段交替电平，数据段 200..1600µs，末低电平 10..20ms，
总时长不超过 200ms，软件等待上限 300ms。正常完成及错误清理会停止 RMT、请求 SIDLE、
读取最终状态；失败后锁定控制。每次尝试结束至少冷却 20ms，上电及状态查询不发射。
“完成”只确认时序、TX 与 IDLE 检查。最初六类灯效以三帧测试；
缩短后的调节版本有正面延迟反馈，但没有重新逐项完成六键矩阵。

参考：[TI 数据手册](https://www.ti.com/lit/ds/symlink/cc1101.pdf)、
[TI DN022](https://www.ti.com/lit/an/swra215e/swra215e.pdf)、
[ESP-IDF 4.4 RMT](https://docs.espressif.com/projects/esp-idf/en/v4.4.7/esp32/api-reference/peripherals/rmt.html)。
