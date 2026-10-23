# 历史验证证据

[English](README.md)

历史硬件 JSON 描述此前真实设备检查及使用者观察，不是本轮 CI 结果，
也不认证整理后的新二进制。[open-source-audit.json](open-source-audit.json)
保留初次本地整理快照，[review-remediation.json](review-remediation.json)
记录后续仓库、CI 及隐私复查。失败及修复前记录仍保留失败，后续通过不会改写旧结果。
[测试指南](../zh-CN/testing.md)标明相关通过记录及局限。

公开 JSON 脱敏个人局域网地址、设备后缀及 MAC，保留检查名称、布尔值、次数、
计时及原始使用者引语。公开整理前已将原文件及逐字节串口日志保存到本地忽略的 build/private。
原始串口不进入仓库，以免未来记录暴露访问凭据。
基线没有提交，因此未重写或删除已提交历史。

后续复查未在跟踪文件或可达初始提交中发现其他凭据/设备标识。
简短技术反馈及有意公开的灯具 RF 样本继续保留。仅修正 historical-phase3/phase4-validation.json
搬迁后的 43 个 evidence 数组引用，变更范围见各文件的 record_maintenance；结果及引语未改动。
