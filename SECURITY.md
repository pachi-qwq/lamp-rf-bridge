# Security policy

[简体中文](SECURITY.zh-CN.md)

This is an experimental LAN RF gateway, not a safety-critical controller. Do not use it for locks,
alarms, medical equipment or any device where an unintended/replayed command could cause harm.

## Reporting

Use the repository's **Security → Advisories → Report a vulnerability** private reporting form
once the owner has enabled GitHub Private Vulnerability Reporting. No public security email has
been designated. The owner must enable this before inviting private reports; this policy
does not indicate that the setting is enabled.

If the private form is unavailable, open a minimal issue asking for a private contact channel.
Do not include exploit details, credentials or personal data in that issue. Include the affected
revision, threat model, reproduction and impact only in the private report. There is no promised
response SLA or bug-bounty program.

## Support scope

Before the first release, security fixes target the current development branch only. There are no
supported historical release lines or long-term-support commitments. A local check or CI build is
not a security certification.

## Network and credential boundaries

- Use a trusted LAN or the password-protected provisioning AP. Do not expose port 80 to the
  internet, use router port forwarding, or assume an arbitrary VPN makes HTTP private.
- HTTP has no TLS. Digest proofs avoid sending the raw login password but do not encrypt traffic;
  a network observer may obtain replayable credentials/proofs or attack a password offline.
  Wi-Fi provisioning itself sends credentials over this HTTP link.
- Host, Origin (when present), CSRF, command whitelists and HttpOnly/SameSite Cookies reduce
  accidental/cross-site access; they do not protect an already compromised LAN or client.
- Access/AP passwords and home Wi-Fi credentials remain in NVS without project-enabled Flash
  encryption. Physical USB access can print access credentials. Trust stores hashes, but a stolen
  browser token is a bearer credential. Do not publish serial logs or private test files.
- Persistent trust has four slots and explicit per-device revocation. Browser Max-Age is one year,
  not a reliable server-side wall-clock expiry. Unremembered sessions have eight-hour monotonic
  expiry and reset with the device. There is no global logout/password-rotation endpoint.
- Login challenges are one-use, two-minute, four-slot entries. There is no dedicated
  password-attempt rate limiter or comprehensive denial-of-service protection.

## RF boundaries

Only fixed captured commands are available over HTTP/serial. There is no arbitrary RF transmit
API or automatic retry. RF has no verified encryption, rolling-code protection or lamp ACK.
Nearby transmitters may replay the same commands independently of HTTP authentication.

Capture/replay only your own devices or devices you have permission to test. Laws for frequency,
power, duty cycle, privacy and interference vary by jurisdiction. Do not publish captures containing
other people's identifiers or access-control codes. Read [RF limits](docs/en/rf-protocol.md).

## Disclosure and remediation

Coordinate a fix and an appropriate disclosure date privately. Maintainers should describe
affected versions, mitigation and actual verification, and avoid claiming that unchanged RF
behavior has been revalidated without hardware. See the
[GitHub private reporting documentation](https://docs.github.com/en/code-security/how-tos/report-and-fix-vulnerabilities/configure-vulnerability-reporting/configure-for-a-repository).
