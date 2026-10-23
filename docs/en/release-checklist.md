# Release checklist

[简体中文](../zh-CN/release-checklist.md) · [Documentation](index.md)

Use this checklist to complete [PR #1](https://github.com/pachi-qwq/lamp-rf-bridge/pull/1) and prepare
the first release. main exists at the empty baseline and is the default branch. The PR remains
open; the cleaned-up firmware still needs the hardware regression deferred by the owner.

<!-- section: status -->
## Current status

| State | Item | Evidence or remaining action |
| --- | --- | --- |
| Completed | Repository and review branch | [pachi-qwq/lamp-rf-bridge](https://github.com/pachi-qwq/lamp-rf-bridge), review/open-source-preparation |
| Completed | main and default branch | main points to empty baseline 35b182edd61ebca3aeae091352fe7d734ebcc9a3 and is the default |
| Completed | First formal PR | [PR #1](https://github.com/pachi-qwq/lamp-rf-bridge/pull/1): main ← review/open-source-preparation; open and unmerged |
| Verified | PR firmware and quality jobs | [Run 39817485832](https://github.com/pachi-qwq/lamp-rf-bridge/actions/runs/39817485832), commit 66dcbfd; both succeeded |
| Verified | Node.js 24 action upgrade | The upgraded Actions passed both the push and PR runs; see [CI results](testing.md#hosted-ci) |
| Completed | Description and Topics | Both are populated; Homepage remains empty |
| Pending manual action | Private reporting and conduct channel | Private Vulnerability Reporting is disabled; designate a private conduct-reporting channel |
| Deferred for source PR | Cleanup firmware regression | Owner accepts deferral; pending before firmware release; see the [test sequence](testing.md#cleanup-regression) |
| Pending binary release | Third-party distribution materials | LGPL corresponding sources/relinking materials and SDK terms |
| Not planned for 0.1.0 | OTA, cloud, offline control and broad compatibility | Outside the verified implementation |

The linked PR run verifies **66dcbfd**, including the action upgrade. Later documentation commits
must pass their own [latest PR checks](https://github.com/pachi-qwq/lamp-rf-bridge/pull/1/checks).
Historical audit JSON retains the state recorded at the time; it is not rewritten to reflect new CI.

<!-- section: version -->
## First version

Recommend **0.1.0** after release checks. The single-device profile and diagnostics work within
their documented scope; compatibility, security and the hardware matrix remain experimental.
Keep changes under Unreleased until an actual tag exists. Future versions follow SemVer for the
documented API/configuration, including explicit notices for incompatible changes before 1.0.

<!-- section: pr -->
## Review and merge PR #1

main creation, default-branch selection and PR creation are complete. The base remains the empty
commit so this PR displays the full project. These read-only checks confirm the intended history:

```sh
git merge-base --is-ancestor 35b182edd61ebca3aeae091352fe7d734ebcc9a3 HEAD
git ls-tree -r --name-only 35b182edd61ebca3aeae091352fe7d734ebcc9a3
git diff --shortstat main...review/open-source-preparation
```

The first exits successfully, the second prints nothing, and the third summarizes the full
211-file import. Preserve the existing history; no branch recreation, renaming or rebase is needed.

Remaining maintainer actions:

1. Review [PR #1](https://github.com/pachi-qwq/lamp-rf-bridge/pull/1), its full source diff and the
   checks for its latest head. The owner has accepted hardware-regression deferral for this source PR.
2. After approval and successful firmware/quality checks, merge using a merge commit to retain the
   reviewed commits. This documentation update does not merge the PR.
3. After merge, protect main against direct/force pushes and require the actual firmware and quality
   checks. No tag or release is created by this procedure.

Until the empty main receives the workflow through the merge, manual workflow_dispatch is
unavailable on that default branch; use review-branch push and PR checks. See
[GitHub's event requirements](https://docs.github.com/en/actions/reference/workflows-and-actions/workflow-syntax#onworkflow_dispatch).

<!-- section: owner -->
## Repository metadata and remaining owner actions

The repository currently uses this description:

> An ESP32 + CC1101 RF bridge for capturing and replaying 433 MHz signals, with local web control for compatible devices.

Topics are 433mhz, arduino, cc1101, esp32, home-automation, ook, platformio, rf-bridge, rf-remote and
web-control. Homepage is empty; leave it empty until there is a maintained project site.

- Enable Private Vulnerability Reporting and designate a private conduct-reporting channel.
  Neither policy invents an email or claims those settings are enabled.
- [MIT LICENSE](../../LICENSE) is present in the review branch. The default main is still empty,
  and GitHub currently reports no detected license there; recheck detection after merging the file.
- Keep English/Chinese README links. An optional CI badge must point to the real workflow and branch.
- Review public RF fixtures and use only your own or authorized devices. Do not upload a workspace
  ZIP containing build/private, caches, raw logs or .git.

This documentation synchronization does not change repository settings.

<!-- section: firmware -->
## Before distributing firmware binaries

- Run startup, Wi-Fi, SPI, login/trust/reboot, network recovery and basic lamp checks against the
  exact new binary. Record its hash; a timer expiry test is still pending.
- Preserve Arduino/SDK notices and provide corresponding sources/relinking materials where LGPL
  requires them. Review SDK/radio-library terms. The complete binary package is not prepared;
  application-source MIT does not make the whole firmware MIT-only.
- Record Git revision, environment, dependencies and actual hardware. Do not infer calibrated
  carrier/power/range, additional devices or lamp-state feedback from CI success.

Hardware regression and binary-distribution obligations block a validated firmware release.
Successful checks for the latest PR head and owner review remain source-PR merge gates.
The owner deferred hardware regression for this documentation/CI follow-up because it changes no
runtime code; existing device use need not be interrupted. This decision does not verify the earlier authentication clock changes.

<!-- section: maintenance -->
## Maintenance checks

Synchronize both languages, review dependency changes separately from RF behavior, and preserve
capture provenance. Report security fixes separately from prose. New recordings stay ignored until
deliberately reviewed. Local private backups are recovery aids, not public distribution artifacts.
