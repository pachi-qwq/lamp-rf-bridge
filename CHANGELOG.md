# Changelog

[简体中文](CHANGELOG.zh-CN.md)

Human-facing changes follow [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).
Future releases will use [Semantic Versioning](https://semver.org/spec/v2.0.0.html).
There is no tagged release or commit history in the audited checkout, so no release dates or
historical version changes are reconstructed. Compare links can be added after a remote and tags exist.

## Unreleased

### Added

- English/Chinese user and developer documentation, bilingual first-party comments and public API docs.
- MIT project license, dependency notices, contribution/security policies and Contributor Covenant 2.1.
- GitHub software-only builds/checks, issue/PR templates and repository consistency checks.
- Offline client Digest vectors and authentication lifetime boundary regression tests.

### Changed

- Host logic tests also support a POSIX C++17 compiler; the PowerShell build wrapper no longer needs rtk.
- Captured CSV bytes are preserved across Git checkouts; gzip metadata is platform-independent.
- Shipped web bundles retain the third-party MD5 license. Raw serial logs and superseded working
  notes remain local, while public evidence omits personal device/network identifiers.

### Security

- RAM session expiry uses a 64-bit monotonic clock. Digest signatures bind the 32-bit timestamp
  epoch to prevent old challenges becoming valid again after millis() wraps.

Existing functionality is an SPI diagnostic image, OOK capture/export and analysis, fixed measured
seven-command replay, and local web provisioning/control. Six lamp effects and mobile control were
historically verified; the timer was not. RF constants and waveforms are unchanged by this cleanup.
