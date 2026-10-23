# Contributing

[简体中文](CONTRIBUTING.zh-CN.md)

Contributions should make the existing capture/replay path easier to verify and maintain.
Read the [development guide](docs/en/development.md) for exact setup and check commands, and
follow the [code of conduct](CODE_OF_CONDUCT.md).

## Before opening an issue

For a bug, include your board/module revision, wiring differences, firmware environment and Git
revision, reproduction steps, expected/actual behavior and relevant sanitized logs. Distinguish
“gateway sent” from an observed device response. Never attach access-password output, Cookies,
Wi-Fi credentials, device MACs or unrelated RF traffic. Report security issues through
[SECURITY.md](SECURITY.md), not the bug template.

For device support, provide the actual modulation/frequency evidence, repeated captures from one
identified key, provenance and authorization to share the data. A matching 433 MHz label is not
a compatibility result. See the [RF guide](docs/en/rf-protocol.md).

## Development and pull requests

1. Create a focused branch and describe the problem and resulting behavior.
2. Keep the pinned ESP32 framework/platform unless an upgrade has its own hardware validation.
3. Regenerate derived headers after changing their inputs; run the software checks and relevant
   firmware builds from the development guide.
4. Explain every RF/GDO/timing change and supply actual before/after observations when hardware
   behavior changes. Mark unavailable hardware tests as **not run**.
5. Update both documentation languages and Unreleased changelog entries for user-visible changes.
6. Submit a PR with the checks you actually ran and any limits.

Do not combine a documentation cleanup with unannounced protocol changes. Keep fixes separate in
the PR description or separate commits. Do not add hardware-response mocks and present them as
real captures. Attach new sample records only after a privacy review; raw serial output stays local.

## Code and documentation conventions

- C++17; run the pinned clang-format on first-party C++ files. Generated headers are checked by
  their generators instead.
- Keep one source implementation. Meaningful first-party comments are English/Chinese pairs.
  Public C++ APIs use bilingual Doxygen with accurate units, input constraints and failure semantics.
- Preserve existing JSON keys, serial parsers and Chinese UI/log messages unless a compatibility
  change is explicitly reviewed. Translating docs does not silently localize the firmware.
- Match filenames and section topic markers in `docs/en/` and `docs/zh-CN/`. Review meaning and
  command coverage in both languages; a structural checker is not a translation review.
- Preserve third-party licenses and unmodified vendor code. Do not add MIT headers to other
  authors' files. New first-party source uses `SPDX-License-Identifier: MIT`.
- Never commit `build/`, local environments, private configuration or raw serial logs.

## License and hardware evidence

Submit only material you have the right to distribute. Project-owned contributions use the root
MIT License unless explicitly identified otherwise; existing third-party and code-of-conduct
licenses remain in force. Include capture dates, firmware revision/build hash, setup, command
count and the operator's actual observations. Do not invent a working distance or compatibility
claim from a successful compile.
