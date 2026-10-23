# User guide

[简体中文](../zh-CN/user-guide.md) · [Documentation](index.md)

Use the web firmware after [setup](getting-started.md). The current page labels and device
messages are in Chinese; bilingual documentation does not add a second UI language.

<!-- section: network -->
## Reach the gateway

Connect your phone to the gateway's LAN. The ESP32 requires 2.4 GHz Wi-Fi, but the phone can use
the same router's 5 GHz network if both networks can communicate. Guest/client isolation may block
access. Cellular data alone cannot reach this local service. A VPN back into your home network is
a possible separate deployment, not a built-in remote-control feature.

Use the address printed by the device, or try its `lamp-xxxxxx.local` name if your network supports
mDNS. Name resolution has not been separately verified on every platform. IP changes require
updating the saved URL; a DHCP reservation avoids that change.

<!-- section: login -->
## Log in and remember a device

The username is `admin`; the random 16-character access password comes from physical USB
`access` output and is also the setup-hotspot password. It isn't a universal factory password.
The standard form uses username/current-password autocomplete, verified with iPhone password fill.
Open `/login` to display the form even when already authenticated.

“信任此设备” means **Trust this device**. A remembered credential survives ESP32 reset; the device
stores only its SHA-256 hash, with four persistent slots. Logging in again replaces that browser's
old credential. The browser Cookie is HttpOnly, SameSite=Strict and host-only, with a one-year
browser Max-Age. The device has no reliable wall clock enforcing a one-year server expiry.

Without trust, the eight-slot RAM session has an eight-hour lifetime and is lost on ESP32 reset.
“已信任 · 取消” revokes persistent trust and issues a new RAM session, so the current window remains
logged in. It is not “log out everywhere.” Different addresses, browsers, private windows or cleared
site data may need separate login.

<!-- section: controls -->
## Send a command

| Page label | Meaning | Behavior |
| --- | --- | --- |
| Power icon | Power toggle | One toggle, not absolute ON/OFF |
| 调亮 / 调暗 | Brighter / dimmer | One relative step |
| 偏冷 / 偏暖 | Cooler / warmer | One relative step |
| 夜灯 | Night mode | Captured preset command |
| 10 分钟 | 10-minute timer | Captured command; effect and expiry untested |

All seven actions use one request at a time, with no automatic retries or long-press repeat.
“已发送” means the gateway finished the waveform and checked IDLE. It doesn't prove that the lamp
received it. After a timeout, observe the lamp before pressing power again.

The lamp artwork animates an **operation illustration**, not live state. A remote, power outage or
missed frame can make it disagree with the lamp. It resets to neutral when returning to the page.
The status badge describes the gateway/radio only. Do not infer an actual brightness or color
temperature from it.

<!-- section: homescreen -->
## Add an iPhone Home Screen entry

Open the gateway in Safari, choose Share → Add to Home Screen, and leave “Open as Web App” enabled
if offered. Log in within that entry and choose trust. Android can use its browser's home-screen
shortcut as available; Android installation behavior was not separately audited.

The app uses an SVG primary icon and an iOS PNG fallback. The fixed-height control page disables
scrolling; login and provisioning can scroll for the keyboard. A matching 1206×2622 dark launch
image is bundled for the tested iPhone 17 Pro display mode.

This is an online HTTP LAN app with a standalone manifest. There is no Service Worker or offline
command cache, and Vue is not required. A dark startup logo does not indicate connection or lamp
state. The owner reported Home Screen startup under one second after the server idle-preconnection
fix; this is one device's observation, not a performance guarantee.

<!-- section: maintenance -->
## Recover after disconnection or a firmware update

The firmware retries Wi-Fi every 15 seconds. After 60 seconds disconnected with saved credentials,
it opens the password-protected setup AP; a successful LAN connection closes that AP after three
minutes. No command is transmitted by these network events.

After updating web assets, close and reopen the Home Screen app to fetch the new page. Ordinary
firmware upload keeps NVS settings; erasing Flash does not. An RF fault latches the controls
unavailable: diagnose wiring/power before resetting. See [Troubleshooting](troubleshooting.md).
