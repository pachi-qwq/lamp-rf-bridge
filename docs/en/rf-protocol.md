# RF protocol, capture and replay

[简体中文](../zh-CN/rf-protocol.md) · [Documentation](index.md)

This guide separates observed data from unverified protocol assumptions. Only one anonymous floor
lamp/remote pair has been tested. A reported PCB marking, `NS-021-RF01`, is unconfirmed and is not
a compatibility identifier.

<!-- section: scope -->
## Chip capability versus project support

TI's CC1101 supports more radio configurations than this project implements. The recorded module
is a 433 MHz board; the implemented path is **asynchronous ASK/OOK**, nominally **433.92 MHz**.
Neither other modulation modes nor other bands are supported profiles here.

The selected receive settings captured this remote and the replay controlled the lamp. That does
not measure the remote's actual carrier center, crystal tolerance, modulation depth, radiated
power or operating range. Matching frequency, modulation or 24-bit length is insufficient to claim
that another device is compatible. Rolling-code handling, pairing, checksums and general protocol
learning are not implemented.

<!-- section: receive -->
## Receive configuration and capture

`SpiDevice::configureReceive()` configures PKTCTRL0=0x30 (asynchronous serial, no packet CRC or
whitening), MDMCFG2=0x30 (ASK/OOK, no Manchester/sync-word detection), IOCFG0=0x0D (asynchronous data)
and IOCFG2=0x0E (carrier sense). The default demodulator bandwidth is 203125 Hz, configured rate
4797.935 baud, assuming a 26 MHz module crystal. FOCCFG uses FOC_LIMIT=0; AGC/profile constants are
in [radio_config.h](../../include/radio_config.h). Writes are read back.

Capture is a separate `phase2` image. GPIO26 both-edge interrupts timestamp with
`esp_timer_get_time()`. Storage holds 1024 pre-trigger and 16384 post-trigger pulses. The ISR
never allocates, prints or performs SPI; the main loop polls RSSI and triggers recording. GDO2
carrier sense was often high in background tests and is not the sole trigger.

After stopping, the firmware exports packed pulses over serial. Bit 0 is level, higher bits are
microseconds. CSV restores `index,level,duration_us,segment`. Metadata records overflow, clipped
prehistory, ambiguous same-level callbacks, zero durations, RSSI and possibly partial boundaries.
The unfinished tail has its own duration/level. Ambiguous callbacks are not a measured packet-loss
count. Independent timing and absolute RSSI calibration have not been performed.

<!-- section: workflow -->
## Collect and analyze

Close other serial monitors first. Replace COM4 with your port:

```powershell
.\.venv\Scripts\python -m platformio run --environment phase2 --target upload --upload-port COM4
.\.venv\Scripts\python scripts/listen_rf.py --port COM4 --label power --seconds 1 --trigger -80
```

Wait for ARMED, then press only the chosen button briefly. The listener defaults to six windows
of up to 60 seconds waiting plus a one-second recording; untriggered windows are retained.
`collect_rf.py` exposes `--immediate`, `--wait`, `--frequency-khz`, `--profile` and
`--output-dir`. Direct recording is limited to 0.1..10 s, waiting 0..60 s and RSSI -120..0 dBm.
Tuning 430000..437000 kHz affects reception only.

For an existing real sample:

```powershell
.\.venv\Scripts\python scripts/analyze_rf.py captures/20261021_225552_366188_power_on_clean_01_wait1.csv --gap-us 5000
.\.venv\Scripts\python tests/test_analysis.py
```

The analyzer estimates complementary short/long pulse clusters and walks backward from candidate
gaps. It does not pad to 24 bits or repair missing edges. Its `candidate_only=true` and
`protocol_confirmed=false` flags deliberately keep analysis claims conservative.
Collect the same physical key repeatedly before assigning a command label.

<!-- section: observations -->
## Recorded encoding and commands

The analysis convention is long-high/short-low = 1, short-high/long-low = 0, first bit most
significant. Complete candidates have 24 such pairs followed by a short high and long low delimiter.
No independent preamble, address field or checksum rule has been established. Earlier unlabelled
debug samples included FFFF08; it is not a supported button.

| Command name | Display hex | Matching complete frames | Selected real frame |
| --- | --- | --- | --- |
| power | FFFF02 | 5 | [power.csv](../../captures/frames/power.csv) |
| brightness_up | FFFF0A | 9 | [brightness_up.csv](../../captures/frames/brightness_up.csv) |
| brightness_down | FFFF0C | 12 | [brightness_down.csv](../../captures/frames/brightness_down.csv) |
| cct_up | FFFF12 | 9 | [cct_up.csv](../../captures/frames/cct_up.csv) |
| cct_down | FFFF10 | 9 | [cct_down.csv](../../captures/frames/cct_down.csv) |
| night_mode | FFFF04 | 6 | [night_mode.csv](../../captures/frames/night_mode.csv) |
| timer_10min | FFFF06 | 6 | [timer_10min.csv](../../captures/frames/timer_10min.csv) |

Across 37 normally delimited full frames: short median 408 µs (374..490), long median 1190.5 µs
(1113..1226), delimiter-low median 13817 µs (13771..13848), full-frame median 52678 µs.
These are measured clusters, not idealized transmission constants. The
[capture index](../../captures/index.json) records source ranges for each selected 50-pulse frame.
The generator checks values and provenance against the original CSV; it preserves each measured duration.

The power key is one physical toggle, not separate ON/OFF commands. Six command types were
observed controlling the lamp. The timer label was reported as 10 minutes; expiry was not tested.
No lamp acknowledgement or state-query protocol has been verified.

<!-- section: replay -->
## Replay and limits

```powershell
.\.venv\Scripts\python -m platformio run --environment phase3 --target upload --upload-port COM4
.\.venv\Scripts\python scripts/replay_power.py --port COM4
.\.venv\Scripts\python scripts/replay_power.py --port COM4 --send --button power
```

The first script call only checks health; **--send intentionally transmits one command**.
Do not run it against a device without authorization. No automatic retry follows a timeout.
Use `--help` for the same seven allowed names, and restore `phase4` for web operation afterward.

TX makes CC1101 GDO0 high impedance before ESP32 drives it. PATABLE uses 0x00 for low and 0x34 for
high with FREND0=0x11, TI's typical -10 dBm 433 MHz entry. RMT uses a verified 1 MHz clock, two
memory blocks and loop mode off. A command repeats a measured frame twice for adjustments, three
times for power/night/timer, adding 14 ms low before and 3 ms after. Maximum three-frame storage is
152 pulses / 76 RMT items, fitting entirely in hardware.

Plan validation requires exactly 50 alternating pulses, payload durations 200..1600 µs and final
low 10..20 ms, total at most 200 ms. Software waits at most 300 ms. Normal completion/error cleanup
stops RMT, requests SIDLE and reads final state; failures latch controls unavailable.
Minimum cooldown is 20 ms after an attempt. Startup and status never transmit. “Completed” confirms
only timing/TX/IDLE checks. Original six-key effect observations used three frames; the shorter
adjustment version received positive latency feedback but not a fresh exhaustive six-key matrix.

See the [TI datasheet](https://www.ti.com/lit/ds/symlink/cc1101.pdf),
[TI DN022](https://www.ti.com/lit/an/swra215e/swra215e.pdf) and
[ESP-IDF 4.4 RMT reference](https://docs.espressif.com/projects/esp-idf/en/v4.4.7/esp32/api-reference/peripherals/rmt.html).
