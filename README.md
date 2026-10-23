# lamp-rf-bridge

[简体中文](README.zh-CN.md) · [Documentation](docs/en/index.md)

An ESP32 + CC1101 project for learning RF capture, analyzing pulse-coded signals, and controlling
a compatible device from a local web browser. The bundled profile comes from **one tested floor
lamp**. Other 433 MHz devices need their own captures and validation; matching frequency alone
doesn't establish compatibility.

## What works

- SPI identity diagnostics with readiness timeouts and consecutive-sample checks.
- Asynchronous OOK reception, GPIO-edge timestamps, CSV export and offline PWM candidate analysis.
- Hardware-RMT replay of seven captured commands. Power, brightness ±, color temperature ± and night
  mode were observed working on the test lamp. The captured 10-minute timer was not lamp-tested.
- Password-protected local web control, 2.4 GHz Wi-Fi provisioning/reconnection, remembered devices,
  and an iPhone Home Screen interface. The current device UI and serial messages are in Chinese.
- Separate diagnostic, capture, replay and web firmware environments; pinned build dependencies.

## Hardware

The verified setup uses a Waveshare **ESP32-DEV-KIT-WROOM-32E-N4-M** (4 MB Flash) and a **CC1101,
E07-M1101D-like 433 MHz SPI module** with a spring antenna. The module's exact manufacturer is
unconfirmed. Power CC1101 from **3.3 V**, never 5 V. Read the
[pin table and electrical notes](docs/en/hardware.md) before connecting it.

## Quick start

Use Python 3.11. From the repository root in Windows PowerShell:

```powershell
python -m venv .venv
.\.venv\Scripts\python -m pip install -r requirements-lock.txt
.\.venv\Scripts\python -m platformio run --environment phase4
```

This builds without hardware. For USB driver checks, the first SPI test, flashing, Linux/macOS
commands and first-time Wi-Fi setup, follow [Getting started](docs/en/getting-started.md).
Do not assume the bundled commands match your lamp.

## Documentation

| Topic | Guide |
| --- | --- |
| First build, SPI check and deployment | [Getting started](docs/en/getting-started.md) |
| Wiring and power | [Hardware](docs/en/hardware.md) |
| Daily control, login and Home Screen use | [User guide](docs/en/user-guide.md) |
| Central constants and generated assets | [Configuration](docs/en/configuration.md) |
| Captures, timing, replay and compatibility | [RF protocol](docs/en/rf-protocol.md) |
| Modules and data flow | [Architecture](docs/en/architecture.md) |
| HTTP interface | [API reference](docs/en/api.md) |
| Building, tests and CI | [Development](docs/en/development.md) · [Testing](docs/en/testing.md) |
| Failures and diagnosis | [Troubleshooting](docs/en/troubleshooting.md) |
| Preparing a release | [Release checklist](docs/en/release-checklist.md) |

## Scope and limits

The web firmware exposes a fixed command whitelist, not a universal RF learning UI. Capture uses a
separate firmware image with Wi-Fi disabled. Power is a **toggle**; there is no verified lamp-status
query, independent absolute on/off command, brightness percentage or Kelvin feedback.

The Home Screen interface is an **online LAN web app**. HTTP deployment has no Service Worker,
offline control, TLS, cloud service or OTA. Your phone must reach the gateway's network. No range,
radiated-power or broad device-compatibility claim is made. See
[testing boundaries](docs/en/testing.md) and [security policy](SECURITY.md).

No hardware photos, control-page screenshots or demo video are bundled. The SVG/PNG assets in
`web/` are application artwork, not evidence of hardware operation.

## Contributing

Read [CONTRIBUTING.md](CONTRIBUTING.md) and the [code of conduct](CODE_OF_CONDUCT.md).
Keep meaningful first-party comments bilingual, update both documentation languages together, and
separate software results from hardware observations. Report vulnerabilities privately as described
in [SECURITY.md](SECURITY.md).

## License

Project-owned source, documentation and artwork are under the [MIT License](LICENSE),
Copyright (c) 2026 pachi-qwq. Third-party components retain their own licenses, including
Arduino-ESP32's LGPL terms; see [third-party notices](THIRD_PARTY_NOTICES.md).
