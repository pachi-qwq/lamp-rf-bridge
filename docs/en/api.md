# HTTP API reference

[简体中文](../zh-CN/api.md) · [Documentation](index.md)

The web image listens on port 80. The shared [OpenAPI description](../api.openapi.json) records
request/response shapes; this page explains operation. No endpoint accepts arbitrary RF data.
The API controls the gateway, not a verified lamp-state model.

<!-- section: auth -->
## Authentication and common checks

Protected routes accept a valid `lamp_device` Cookie (RAM session or persistent trust), or HTTP
Digest with username `admin` and the device's USB-retrieved password. Static `/`, `/login`,
`/style.css`, `/app.js`, manifest and images contain no control credentials. The first four
also require the canonical device Host; metadata/images are public.

All API requests require the canonical LAN IP, active AP IP or device .local Host, optionally
with :80. Mutations require `application/x-www-form-urlencoded`, the current `X-Lamp-CSRF`,
and an exact same-origin match if Origin is present. There is no CORS permission. Form login uses
its challenge-specific CSRF instead of the authenticated boot CSRF.

A browser request with `X-Lamp-Web: 1` receives JSON 401 without a native auth popup; diagnostic
Digest clients receive WWW-Authenticate. Passwords are never returned through a public endpoint.

<!-- section: routes -->
## Endpoints

| Method and path | Access / input | Result |
| --- | --- | --- |
| GET /api/session | Public probe; optional auth | 200 authenticated=false, or full authenticated status |
| GET /api/login/challenge | Public, valid Host | realm, nonce, opaque, csrf |
| POST /api/session/login | One-use challenge Digest + CSRF; remember=0/1 | Cookie, ok, trusted_device |
| GET /api/status | Auth | Gateway/radio/network/CSRF status |
| POST /api/command | Auth + boot CSRF; button, request_id | Send outcome, timing and false lamp_state_confirmed |
| POST /api/wifi | Auth + boot CSRF; ssid, password; AP interface only | Save credentials and begin connection |
| POST /api/session/trust | Auth + boot CSRF; empty body | Persistent trust Cookie |
| POST /api/session/forget | Auth + boot CSRF; empty body | Revoke trust and issue RAM Cookie |

Unknown authenticated routes/methods return 404, not a guaranteed 405.
Unauthenticated unknown routes first require authentication.

<!-- section: login -->
## Form-login exchange

1. GET /api/login/challenge obtains a signed nonce, opaque, realm `Lamp Bridge`, and independent CSRF.
2. The real [auth.js](../../web/auth.js) computes a standard Digest proof for
   **POST /api/session/login**, qop=auth, nc=00000001 and a random cnonce.
3. POST that proof in Authorization, challenge CSRF in X-Lamp-CSRF and only `remember=0` or `1`
   in the body. Raw password/HA1 is not sent by this form.
4. Confirm Cookie access with GET /api/session before navigating to the control page.

Four outstanding challenges rotate in RAM, expire after two minutes and are consumed before
password validation, including wrong-password attempts. Login success returns a new HttpOnly,
SameSite=Strict, Path=/ Cookie. Remembered tokens use one-year browser Max-Age; RAM tokens do not.
The device hashes tokens. See [User guide](user-guide.md) for lifetime/revocation semantics.

Signed Digest nonces expire after an hour and are bound to this boot and the 32-bit clock epoch;
a wrap boundary can invalidate an otherwise young nonce early, requiring a fresh challenge.
The 64-bit RAM session lifetime prevents expired tokens reviving after millis() wraps.
These clock changes have local boundary tests; fresh real-device auth regression is still required.

<!-- section: status -->
## Status fields

Authenticated status has no top-level `ok`; it includes:

| Field | Meaning |
| --- | --- |
| authenticated / phase / csrf | true / 4 / boot CSRF for mutations |
| auth.trusted_device | This request carries a valid persistent trust Cookie |
| http.idle_preconnect_closed | Boot-local idle-connection guard count |
| rf.ready / marcstate / bus_error | Gateway radio readiness, raw state and numeric BusError |
| network.connected / configured / ap_active | Network lifecycle flags |
| network.can_configure | Request arrived on the active AP interface |
| network.ip / ssid | Connected LAN values; empty when disconnected |
| attempts / last_button | Boot-local actual TX attempts and last attempted command |
| cooldown_ms / uptime_s | Remaining cooldown and unsigned millis()/1000 uptime |

Neither radio state nor request success is lamp-state feedback. Do not publish status responses
without removing private SSIDs/IPs. `BusError` values in declaration order are None=0,
ReadyTimeout=1, ResetTimeout=2, NotInitialized=3, RegisterMismatch=4, StateTimeout=5,
InvalidSettings=6, ChipNotReady=7.

<!-- section: command -->
## Command input and output

The body must contain exactly `button` and `request_id`. Names:
`power`, `brightness_up`, `brightness_down`, `cct_up`, `cct_down`,
`night_mode`, `timer_10min`. IDs are 16..48 lowercase hexadecimal characters.

A successful command response includes `ok`, Chinese `message`, `button`, `attempt`,
`cooldown_ms`, `radio_elapsed_us`, `frames` and `lamp_state_confirmed:false`.
Eight recent IDs cache the original status/body, including failed attempts. Reusing an ID with
another button returns 409. Cache eviction or reboot loses deduplication; do not blindly retry
a power toggle after a timeout.

Read-only PowerShell example; curl prompts for the access password:

```powershell
$Base = Read-Host 'Device URL, without trailing slash'
curl.exe --digest --user admin "$Base/api/status"
```

The following **intentionally transmits one brightness-up command**. Run only against your
authorized compatible lamp; no output in this guide is fabricated as a hardware result.

```powershell
$Status = curl.exe --silent --show-error --digest --user admin "$Base/api/status" | ConvertFrom-Json
$Id = [guid]::NewGuid().ToString('N')
curl.exe --digest --user admin "$Base/api/command" --header "X-Lamp-CSRF: $($Status.csrf)" --data-urlencode 'button=brightness_up' --data-urlencode "request_id=$Id"
```

<!-- section: errors -->
## Validation and errors

| HTTP status | Conditions |
| --- | --- |
| 400 | Unknown/extra/malformed button or request ID, invalid Wi-Fi byte lengths, extra trust parameters |
| 401 | Protected API without accepted authentication |
| 403 | Host/Origin/CSRF mismatch, invalid/expired login proof, or Wi-Fi configuration from LAN |
| 404 | Authenticated route/method not registered |
| 409 | Cached request ID reused for another button |
| 415 | Protected mutation content type unsupported |
| 429 | Command attempted during cooldown |
| 500 | NVS/session persistence failure |
| 503 | RF not ready/failed, or authentication signing failure |

Errors normally return `{"ok":false,"message":"..."}`; command outcomes also carry the command
fields above. The client must inspect status/ok, and must never automatically replay a command.
SSID allows 1..32 bytes; password allows empty or 8..63 bytes; control characters are rejected.
Wi-Fi save writes one complete NVS record and reconnects. Only the AP interface may call it.
