# Configuration reference

[简体中文](../zh-CN/configuration.md) · [Documentation](index.md)

Hardware and protocol settings are compile-time constants. Network credentials are provisioned at
runtime. There is no public API for arbitrary RF data, frequency or power.

<!-- section: files -->
## Configuration files

| File | Responsibility |
| --- | --- |
| [config.h](../../include/config.h) | GPIOs, SPI/serial clocks, reset/readiness timing, identity streak |
| [radio_config.h](../../include/radio_config.h) | Assumed crystal, nominal receive frequency/range and demodulator profiles |
| [replay_config.h](../../include/replay_config.h) | TX power entry, frame repeats, RMT capacity and cooldown |
| [web_config.h](../../include/web_config.h) | HTTP port, username, reconnect/AP/preconnection timing |
| [platformio.ini](../../platformio.ini) | Board, pinned toolchain, firmware source selection and warnings |
| [captures/index.json](../../captures/index.json) | Captured command labels and selected-frame provenance |

The frame lookup header and web header are generated. Change the inputs, not the byte arrays.

<!-- section: defaults -->
## Recorded defaults

| Setting | Value |
| --- | --- |
| Serial / SPI | 115200 baud / 1 MHz, mode 0 |
| CSN / SCK / MOSI / MISO | GPIO5 / GPIO18 / GPIO23 / GPIO19 |
| GDO0 / GDO2 | GPIO26 / GPIO27 |
| Identity streak / ready timeout | 10 samples / 10000 µs |
| Nominal RF / assumed RF crystal | 433920000 Hz / 26000000 Hz |
| Default receive bandwidth / configured rate | 203125 Hz / 4797.935 baud |
| Receive-only tuning range | 430..437 MHz; this is not a TX API |
| TX PATABLE / FREND0 | [0x00, 0x34, 0, 0, 0, 0, 0, 0] / 0x11 |
| Adjustment / mode repetitions | 2 / 3 frames; maximum 3 |
| Added leading / trailing low | 14000 / 3000 µs |
| RMT | Channel 0, 1 µs tick, 2 blocks (128 items), loop off |
| Plan / software-wait limits | 200000 µs / 300 ms |
| Minimum post-attempt cooldown | 20 ms, including errors |
| HTTP / Wi-Fi retry / fallback AP | Port 80 / 15 s / 60 s |
| AP grace / channel / clients | 180 s / 6 / 2 |
| Idle preconnection contention threshold | 200 ms |
| Request-result cache | 8 IDs, RAM only |

The receiver also offers wide (406250 Hz), narrow (81250 Hz) and fast (203125 Hz / 9595.871 baud)
profiles for experiments. These rates configure the CC1101 demodulator, not the remote's confirmed
bit rate. Keep verified defaults until new captures provide evidence for a change.

<!-- section: storage -->
## Runtime storage

NVS namespace `lamp` stores `network` (SSID/password), `access` (random access/AP password) and
`trusted` (up to four token hashes). NVS/Flash encryption is not enabled by this project. The
firmware deliberately disables the framework's automatic Wi-Fi persistence and writes its own
complete credential record.

Changing the username constant does not rotate already issued trust credentials. There is no
password-change, factory-reset or “revoke all” API. Treat physical Flash erasure as destructive:
it removes Wi-Fi, passwords and trust data as well as requiring reinstallation. Do not erase it
as a routine deployment step.

<!-- section: generated -->
## Regenerate derived files

```powershell
.\.venv\Scripts\python scripts/generate_replay_frames.py
.\.venv\Scripts\python scripts/embed_web.py
.\.venv\Scripts\python scripts/generate_replay_frames.py --check
.\.venv\Scripts\python scripts/embed_web.py --check
```

Captured CSV bytes are preserved by Git attributes because provenance hashes depend on them.
Web packing uses fixed gzip time/OS metadata and exact CSP hashes. The bundled third-party MD5
license is included in the HTML script comment. PNG assets are already tracked; only regenerating
them with `scripts/export_icon.py` needs Pillow, which is not a firmware-build dependency.
