# Testing and verification

[简体中文](../zh-CN/testing.md) · [Documentation](index.md)

Software checks, historical hardware evidence and unrun tests are distinct. No hardware was
connected, flashed or RF-transmitted during the open-source cleanup.

<!-- section: local -->
## Local software results

On the recorded Windows environment:

| Check | Actual result |
| --- | --- |
| PlatformIO esp32dev / phase2 / phase3 / phase4 | All four built successfully |
| C++ logic | 273 checks passed: identity 29, RF math 12, replay 189, web/auth 43 |
| Python real-capture regression | 5 tests passed |
| JavaScript MD5/Digest vectors | 8 checks passed using actual vendor/client code |
| Workflow syntax | actionlint 1.7.7 passed |
| Cross-platform generators | Both --check commands passed on Ubuntu/WSL Python 3.12.3 |
| Markdown, repository consistency and diff | See [current audit record](../evidence/open-source-audit.json) |

The current web build used 895689 / 1310720 application bytes (68.3%) and 49680 / 327680 static RAM
bytes (15.2%); dynamic heap is additional. Its binary SHA-256 is
`606c73da7fde56dd3705b9baf19658b5eda139ff0a5daf23086a04c17b3f1119`.
This binary was **not flashed**. Source formatting, bilingual comments, preserved CSV bytes,
canonical gzip metadata and an embedded third-party license are included. The separate
authentication expiry fix has ten new boundary checks; it is not a claim of a 49-day physical run.

Use the [development commands](development.md) to reproduce the software checks.
GitHub-hosted CI and the POSIX host compiler branch have not run here; no badge claims otherwise.

<!-- section: history -->
## Historical real-device evidence

Earlier sessions verified the wired board's identity, capture and lamp behavior before cleanup:

| Scope | Observation / evidence |
| --- | --- |
| SPI | Stable PARTNUM=0x00, VERSION=0x14, IDLE=0x01 after jumper replacement |
| Capture | Seven labeled button types, repeated full candidates and retained source CSV |
| Replay | Power both directions, brightness ±, temperature ± and night mode visibly worked |
| Timer | Captured, implementation retained; lamp effect/expiry deliberately skipped |
| Android | Setup AP, home 2.4 GHz provisioning, login and lamp control accepted |
| iPhone | Login/control, Home Screen icon/layout, device trust, autofill and button feedback accepted |
| Startup | Owner reported under one second after idle-preconnection server fix |

Structured operator records:
[replay](../evidence/historical-phase3-validation.json) and
[web](../evidence/historical-phase4-validation.json).
The earlier six lamp-effect checks used three frames. After shortening adjustments to two frames,
the owner accepted response speed, but no fresh exhaustive button matrix was recorded.

The latest earlier network/auth set had **87 checks** and no RF commands:
[form login 27](../evidence/20261022_014238_form-login-checks.json),
[real reboot 6](../evidence/20261022_014356_form-login-reboot-checks.json),
[HTTP 35](../evidence/20261022_014242_http-checks.json),
[Digest 8](../evidence/20261022_014348_digest-checks.json),
[preconnection 11](../evidence/20261022_014250_preconnect-checks.json).
These belong to the earlier firmware, not the new cleanup binary.
Before-fix and failed files remain in the evidence directory with their original outcomes;
in particular, the older cross-URI test needed correction after the static shell became public.

<!-- section: hardware -->
## Re-run on hardware deliberately

Scripts `check_http.py`, `check_digest.py`, `check_preconnect.py`, `check_trust.py` and
`check_form_login.py` require a real gateway URL and a private access file produced by
`check_phase4_serial.py`. They must not be run in offline CI. Review each `--help` first.
The network checks avoid valid RF commands; trust/login checks temporarily mutate real credentials
and need their documented cleanup/reboot continuation.

`check_trust.py --resume` and `check_form_login.py --resume` require the saved local test data
and an actual ESP32 restart between runs. Form-login checks also need Node.js.
Keep every access file/token under ignored build/private and remove temporary persistent trust.
A real-device test is not a software mock.

Before releasing the cleanup firmware, run its actual startup, form login, trust/reboot and
basic authorized lamp checks. Observe one command at a time; never infer lamp effects from HTTP
200 or RMT completion. Binary release compliance is a separate prerequisite.

<!-- section: limits -->
## Unverified limits

There is no exhaustive board/device compatibility matrix, measured working distance or spectrum/
power calibration, 10-minute expiry test, replacement-supply cold-start acceptance, fault-injection
campaign, long-duration stress run, real eight-hour session wait or full browser automation.
The iPhone feedback is from one phone/display mode. Hardware evidence cannot establish regulatory
compliance, radio confidentiality, rolling-code support or real lamp-state readback.
