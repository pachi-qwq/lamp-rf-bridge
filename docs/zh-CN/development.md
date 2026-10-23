# 开发指南

[English](../en/development.md) · [文档导航](index.md)

先完成[快速开始](getting-started.md)的工具环境。固件使用 C++17，
PlatformIO/Arduino 固定包版本保持不变。Windows 主机测试需要 Visual Studio C++ Build Tools；
POSIX 使用 g++ 或 CXX 指定的可执行文件。质量检查另需 Node.js 22。

<!-- section: setup -->
## 安装检查工具

```powershell
.\.venv\Scripts\python -m pip install -r requirements-dev.txt
npm ci --ignore-scripts
```

这些是开发依赖，不是网页运行时。页面不用 npm 加载器或打包器，
唯一固定的 MD5 第三方实现保留自身 MIT 声明。
只有重新导出图形时需要 Pillow，普通构建及检查不重新生成 PNG。

<!-- section: build -->
## 生成与构建

```powershell
.\.venv\Scripts\python scripts/generate_replay_frames.py
.\.venv\Scripts\python scripts/embed_web.py
.\.venv\Scripts\python -m platformio run -e esp32dev -e phase2 -e phase3 -e phase4
```

保留 Wall/Wextra，项目源码警告按错误处理，不修改已安装框架。
`scripts/pio.ps1` 可从其他目录调用，并选用本地环境及缓存，不依赖 rtk。
POSIX 将 Windows 解释器路径改为 `.venv/bin/python`。

派生文件为 `include/remote_frames.h`、`include/web_assets.h`，
生成器校验 CSV 来源索引与数值，并计算精确 CSP 哈希；不要手动格式化生成头文件。

<!-- section: checks -->
## 执行本地软件检查

```powershell
.\.venv\Scripts\python scripts/test_host.py
.\.venv\Scripts\python tests/test_analysis.py
node --check web/app.js
node --check web/auth.js
node tests/test_web_auth.js
.\.venv\Scripts\python scripts/check_repo.py
npm run lint:markdown
git diff --check
```

C++ 测试检查身份、RF 数值、重放边界与 Web/认证策略，不执行 I/O；
Python 使用留存真实样本，JavaScript 向量使用实际客户端代码。
以上命令不访问串口、网关或射频。`check_repo.py` 检查内部链接、双语文件/章节/命令覆盖、
源码格式与语法、引脚、JSON/YAML、API 路由对应及生成资源，
不是浏览器渲染器、翻译工具或完整安全审计。

修改一方 C++ 后，使用虚拟环境 Python 同目录中的固定 clang-format，
只对编辑过的文件加 `-i`。Python 四空格，C++/网页两空格。
保留既有紧凑网页代码，不额外引入前端格式迁移。

安装 Go 时可执行工作流语法检查：

```sh
go run github.com/rhysd/actionlint/cmd/actionlint@v1.7.7 .github/workflows/ci.yml
```

<!-- section: ci -->
## CI 覆盖

[ci.yml](../../.github/workflows/ci.yml)包含两个 Ubuntu 24.04 任务：
四环境固件构建及离线质量/测试。Actions 固定到提交，
权限为 contents:read，不保留 checkout 凭据，任务有超时，没有发布或部署步骤。
Python 3.11、Node 22 使用兼容维护补丁，固件包及 npm 依赖固定。
不模拟硬件、凭据或射频响应。

原版[托管运行](https://github.com/pachi-qwq/lamp-rf-bridge/actions/runs/39805827721)
已对 8ca1b79 通过两个 Job，包含 Ubuntu 主机测试。当前工作流固定 checkout v7.0.1、
setup-python v7.1.0、setup-node v7.1.0，它们均声明 node24，要求 runner 至少 2.327.1。
项目检查工具仍选择 Node 22；Actions 的 ESM/运行时升级不改变固件依赖，
并显式关闭包管理器自动缓存。更新后的工作流目前只有本地配置检查，
需经授权推送后触发新运行。CI 不分发固件二进制产物。

<!-- section: changes -->
## 保持改动可审查

遵守[贡献指南](../../CONTRIBUTING.zh-CN.md)。新证据及独立改动前，保留 RF 参数。
一方有意义注释须双语，公开 API 合约准确；修改两种语言对应文件及章节标记，
结构检查通过后还要人工核对含义。保留机器 JSON/串口字段及使用者原始引语。

本次认证期限修复与格式、文档分开记录：RAM 改用 64 位期限，Digest 绑定计时周期，
阻止旧凭据在回绕后复活；增加十项纯边界检查，不改变 RF 行为。
固件发布前仍需对当前版本重新验证真实登录及重启。
