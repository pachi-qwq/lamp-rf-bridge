# Hardware and wiring

[简体中文](../zh-CN/hardware.md) · [Documentation](index.md)

This is the recorded build, not a compatibility list for every ESP32 or CC1101 board.

<!-- section: equipment -->
## Verified equipment

| Item | Recorded configuration |
| --- | --- |
| ESP32 board | Waveshare ESP32-DEV-KIT-WROOM-32E-N4-M |
| Module / Flash | ESP32-WROOM-32E / 4 MB |
| USB | Type-C, CH343P USB-to-serial |
| RF board | CC1101, E07-M1101D-like SPI module, 433 MHz spring antenna; exact vendor unconfirmed |
| PlatformIO board | esp32dev, DIO Flash at 40 MHz |
| Development power | Computer USB |
| Intended standalone power | 5 V / 1 A USB supply to ESP32; replacement-supply cold start not separately verified |

The ESP32 crystal and the assumed **26 MHz CC1101 module crystal** are different clocks.
CC1101 identity was read as PARTNUM=0x00, VERSION=0x14. This does not identify the RF board vendor.

<!-- section: pins -->
## Pin table

Cross-checked against [include/config.h](../../include/config.h):

| CC1101 module pin | Signal | Wire | ESP32 |
| --- | --- | --- | --- |
| 1 | GND | White | G / GND |
| 2 | VCC | Yellow | 3V3 |
| 3 | GDO0 | Gray | GPIO26 |
| 4 | CSN | Blue | GPIO5 |
| 5 | SCK | Green | GPIO18 |
| 6 | MOSI / SI | Purple | GPIO23 |
| 7 | MISO / SO / CHIP_RDYn | Black | GPIO19 |
| 8 | GDO2 | Orange | GPIO27 |

Pin numbers and wire colors refer only to the recorded module. Other CC1101 boards can place the
same signals on different header positions: identify labels and orientation before wiring.

<!-- section: electrical -->
## Electrical constraints

- Disconnect USB before changing wires. Check pin orientation and common ground first.
- CC1101 uses **3.3 V**. Never connect its VCC or signal inputs directly to 5 V.
- GPIO5 is an ESP32 boot-strapping pin; avoid external circuits that force an incompatible boot level.
- Loose jumper contacts caused SPI failure in the original setup; reseating MISO wasn't enough,
  while replacing a jumper restored stable identity reads. Actual module voltage was not measured.
- Keep the antenna attached for intentional transmission. Do not alter power or frequency based
  only on a nominal module description.
- During diagnostics both GDOs are ESP32 inputs. RX uses GDO0 as a CC1101 data output.
  TX first makes CC1101 GDO0 high impedance, then ESP32/RMT drives that pin. GDO2 remains an input.

The driver uses VSPI, mode 0, MSB first at 1 MHz. It waits for SO/CHIP_RDYn with a 10 ms timeout.
Status-register reads set READ|BURST separately: PARTNUM 0xF0, VERSION 0xF1, MARCSTATE 0xF5.
The burst bit distinguishes status reads from command strobes; this is not a consecutive register burst.

<!-- section: radio -->
## Radio requirements

433 MHz allocations, allowed power, duty cycle and permitted use differ by region. Determine the
rules for your location before transmitting. The project does not establish regulatory compliance.
The configured PA value corresponds to TI's typical -10 dBm entry at 433 MHz, not measured antenna
output or EIRP. Use only devices you own or are authorized to test.

References: [TI CC1101 datasheet](https://www.ti.com/lit/ds/symlink/cc1101.pdf),
[PlatformIO esp32dev](https://docs.platformio.org/en/stable/boards/espressif32/esp32dev.html),
[Waveshare board documentation](https://docs.waveshare.net/ESP32-DEV-KIT-XX/).
