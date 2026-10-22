# 发布前检查表

[English](../en/release-checklist.md) · [文档导航](index.md)

本表用于准备首次 PR 和发布。仓库及原版 CI 已存在，整理后的固件仍需实机回归。
本轮评审只创建本地提交，不推送分支、创建 PR、修改仓库设置或安装固件。

<!-- section: status -->
## 当前状态

| 状态 | 项目 | 证据或剩余操作 |
| --- | --- | --- |
| 已完成 | 公开仓库及评审分支 | [pachi-qwq/lamp-rf-bridge](https://github.com/pachi-qwq/lamp-rf-bridge)，review/open-source-preparation |
| 已验证 | 原版 firmware、quality Job | [CI 39805827721](https://github.com/pachi-qwq/lamp-rf-bridge/actions/runs/39805827721)，提交 8ca1b79；两者成功 |
| 已验证 | 空的共同祖先 | 35b182edd61ebca3aeae091352fe7d734ebcc9a3 是祖先，提交树为空 |
| 本地已完成 | Actions 升级至 Node.js 24 | 稳定版本固定 SHA；更新后的工作流仍待远程运行 |
| 待人工操作 | main、默认分支及 PR | 远程仅有评审分支，当前默认分支也是它 |
| 待人工操作 | 私密报告及仓库资料 | Private Vulnerability Reporting 未开启；简介、Topics 为空 |
| 源码 PR 暂缓 | 整理后固件回归 | 所有者接受暂缓，固件发布前仍待验证；见[测试顺序](testing.md#cleanup-regression) |
| 待二进制发布准备 | 第三方分发材料 | LGPL 对应源码、重链接材料及 SDK 条款 |
| 0.1.0 暂不实施 | OTA、云端、离线控制及广泛兼容 | 超出已验证实现 |

原版 CI 成功对应 **8ca1b79**，不代表尚未推送的 Actions 升级也已通过。
旧 JSON 继续只证明其原始固件的历史结果。

<!-- section: version -->
## 首次版本

完成发布检查后建议采用 **0.1.0**。单设备配置与诊断已在文档范围内工作，
兼容、安全及硬件矩阵仍属实验阶段。实际标签建立前保持 Unreleased。
后续按 SemVer 管理已文档化 API 和配置，1.0 前的不兼容变更也须明确说明。

<!-- section: pr -->
## 准备首次 PR

现有历史支持该方案，无需变基、合并无关历史或重复导入。在评审分支执行以下只读检查：

```sh
git merge-base --is-ancestor 35b182edd61ebca3aeae091352fe7d734ebcc9a3 HEAD
git ls-tree -r --name-only 35b182edd61ebca3aeae091352fe7d734ebcc9a3
git diff --stat 35b182edd61ebca3aeae091352fe7d734ebcc9a3...HEAD
```

第一条成功退出；第二条无输出；第三条列出整个项目。
初次导入涉及 210 个文件，后续整改继续叠加在该 Diff 上。
main 应指向空基线，而非评审分支顶端，PR 才会包含完整源码导入。

所有者批准本轮评审后，再**手动**按以下顺序操作。
这些命令仅供准备，本轮**没有执行**：

```sh
git push origin review/open-source-preparation
git push origin 35b182edd61ebca3aeae091352fe7d734ebcc9a3:refs/heads/main
gh repo edit pachi-qwq/lamp-rf-bridge --default-branch main
```

1. 确认第一步推送触发的新 firmware、quality Job 均成功。
2. 从空基线建立 main，再由管理员设为默认分支。
   GitHub 要求仓库有多个分支后才能[更改默认分支](https://docs.github.com/en/repositories/configuring-branches-and-merges-in-your-repository/managing-branches-in-your-repository/changing-the-default-branch)。
3. 创建 **base main**、**compare review/open-source-preparation** 的 PR，
   检查完整源码 Diff 及最新 PR 检查。
4. 记录所有者对源码 PR 实机回归的暂缓决定，在新固件二进制发布验收前再完成回归。
5. 使用 merge commit 保留已评审提交，再为 main 开启禁止直接/强制推送、
   必须通过实际 firmware、quality 检查的保护。

空 main 合并工作流之前，该默认分支的手动 workflow_dispatch 不可用；
使用评审分支推送及 PR 检查，见 [GitHub 事件要求](https://docs.github.com/en/actions/reference/workflows-and-actions/workflow-syntax#onworkflow_dispatch)。
不要将评审分支直接改名为 main，也不重写现有两个提交。

<!-- section: owner -->
## 其他所有者操作

- 审阅公开 RF 样本，只用于自有或获准设备。不上传包含 build/private、缓存、
  原始日志或 .git 的整个工作目录压缩包。
- 启用 Private Vulnerability Reporting，并确定私密行为投诉渠道。
  两份政策均未虚构邮箱或声称相关设置已开启。
- 建议 Description：**ESP32 and CC1101 RF bridge with OOK capture, pulse analysis, and local
  web control for a verified lamp profile.**
- 建议 Topics：esp32、cc1101、rf-bridge、433mhz、arduino、platformio、home-automation、ook。
  没有长期维护的独立站点前，Homepage 可留空。
- GitHub 已识别 MIT。保留 README 语言切换；如添加 CI 徽章，必须指向真实工作流及分支。

本轮只读检查这些设置，仍由所有者手动操作。

<!-- section: firmware -->
## 分发固件二进制前

- 针对准确的新二进制执行启动、Wi-Fi、SPI、登录/信任/重启、网络恢复及基本灯效检查，
  并记录哈希；定时到期仍待验证。
- 保留 Arduino/SDK 声明，按 LGPL 义务提供对应源码、重链接材料，核对 SDK/无线库条款。
  完整二进制包尚未准备；应用源码 MIT 不表示整份固件仅 MIT。
- 记录 Git 版本、环境、依赖和实际硬件。
  不从 CI 成功推导载波/功率/距离校准、更多设备兼容性或灯具状态回读。

实机回归与二进制分发义务阻塞已验收固件的正式发布。
更新后 CI 和所有者评审仍是源码 PR 合并条件。本轮文档/CI 整改未改运行代码，
所有者接受暂缓实机回归，无需中断现有设备使用；该决定不代表早先认证计时改动已实测。

<!-- section: maintenance -->
## 后续维护

同步两种语言，依赖变更与 RF 行为分开审查，保留采样来源。
安全修复与文档调整分别记录。新采样明确审核后再公开。
本地私密备份仅供恢复，不作为公开分发材料。
