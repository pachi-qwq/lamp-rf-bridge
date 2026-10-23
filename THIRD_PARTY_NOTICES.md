# Third-party notices

[简体中文](THIRD_PARTY_NOTICES.zh-CN.md)

The root MIT License applies to project-owned material. It does not replace these licenses.

## Bundled material

| Component | Version / location | License and attribution |
| --- | --- | --- |
| blueimp-md5 | 2.19.0, web/vendor | MIT, Sebastian Tschan; retain [original license](web/vendor/blueimp-md5-LICENSE.txt) |
| Contributor Covenant text | 2.1, CODE_OF_CONDUCT files | CC BY 4.0, Contributor Covenant contributors; English adapted for contact method, Chinese reference translation |
| Original icons / launch image | web/icon.svg and generated PNGs | Project-owned MIT artwork; no external icon pack or stock image |

The MD5 script is vendored unmodified. Its npm tarball SHA-512 integrity and upstream are recorded in
[blueimp-md5-source.json](web/vendor/blueimp-md5-source.json); the full MIT notice is also embedded in
the shipped page. Vendor comments remain in their original language.

The Covenant is sourced from the upstream
[pinned CC BY 4.0 revision](https://github.com/EthicalSource/contributor_covenant/tree/91532d80ea5f39b6685aff4892bc6854bf2f6d6f),
not the redesigned website's current source-code license. The
[original CC BY 4.0 text](LICENSES/CC-BY-4.0.txt) and attribution are retained.
The contact method and language-switch links are project adaptations. Community policy text is
not re-licensed as MIT.

## Firmware/build dependencies

| Dependency | Pinned version | Relevant license |
| --- | --- | --- |
| Arduino-ESP32 | 2.0.17 / PlatformIO package 3.20017.241212 | LGPL-2.1-or-later for core/libraries; individual bundled components can differ |
| ESP-IDF SDK within Arduino package | 4.4 series bundled by that package | Apache-2.0 for ESP-IDF source; component and prebuilt radio-library terms can differ |
| Mbed TLS SHA-256/HMAC headers | Bundled SDK | Apache-2.0 OR GPL-2.0-or-later; Apache option used here |
| PlatformIO Core / espressif32 platform | 6.1.19 / 6.12.0 | Apache-2.0 |
| Python requests / pyserial | requirements-lock.txt | Apache-2.0 / BSD-3-Clause |
| Toolchain, esptool and other transitive tools | platformio.ini / lock file | Their own package licenses; not project MIT |

[Arduino's original license](LICENSES/LGPL-2.1.txt) and the
[Apache 2.0 text](LICENSES/Apache-2.0.txt) are included for reference.
Sources: [Arduino-ESP32 2.0.17](https://github.com/espressif/arduino-esp32/tree/2.0.17),
[ESP-IDF](https://github.com/espressif/esp-idf/tree/v4.4.7) and local package notices.
This is a notice of the used components, not a claim that every SDK binary or transitive build
tool is MIT/Apache-licensed.

## Distribution boundary

The repository contains application source and original RF samples, not copied TI documentation,
a vendored ESP32 SDK or prebuilt firmware. External datasheets are linked only.
Before distributing a firmware binary, retain applicable notices and provide corresponding library
source and materials needed for relinking as required by the LGPL, plus any SDK binary obligations.
Do not describe a complete firmware image as “MIT-only.” CI currently builds without publishing
binary artifacts. The full binary-distribution compliance package has not been assembled.

Development npm metadata was checked for 80 installed packages: 76 MIT, two ISC, one BSD-2-Clause
and one Python-2.0. These are check tools, not shipped web dependencies. Python check-tool
transitive versions are pinned in requirements-dev.txt.
