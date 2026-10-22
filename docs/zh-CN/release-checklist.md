# 发布前检查表

[English](../en/release-checklist.md) · [文档导航](index.md)

本表用于完成 [PR #1](https://github.com/pachi-qwq/lamp-rf-bridge/pull/1) 并准备首次发布。
main 已从空基线建立并设为默认分支，PR 仍未合并。
整理后固件的实机回归按所有者决定暂缓，仍保留为固件发布前待办。

<!-- section: status -->
## 当前状态

| 状态 | 项目 | 证据或剩余操作 |
| --- | --- | --- |
| 已完成 | 仓库及评审分支 | [pachi-qwq/lamp-rf-bridge](https://github.com/pachi-qwq/lamp-rf-bridge)，review/open-source-preparation |
| 已完成 | main 及默认分支 | main 指向空基线 35b182edd61ebca3aeae091352fe7d734ebcc9a3，且已设为默认 |
| 已完成 | 首个正式 PR | [PR #1](https://github.com/pachi-qwq/lamp-rf-bridge/pull/1)：main ← review/open-source-preparation，开放且未合并 |
| 已验证 | PR firmware、quality Job | [运行 39817485832](https://github.com/pachi-qwq/lamp-rf-bridge/actions/runs/39817485832)，提交 66dcbfd；两者成功 |
| 已验证 | Actions 升级至 Node.js 24 | 升级后推送及 PR 运行均通过；见 [CI 结果](testing.md#hosted-ci) |
| 已完成 | 简介及 Topics | 两者已填写，Homepage 仍为空 |
| 待人工操作 | 私密报告及行为投诉渠道 | Private Vulnerability Reporting 未开启，仍需指定私密行为投诉渠道 |
| 源码 PR 暂缓 | 整理后固件回归 | 所有者接受暂缓，固件发布前仍待验证；见[测试顺序](testing.md#cleanup-regression) |
| 待二进制发布准备 | 第三方分发材料 | LGPL 对应源码、重链接材料及 SDK 条款 |
| 0.1.0 暂不实施 | OTA、云端、离线控制及广泛兼容 | 超出已验证实现 |

所链接的 PR 运行验证 **66dcbfd**，包含 Actions 升级。
后续文档提交也须通过其自身的[最新 PR 检查](https://github.com/pachi-qwq/lamp-rf-bridge/pull/1/checks)。
历史审计 JSON 保留记录当时的状态，不为反映新 CI 而追溯改写。

<!-- section: version -->
## 首次版本

完成发布检查后建议采用 **0.1.0**。单设备配置与诊断已在文档范围内工作，
兼容、安全及硬件矩阵仍属实验阶段。实际标签建立前保持 Unreleased。
后续按 SemVer 管理已文档化 API 和配置，1.0 前的不兼容变更也须明确说明。

<!-- section: pr -->
## 评审并合并 PR #1

main、默认分支及 PR 均已建立。基分支继续保留空提交，使 PR 展示完整项目。
以下只读检查可确认预期历史：

```sh
git merge-base --is-ancestor 35b182edd61ebca3aeae091352fe7d734ebcc9a3 HEAD
git ls-tree -r --name-only 35b182edd61ebca3aeae091352fe7d734ebcc9a3
git diff --shortstat main...review/open-source-preparation
```

第一条成功退出，第二条无输出，第三条汇总完整的 211 文件导入。
保留已有历史，无需重新创建分支、改名或变基。

维护者剩余操作：

1. 评审 [PR #1](https://github.com/pachi-qwq/lamp-rf-bridge/pull/1)、
   完整源码 Diff 及最新 HEAD 的检查。所有者已接受当前源码 PR 暂缓实机回归。
2. 批准且 firmware、quality 检查成功后，使用 merge commit 保留已评审提交。
   本次文档同步不合并 PR。
3. 合并后为 main 开启禁止直接/强制推送、要求实际 firmware、quality 检查成功的保护。
   此流程不创建标签或 Release。

空 main 合并工作流之前，该默认分支的手动 workflow_dispatch 不可用；
使用评审分支推送及 PR 检查，见 [GitHub 事件要求](https://docs.github.com/en/actions/reference/workflows-and-actions/workflow-syntax#onworkflow_dispatch)。

<!-- section: owner -->
## 仓库资料及剩余所有者操作

仓库当前 Description 为：

> An ESP32 + CC1101 RF bridge for capturing and replaying 433 MHz signals, with local web control for compatible devices.

Topics 为 433mhz、arduino、cc1101、esp32、home-automation、ook、platformio、rf-bridge、
rf-remote、web-control。Homepage 为空，没有长期维护的项目站点前可保持留空。

- 启用 Private Vulnerability Reporting，并确定私密行为投诉渠道。
  两份政策均未虚构邮箱或声称相关设置已开启。
- 评审分支包含 [MIT LICENSE](../../LICENSE)。默认 main 仍为空，
  GitHub 当前未检测到该分支的许可证；合并文件后再核对识别结果。
- 保留 README 中英文入口；如添加 CI 徽章，必须指向真实工作流及分支。
- 审阅公开 RF 样本，只用于自有或获准设备。不上传包含 build/private、缓存、
  原始日志或 .git 的整个工作目录压缩包。

本次文档同步不修改仓库设置。

<!-- section: firmware -->
## 分发固件二进制前

- 针对准确的新二进制执行启动、Wi-Fi、SPI、登录/信任/重启、网络恢复及基本灯效检查，
  并记录哈希；定时到期仍待验证。
- 保留 Arduino/SDK 声明，按 LGPL 义务提供对应源码、重链接材料，核对 SDK/无线库条款。
  完整二进制包尚未准备；应用源码 MIT 不表示整份固件仅 MIT。
- 记录 Git 版本、环境、依赖和实际硬件。
  不从 CI 成功推导载波/功率/距离校准、更多设备兼容性或灯具状态回读。

实机回归与二进制分发义务阻塞已验收固件的正式发布。
最新 PR HEAD 的检查成功与所有者评审仍是源码 PR 合并条件。本轮文档/CI 整改未改运行代码，
所有者接受暂缓实机回归，无需中断现有设备使用；该决定不代表早先认证计时改动已实测。

<!-- section: maintenance -->
## 后续维护

同步两种语言，依赖变更与 RF 行为分开审查，保留采样来源。
安全修复与文档调整分别记录。新采样明确审核后再公开。
本地私密备份仅供恢复，不作为公开分发材料。
