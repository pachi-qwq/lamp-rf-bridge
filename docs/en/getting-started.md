# Getting started

[简体中文](../zh-CN/getting-started.md) · [Documentation](index.md)

Build and deploy the verified board configuration. You need Python 3.11, a USB data cable,
a wired ESP32/CC1101 pair and a serial driver for your board. Read [Hardware](hardware.md) first.

<!-- section: tools -->
## Install the tools

Windows PowerShell, from the repository root:

```powershell
python --version
python -m venv .venv
.\.venv\Scripts\python -m pip install -r requirements-lock.txt
.\.venv\Scripts\python -m platformio device list
```

Linux/macOS:

```sh
python3.11 -m venv .venv
.venv/bin/python -m pip install -r requirements-lock.txt
.venv/bin/python -m platformio device list
```

On POSIX, replace the interpreter prefix in subsequent examples with `.venv/bin/python`.
The pinned PlatformIO Core is 6.1.19; the platform is espressif32 6.12.0 and Arduino-ESP32 is
2.0.17. The first build downloads the pinned toolchain. Node.js is only needed for developer
checks and the form-login test, not normal firmware compilation.

The verified CH343P board appeared as COM4. Use your listed port instead; on Linux this is often
a ttyUSB device. A power-only USB cable will not expose a serial port. Install the board vendor's
CH343 driver if the OS reports an unknown device. Don't select an unrelated motherboard serial port.

<!-- section: spi -->
## Verify SPI before using RF

With the verified wiring and antenna attached:

```powershell
.\.venv\Scripts\python -m platformio run --environment esp32dev
.\.venv\Scripts\python -m platformio run --environment esp32dev --target upload --upload-port COM4
.\.venv\Scripts\python -m platformio device monitor --port COM4 --baud 115200
```

Replace COM4 with your port. This diagnostic image never transmits. Require ten consecutive valid
identity samples and the final PASS line. PARTNUM must be 0x00, VERSION 0x04 or 0x14, the ready bit
clear and MARCSTATE 0x01 (IDLE). All-zero and all-0xFF responses fail; PARTNUM=0 alone is insufficient.
Stop the monitor with Ctrl+C before another tool opens the port. See
[Troubleshooting](troubleshooting.md) for failures.

<!-- section: deploy -->
## Install the web firmware

The generated headers are included. After source or web changes, regenerate them before building:

```powershell
.\.venv\Scripts\python scripts/generate_replay_frames.py
.\.venv\Scripts\python scripts/embed_web.py
.\.venv\Scripts\python -m platformio run --environment phase4
.\.venv\Scripts\python -m platformio run --environment phase4 --target upload --upload-port COM4
.\.venv\Scripts\python scripts/check_phase4_serial.py --port COM4
```

The last command reads real status and private access details into ignored `build/private/`.
Its terminal output includes the device password: don't attach it to a public issue.
The web image uses 460800 baud upload; serial remains 115200. If upload is unreliable, add
`--upload-speed 115200` to the upload command.

<!-- section: setup -->
## Configure your network

1. Connect your phone to the `Lamp-xxxxxx` hotspot using the password shown by the serial
   `access` command or the private serial checker output.
2. Open the setup IP printed in the `setup_ap` serial record. Log in as `admin` with the same password.
3. Enter your home 2.4 GHz SSID and password. Wait for the page to show the assigned LAN address.
4. Reconnect the phone to your home network and open that address.

The hotspot closes three minutes after a successful home-network connection. Wi-Fi credentials
and the generated access password survive reset. A physical USB `setup` command reopens provisioning.
Never put your household credentials in source files. Start with the
[user guide](user-guide.md) before transmitting: the bundled profile may not match your device.

<!-- section: power -->
## Run without a computer

Keep CC1101 connected to ESP32 3V3 and GND. Move only the ESP32 USB cable from the computer to a
5 V / 1 A USB supply. The firmware doesn't wait for a serial monitor and reconnects to saved Wi-Fi.
A real cold-start test on the replacement supply is still required; previous reset tests do not
establish that supply's stability. DHCP can assign a new IP after restarting; a router reservation
can keep a Home Screen URL stable.
