# 测试与验证

[English](../en/testing.md) · [文档导航](index.md)

软件检查、历史硬件证据及未执行测试分开记录。本次开源整理未连接硬件、未烧录、未发射射频。

<!-- section: local -->
## 本地软件结果

在记录的 Windows 环境中：

| 检查 | 实际结果 |
| --- | --- |
| PlatformIO esp32dev / phase2 / phase3 / phase4 | 四个环境均构建成功 |
| C++ 纯逻辑 | 273 项通过：身份 29、RF 数值 12、重放 189、Web/认证 43 |
| Python 真实采样回归 | 5 个测试通过 |
| JavaScript MD5/Digest 向量 | 实际第三方及客户端代码 8 项通过 |
| 工作流语法 | actionlint 1.7.7 通过 |
| 跨平台生成器 | Ubuntu/WSL Python 3.12.3 的两个 --check 均通过 |
| Markdown、仓库一致性及 Diff | 见[本轮审计记录](../evidence/open-source-audit.json) |

当前 Web 构建应用 Flash 为 895689 / 1310720 字节（68.3%），静态 RAM
49680 / 327680（15.2%），动态堆另计。二进制 SHA-256 为
`606c73da7fde56dd3705b9baf19658b5eda139ff0a5daf23086a04c17b3f1119`。
该二进制**未烧录**，包含格式、双语注释、CSV 字节保留、统一 gzip 元数据及第三方页面许可。
独立认证期限修复新增十项边界测试，不表示实际连续运行了 49 天。

复现软件检查见[开发命令](development.md)。
原版 [GitHub CI](https://github.com/pachi-qwq/lamp-rf-bridge/actions/runs/39805827721)
已对 8ca1b79 通过 firmware、quality，包含 Ubuntu 主机编译路径。
后续[评审记录](../evidence/review-remediation.json)区分本地重跑结果与尚未推送的
Node.js 24 Actions 升级；更新后的工作流仍需单独远程运行。

<!-- section: history -->
## 历史真实设备证据

整理前已验证接线开发板的身份、采集及灯效：

| 范围 | 观察与证据 |
| --- | --- |
| SPI | 换杜邦线后 PARTNUM=0x00、VERSION=0x14、IDLE=0x01 稳定 |
| 采集 | 七类标定按键，重复完整候选及留存原始 CSV |
| 重放 | 电源双向切换、亮度增减、色温增减及夜灯有可见效果 |
| 定时 | 已采集并保留实现，按要求跳过灯效及到期测试 |
| Android | 热点、家庭 2.4 GHz 配网、登录及控灯已确认 |
| iPhone | 登录控灯、主屏幕图标/布局、信任、密码填充及按键反馈已确认 |
| 启动 | 空闲预连接修复后，使用者报告一秒以内 |

结构化反馈：[重放](../evidence/historical-phase3-validation.json)、
[Web](../evidence/historical-phase4-validation.json)。
最初六类灯效以三帧确认；调节缩短至两帧后，使用者认可响应速度，
但没有重新记录完整按键矩阵。

此前最新网络/认证组合为 **87 项**，期间没有发射：
[表单登录 27](../evidence/20261022_014238_form-login-checks.json)、
[真实重启 6](../evidence/20261022_014356_form-login-reboot-checks.json)、
[HTTP 35](../evidence/20261022_014242_http-checks.json)、
[Digest 8](../evidence/20261022_014348_digest-checks.json)、
[预连接 11](../evidence/20261022_014250_preconnect-checks.json)。
它们属于此前固件，不是本轮新二进制结果。证据目录保留修复前及失败文件，不改写结果；
其中静态页面公开后，早期跨 URI 测试也需要改为受保护路径。

<!-- section: hardware -->
## 有意执行实体复验

`check_http.py`、`check_digest.py`、`check_preconnect.py`、
`check_trust.py`、`check_form_login.py` 需要真实网关 URL，
以及 `check_phase4_serial.py` 生成的私密访问文件，不能放到离线 CI。
先读各脚本 `--help`。网络检查不发送合法 RF 命令；
信任/登录测试会临时修改真实凭证，需完成对应清理及重启续测。

`check_trust.py --resume`、`check_form_login.py --resume`
需要保留本地测试数据，并在两次运行之间实际重启 ESP32。表单检查另需 Node.js。
访问文件及凭证只放已忽略的 build/private，撤销临时持久信任。真实设备测试不是软件模拟。

发布整理后的固件前，应重新验证启动、表单登录、信任/重启及基本授权控灯。
逐条观察，不能从 HTTP 200 或 RMT 完成推导灯效；二进制许可分发准备是另一前提。

<!-- section: cleanup-regression -->
<a id="cleanup-regression"></a>

## 整理后固件回归（Pending）

本轮只发现 COM1（ACPI 串口），未识别到 ESP32 USB 转串口。
没有尝试烧录、网络测试或射频发送；以下项目在取得真实证据前均保持 Pending。

| 顺序 | 最小检查 | 预期观察 |
| --- | --- | --- |
| 1 | 连接 USB，构建/烧录 phase4，不擦除 NVS | 识别正确适配器，私密保存二进制哈希和启动日志 |
| 2 | 启动并连接已保存 Wi-Fi | rf_boot 身份合法、bus_error=0、ready=true、MARCSTATE=1；不自动发射 |
| 3 | 使用现有脚本检查 HTTP、Digest 及一次性表单登录 | 未授权、伪造、跨源、错误 Host/CSRF 被拒；合法登录成功 |
| 4 | 建立临时 RAM 会话及信任凭证，再重启一次 | RAM 凭证失效、信任凭证有效、旧挑战失效；清理临时信任 |
| 5 | 发一次电源命令，再重复其 request_id，最后一次调亮 | 观察灯效；重复请求不增加 attempts、不再次发射 |
| 6 | 在约定时间中断并恢复网关 WLAN | 记录断线/重连，认证状态接口恢复可用，不自动重发 RF |

每次只向操作者提出一个动作。运行前检查登录/信任脚本：它们会写入临时持久凭证，
可能替换四槽信任环中的已有条目。保留原有访问，只撤销测试临时凭证，不为此测试清空 NVS。

64 位期限和 Digest 周期绑定已有边界测试，但没有实际等待八小时或运行完整 49.7 天。
Wi-Fi 恢复是待观察项目，不是已通过结果。[发布检查表](release-checklist.md)保留该验收条件。

<!-- section: limits -->
## 未验证范围

尚无完整板型/设备矩阵、实测距离、频谱/功率校准、十分钟到期、
更换电源冷启动验收、故障注入、长时间压力、八小时会话真实等待或完整浏览器自动化。
iPhone 反馈来自单台手机及显示模式。硬件记录也不证明法规合规、射频保密、
滚动码支持或真实灯具状态回读。
