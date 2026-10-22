# Troubleshooting

[简体中文](../zh-CN/troubleshooting.md) · [Documentation](index.md)

Preserve the first failure before changing settings. Stop other serial tools before opening a port.
Disconnect USB before touching wiring; do not change several unrelated things at once.

<!-- section: usb -->
## USB and upload

| Symptom | Next check |
| --- | --- |
| No serial device | Use a USB data cable and run PlatformIO device list |
| Unknown CH343 device | Check hardware ID and install the Waveshare-provided CH343 driver |
| Port busy | Close the program holding that port |
| Upload stuck at Connecting | Hold BOOT, press/release EN, then release BOOT and retry |
| Intermittent high-speed upload | Retry the same upload with --upload-speed 115200 |
| Unreadable terminal output | Use 115200 baud and UTF-8; upload speed is a different setting |

<!-- section: spi -->
## SPI and identity

| Symptom | Next check |
| --- | --- |
| Waiting for SO/CHIP_RDYn times out | With power disconnected, inspect CC1101 pin 7 SO/MISO to GPIO19 |
| IDs all 0x00 or all 0xFF | Save the readings, inspect 3V3/common ground and pin orientation |
| Unknown VERSION | Preserve the byte and verify chip markings/revision; do not accept “nonzero” |
| Reading stops when wires move | Replace the suspect jumper, then repeat the ten-sample SPI test |
| Web reports RF unavailable | Diagnose power/wiring and recorded bus/state errors before resetting |

The tested chip returned PARTNUM=0x00 and VERSION=0x14. Part number zero alone is valid but not a
complete communication test. CC1101 voltage was never measured with a meter in the original setup.

<!-- section: network -->
## Network, login and startup

Use the device's current serial/router address, not a copied owner's IP. A phone on 5 GHz can work
with the 2.4 GHz gateway if the router permits LAN traffic; guest isolation or VPN routing can block it.
The .local name is optional, not a replacement for checking the actual IP.

If login fails, open /login and check the random access password from private USB output.
Wrong-password/expired-challenge messages stay in the form. Trust belongs to the exact host and
browser context; clearing site data or changing addresses can require login again.
Do not publish the access output. If no network is configured, use the setup AP printed by serial;
a physical `setup` command reopens it.

Earlier iOS login loops were traced to the pinned server's single global Digest challenge and fixed
with independent signed challenges. An earlier 5..6 s launch delay was reproduced using an idle TCP
preconnection; the current guard yields only under contention after 200 ms. A dark launch screen
only changes the background. Compare the exact revision and sanitized connection evidence before
changing unrelated Wi-Fi/RF settings. The owner reported under-one-second startup after that fix.

<!-- section: rf -->
## Capture and control

If a capture times out, it still saves the window. First confirm the correct physical button was
pressed after ARMED. Compare repeated same-key captures. Only then investigate nominal frequency,
OOK modulation, profile bandwidth/rate, RSSI threshold and noise. Carrier-sense alone was unreliable.

Overflow, clipped prehistory or ambiguous callbacks require data review, not invented missing
bits. Do not send a candidate because its prefix resembles a supported command. The analyzer's
protocol_confirmed=false flag is intentional.

If a command is “sent” but the lamp does not respond, observe distance/obstructions, the correct
profile and actual lamp behavior. Do not repeatedly retry the power toggle automatically.
A different remote may need independent analysis even when it says 433 MHz. There is no verified
lamp-state readback to correct the page illustration.

<!-- section: evidence -->
## Evidence for an issue

Include the firmware environment/revision, board/module details, unchanged or modified wiring,
exact failing command and actual output/error. Remove SSIDs, IPs, MACs, passwords, authorization
headers, Cookies and unrelated RF traffic before sharing. State which checks were not run.
Use the [bug form](../../.github/ISSUE_TEMPLATE/bug_report.yml), or the
[private security route](../../SECURITY.md) for a vulnerability.
