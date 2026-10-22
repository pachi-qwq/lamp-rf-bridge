# 变更日志

[English](CHANGELOG.md)

面向使用者的变更参照 [Keep a Changelog](https://keepachangelog.com/en/1.1.0/)。
未来发布采用 [Semantic Versioning](https://semver.org/spec/v2.0.0.html)。
审计时工作区没有标签发布或提交历史，因此不补造发布日期及历史版本变化。
建立远程仓库及标签后再增加版本比较链接。

## Unreleased

### 新增

- 中英文用户及开发文档、双语一方注释与公开 API 说明。
- 项目 MIT、依赖许可、贡献及安全政策、Contributor Covenant 2.1。
- GitHub 纯软件构建及检查、Issue/PR 模板、仓库一致性检查。
- 离线客户端 Digest 向量与认证期限边界回归。

### 调整

- 主机逻辑测试增加 POSIX C++17 编译器支持，PowerShell 构建包装不再依赖 rtk。
- Git 检出保留采样 CSV 字节，gzip 元数据统一跨平台。
- 页面内保留第三方 MD5 许可；原始串口及旧工作笔记仅本地保留，公开证据移除个人设备及网络标识。

### 安全

- RAM 会话过期改用 64 位单调时钟；Digest 签名绑定 32 位时间戳周期，
  防止 millis() 回绕后旧挑战再次生效。

既有功能包括 SPI 诊断、OOK 采集导出及分析、七类固定实测波形重放、局域网配网及控制。
历史已验证六类灯效及手机控制，定时未实测。本次整理没有改变射频常量或波形。
