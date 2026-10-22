# Release checklist

[简体中文](../zh-CN/release-checklist.md) · [Documentation](index.md)

Use this checklist to prepare the first PR and release. The repository and original CI now exist;
the cleaned-up firmware still needs hardware regression. The current review creates local commits
only: it does not push branches, open a PR, change repository settings or install firmware.

<!-- section: status -->
## Current status

| State | Item | Evidence or remaining action |
| --- | --- | --- |
| Completed | Public repository and review branch | [pachi-qwq/lamp-rf-bridge](https://github.com/pachi-qwq/lamp-rf-bridge), review/open-source-preparation |
| Verified | Original firmware and quality jobs | [CI run 39805827721](https://github.com/pachi-qwq/lamp-rf-bridge/actions/runs/39805827721), commit 8ca1b79; both succeeded |
| Verified | Empty common ancestor | 35b182edd61ebca3aeae091352fe7d734ebcc9a3 is an ancestor with an empty tree |
| Completed locally | Node.js 24 action upgrade | Stable releases pinned by SHA; updated workflow still needs its own remote run |
| Pending manual action | main, default branch and PR | Only the review branch exists remotely and is currently the default |
| Pending manual action | Private reporting and metadata | Private Vulnerability Reporting is disabled; description/topics are empty |
| Deferred for source PR | Cleanup firmware regression | Owner accepts deferral; remains pending before firmware release; see the [test sequence](testing.md#cleanup-regression) |
| Pending binary release | Third-party distribution materials | LGPL corresponding sources/relinking materials and SDK terms |
| Not planned for 0.1.0 | OTA, cloud, offline control and broad compatibility | Outside the verified implementation |

Original CI success applies to **8ca1b79**, not to the unpushed action upgrade.
Earlier historical JSON remains evidence for its original firmware only.

<!-- section: version -->
## First version

Recommend **0.1.0** after release checks. The single-device profile and diagnostics work within
their documented scope; compatibility, security and the hardware matrix remain experimental.
Keep changes under Unreleased until an actual tag exists. Future versions follow SemVer for the
documented API/configuration, including explicit notices for incompatible changes before 1.0.

<!-- section: pr -->
## Prepare the first PR

The existing history supports the proposed workflow without rebasing, unrelated histories or
duplicate imports. These read-only checks must succeed from the review branch:

```sh
git merge-base --is-ancestor 35b182edd61ebca3aeae091352fe7d734ebcc9a3 HEAD
git ls-tree -r --name-only 35b182edd61ebca3aeae091352fe7d734ebcc9a3
git diff --stat 35b182edd61ebca3aeae091352fe7d734ebcc9a3...HEAD
```

The first exits successfully; the second prints nothing; the third lists the complete project.
The initial import changes 210 files; later remediation adds to that diff. Establish main at the
empty baseline, not at the review tip, so the PR contains the source import.

After the owner approves this review, perform the following **manually**. These commands are
preparation instructions and were **not executed** by the remediation:

```sh
git push origin review/open-source-preparation
git push origin 35b182edd61ebca3aeae091352fe7d734ebcc9a3:refs/heads/main
gh repo edit pachi-qwq/lamp-rf-bridge --default-branch main
```

1. Confirm the first push's new firmware and quality jobs succeed.
2. Create main from the empty baseline, then select it as the default branch with administrator
   permission. GitHub requires more than one branch to [change the default](https://docs.github.com/en/repositories/configuring-branches-and-merges-in-your-repository/managing-branches-in-your-repository/changing-the-default-branch).
3. Open a PR with **base main** and **compare review/open-source-preparation**. Inspect the full
   source diff and latest PR checks.
4. Record the owner's hardware-regression deferral for the source PR. Complete the regression
   before accepting a new firmware binary for release.
5. Merge using a merge commit to retain the reviewed commits, then enable main protection against
   direct/force pushes and require the actual firmware and quality checks.

Until the empty main receives the workflow through the merge, manual workflow_dispatch is
unavailable on that default branch; use the review push and PR checks. See
[GitHub's event requirements](https://docs.github.com/en/actions/reference/workflows-and-actions/workflow-syntax#onworkflow_dispatch).
Do not rename the review branch into main or rewrite either existing commit.

<!-- section: owner -->
## Other owner actions

- Review public RF fixtures and use only your own or authorized devices. Do not upload a workspace
  ZIP containing build/private, caches, raw logs or .git.
- Enable Private Vulnerability Reporting and designate a private conduct-reporting channel.
  Neither policy invents an email or claims those settings are enabled.
- Suggested description: **ESP32 and CC1101 RF bridge with OOK capture, pulse analysis, and local
  web control for a verified lamp profile.**
- Suggested topics: esp32, cc1101, rf-bridge, 433mhz, arduino, platformio, home-automation, ook.
  Leave Homepage empty until there is a maintained project site.
- MIT is already recognized by GitHub. Keep English/Chinese README links; an optional CI badge must
  point to the real workflow and branch.

These settings were inspected read-only and remain manual owner actions.

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
Updated CI and owner review remain source-PR merge gates. The owner deferred hardware regression
for this documentation/CI follow-up because it changes no runtime code; existing device use need
not be interrupted. This decision does not verify the earlier authentication clock changes.

<!-- section: maintenance -->
## Maintenance checks

Synchronize both languages, review dependency changes separately from RF behavior, and preserve
capture provenance. Report security fixes separately from prose. New recordings stay ignored until
deliberately reviewed. Local private backups are recovery aids, not public distribution artifacts.
