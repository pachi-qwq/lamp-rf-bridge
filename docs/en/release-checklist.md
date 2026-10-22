# Release checklist

[简体中文](../zh-CN/release-checklist.md) · [Documentation](index.md)

The repository can be reviewed as source. No GitHub push, repository setting change, public release
or new firmware installation was performed by this cleanup.

<!-- section: version -->
## First version

Recommend **0.1.0** for the first tagged release after the checks below. The project has a verified
single-device profile and useful diagnostics, but its compatibility, security and hardware matrix
are experimental. Do not call the local “phase4” label a released semantic version or invent old
release dates. Keep changes under Unreleased until a real tag exists.

Future versions follow SemVer for documented API/configuration behavior. Before 1.0, communicate
breaking changes explicitly; patch releases should not silently change an installed RF profile.
Add real compare/release links after the owner establishes the remote URL and tags.

<!-- section: owner -->
## Owner actions before GitHub publication

1. Review the complete diff and public RF fixtures. The owner confirmed the MIT line as
   **Copyright (c) 2026 pachi-qwq**; 2026 is the copyright year, not part of the name.
2. Create/select the actual GitHub repository, then make the initial commit and push only the
   reviewed source. No commit or remote existed at audit start. Do not upload a workspace ZIP
   containing build/private, caches, raw logs or .git.
3. Enable GitHub Private Vulnerability Reporting and designate a lasting private conduct-reporting
   channel. The policies do not invent an email or claim those settings already exist.
4. Let the actual GitHub workflow run. Confirm the Ubuntu builds/host tests as well as links and
   formatting before adding a CI badge or protecting a branch around those checks.

These are manual owner actions; this task does not authorize changing remote settings or publishing.

<!-- section: firmware -->
## Before distributing firmware binaries

- Run the cleaned-up binary's real startup, SPI health, login/trust/reboot and basic lamp checks.
  The original lamp effects were verified earlier, but the authentication clock fix has local tests only.
- Preserve Arduino/SDK notices and provide corresponding sources and relinking materials where the
  LGPL requires them. Review prebuilt SDK/radio-library terms; the full binary distribution package
  has not been prepared. Source MIT does not make the complete firmware MIT-only.
- Record the exact Git revision, environment, dependency versions, binary hash and actual hardware.
- State that the timer, carrier/power/range calibration, other devices and real lamp-state feedback
  are not validated. Do not promote a successful CI build into a compatibility claim.

<!-- section: maintenance -->
## Maintenance checks

Keep both language guides synchronized. Review dependency updates separately from RF behavior,
and keep original provenance when adding captures. Report security fixes separately from prose
changes. New local recordings stay ignored until deliberately reviewed. The local baseline backup
is a recovery aid for the owner, not a public artifact or version history.
