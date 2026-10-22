# HTTP API 参考

[English](../en/api.md) · [文档导航](index.md)

Web 固件监听 80 端口，共享的 [OpenAPI 描述](../api.openapi.json)记录输入输出结构，
本页解释操作语义。没有接口接受任意射频数据；API 表示网关，不是已确认的灯具状态模型。

<!-- section: auth -->
## 认证及共同检查

受保护路由接受有效 `lamp_device` Cookie（RAM 或持久信任），或用户名 `admin`
及 USB 获取密码的 HTTP Digest。静态 `/`、`/login`、`/style.css`、`/app.js`、
清单及图片不含控制凭据；前四项仍检查规范设备 Host，元数据及图片公开。

全部 API 要求 Host 为规范局域网 IP、当前热点 IP 或自身 .local 名称，可带 :80。
修改操作要求 `application/x-www-form-urlencoded`、当前 `X-Lamp-CSRF`，
Origin 存在时还须与当前地址完全同源，不放行 CORS。
表单登录使用独立挑战 CSRF，而非已认证状态的启动周期 CSRF。

浏览器携带 `X-Lamp-Web: 1` 时，未认证返回 JSON 401，不弹系统认证窗口；
诊断 Digest 客户端会收到 WWW-Authenticate。任何公开接口都不返回密码。

<!-- section: routes -->
## 接口列表

| 方法及路径 | 权限与输入 | 结果 |
| --- | --- | --- |
| GET /api/session | 公开探测，可带认证 | 200 authenticated=false，或完整认证状态 |
| GET /api/login/challenge | 公开，Host 合法 | realm、nonce、opaque、csrf |
| POST /api/session/login | 一次性挑战 Digest 及 CSRF，remember=0/1 | Cookie、ok、trusted_device |
| GET /api/status | 认证 | 网关、模块、网络及 CSRF |
| POST /api/command | 认证及启动 CSRF，button、request_id | 发送结果、耗时及 false 的 lamp_state_confirmed |
| POST /api/wifi | 认证及启动 CSRF，ssid、password，仅热点接口 | 保存并开始连接 |
| POST /api/session/trust | 认证及启动 CSRF，空正文 | 持久信任 Cookie |
| POST /api/session/forget | 认证及启动 CSRF，空正文 | 撤销信任并签发 RAM Cookie |

已认证的未知路由或方法返回 404，不保证 405；未认证的未知路由先要求认证。

<!-- section: login -->
## 表单登录流程

1. GET /api/login/challenge 获取签名 nonce、opaque、realm `Lamp Bridge` 和独立 CSRF。
2. 实际 [auth.js](../../web/auth.js) 为 **POST /api/session/login** 计算标准 Digest，
   qop=auth、nc=00000001，并生成随机 cnonce。
3. Authorization 携带证明，X-Lamp-CSRF 携带挑战 CSRF，正文仅为 `remember=0` 或 `1`。
   此表单不发送明文密码或 HA1。
4. 通过 GET /api/session 确认 Cookie 生效，再进入控制页。

RAM 循环保留四个待用挑战，期限两分钟，验证密码前即消耗，包括错误密码尝试。
成功后签发 HttpOnly、SameSite=Strict、Path=/ Cookie；持久信任的浏览器 Max-Age 为一年，
RAM Cookie 不设置 Max-Age。设备保存哈希，期限及撤销语义见[使用指南](user-guide.md)。

签名 Digest nonce 有效一小时，绑定本次启动及 32 位计时周期；
回绕边界可能使较新的 nonce 提前失效，需重新取挑战。
RAM 会话采用 64 位期限，避免 millis() 回绕后过期凭证复活。
这些改动已有本地边界测试，仍需重新执行真实设备认证回归。

<!-- section: status -->
## 状态字段

认证状态没有顶层 `ok`，包括：

| 字段 | 含义 |
| --- | --- |
| authenticated / phase / csrf | true / 4 / 修改操作的启动周期 CSRF |
| auth.trusted_device | 当前请求携带有效持久信任 Cookie |
| http.idle_preconnect_closed | 本次启动空闲连接保护计数 |
| rf.ready / marcstate / bus_error | 网关射频就绪、原始状态及 BusError 数值 |
| network.connected / configured / ap_active | 网络生命周期标志 |
| network.can_configure | 请求通过当前热点接口进入 |
| network.ip / ssid | 已连接局域网值；断线时为空 |
| attempts / last_button | 本次启动实际发射尝试数及最近尝试命令 |
| cooldown_ms / uptime_s | 剩余冷却及无符号 millis()/1000 启动时间 |

模块状态与请求成功都不是灯具状态回传。公开状态响应前先去除私人 SSID/IP。
`BusError` 按声明顺序为 None=0、ReadyTimeout=1、ResetTimeout=2、NotInitialized=3、
RegisterMismatch=4、StateTimeout=5、InvalidSettings=6、ChipNotReady=7。

<!-- section: command -->
## 命令输入输出

正文只能含 `button`、`request_id` 两项。名称为
`power`、`brightness_up`、`brightness_down`、`cct_up`、`cct_down`、
`night_mode`、`timer_10min`，请求编号为 16..48 位小写十六进制字符。

成功响应包含 `ok`、中文 `message`、`button`、`attempt`、
`cooldown_ms`、`radio_elapsed_us`、`frames`、`lamp_state_confirmed:false`。
最近八个编号缓存原状态码及正文，包括失败尝试；同一编号换按键返回 409。
重启或缓存淘汰后不再去重，电源请求超时后不能盲目重试。

只读 PowerShell 示例，curl 会提示输入访问密码：

```powershell
$Base = Read-Host 'Device URL, without trailing slash'
curl.exe --digest --user admin "$Base/api/status"
```

以下命令**会有意发送一次调亮**，仅在已授权的兼容灯具上运行。
本页没有伪造输出作为硬件结果。

```powershell
$Status = curl.exe --silent --show-error --digest --user admin "$Base/api/status" | ConvertFrom-Json
$Id = [guid]::NewGuid().ToString('N')
curl.exe --digest --user admin "$Base/api/command" --header "X-Lamp-CSRF: $($Status.csrf)" --data-urlencode 'button=brightness_up' --data-urlencode "request_id=$Id"
```

<!-- section: errors -->
## 校验及错误

| HTTP 状态 | 情况 |
| --- | --- |
| 400 | 未知、多余、格式错误的按键/编号，非法 Wi-Fi 长度，信任接口多余参数 |
| 401 | 受保护 API 没有有效认证 |
| 403 | Host/Origin/CSRF 不符，登录证明非法或过期，从局域网调用配网 |
| 404 | 已认证但路由或方法未注册 |
| 409 | 缓存编号被其他按键使用 |
| 415 | 受保护修改操作格式不支持 |
| 429 | 冷却期间尝试命令 |
| 500 | NVS 或会话保存失败 |
| 503 | 射频未就绪、失败，或认证签名失败 |

错误通常为 `{"ok":false,"message":"..."}`；命令结果还含上述命令字段。
客户端须检查状态码和 ok，不能自动重放。
SSID 限制 1..32 字节，密码为空或 8..63 字节，拒绝控制字符。
配网一次写入完整 NVS 记录并重连，仅热点接口可调用。
