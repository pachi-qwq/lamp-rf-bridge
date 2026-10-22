# SPDX-License-Identifier: MIT
"""Check read-only and rejection paths on a real ESP32; never send valid RF commands.
对真实 ESP32 HTTP 执行只读及拒绝路径检查；不发送合法射频命令。
"""
import argparse
import base64
import hashlib
import re
from datetime import datetime
import json
from pathlib import Path
import requests
from requests.auth import HTTPDigestAuth
from urllib.parse import urlparse

ROOT = Path(__file__).resolve().parent.parent

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--url", required=True)
    parser.add_argument("--access-file", type=Path, required=True, help="build/private/ 下的串口检查 JSON")
    args = parser.parse_args()
    target = urlparse(args.url)
    if target.scheme != "http" or not target.hostname or target.path not in ("", "/"):
        parser.error("请提供设备本地 http 地址")
    access = json.loads(args.access_file.read_text(encoding="utf-8"))["access"]
    base = args.url.rstrip("/")
    session = requests.Session()
    session.trust_env = False  # Do not send local-device authentication through system proxies / 不把本地设备认证交给系统代理
    session.auth = HTTPDigestAuth(access["username"], access["access_password"])
    checks = []
    def check(name, passed):
        checks.append({"name": name, "passed": bool(passed)})
        if not passed: raise RuntimeError("检查失败：" + name)

    no_auth = requests.Session(); no_auth.trust_env = False
    check("unauthenticated_status_401", no_auth.get(base + "/api/status", timeout=5).status_code == 401)
    browser = no_auth.get(base + "/api/status", headers={"X-Lamp-Web":"1"}, timeout=5)
    check("browser_expiry_uses_form_not_native_dialog", browser.status_code == 401 and "WWW-Authenticate" not in browser.headers)
    shell = no_auth.get(base + "/", timeout=5)
    check("public_shell_does_not_grant_api_access", shell.status_code == 200 and 'autocomplete="current-password"' in shell.text)
    probe = no_auth.get(base + "/api/session", timeout=5)
    check("session_probe_does_not_challenge_browser", probe.status_code == 200 and not probe.json()["authenticated"] and "WWW-Authenticate" not in probe.headers)
    status = session.get(base + "/api/status", timeout=5)
    status.raise_for_status()
    before = status.json()
    check("real_radio_idle", before["phase"] == 4 and before["rf"]["ready"] and before["rf"]["marcstate"] == 1)
    for path, expected in (("/", "text/html"), ("/style.css", "text/css"), ("/app.js", "application/javascript")):
        result = session.get(base + path, timeout=5)
        check("asset_" + path, result.status_code == 200 and expected in result.headers.get("Content-Type", "") and len(result.content) > 100)
        check("security_headers_" + path, result.headers.get("X-Content-Type-Options") == "nosniff" and "frame-ancestors 'none'" in result.headers.get("Content-Security-Policy", ""))
    page = session.get(base + "/", timeout=5)
    csp = page.headers.get("Content-Security-Policy", "")
    styles = re.findall(r"<style>([\s\S]*?)</style>", page.text)
    scripts = re.findall(r"<script>([\s\S]*?)</script>", page.text)
    check("single_response_css_js", len(styles) == 1 and len(scripts) == 1 and
          '<link rel="stylesheet"' not in page.text and '<script defer src=' not in page.text)
    for kind, content in (("style", styles[0]), ("script", scripts[0])):
        digest = base64.b64encode(hashlib.sha256(content.encode("utf-8")).digest()).decode("ascii")
        directive = next(part for part in csp.split(";") if part.strip().startswith(kind + "-src"))
        check("inline_" + kind + "_hash_matches_csp", "'sha256-" + digest + "'" in directive)
    check("csp_no_unsafe_inline", "'unsafe-inline'" not in csp)
    check("icon_only_power_keeps_accessible_name", 'aria-label="切换开关"' in page.text and 'class="power-label"' not in page.text)
    check("iphone_launch_metadata", 'rel="apple-touch-startup-image"' in page.text and 'device-width: 402px' in page.text)
    for path, expected in (("/manifest.webmanifest", "application/manifest+json"), ("/icon.svg", "image/svg+xml"), ("/apple-touch-icon.png", "image/png"), ("/launch-1206x2622-v1.png", "image/png")):
        public = no_auth.get(base + path, timeout=5)
        check("public_install_asset_" + path, public.status_code == 200 and expected in public.headers.get("Content-Type", ""))
        if path.endswith("webmanifest"):
            manifest = public.json()
            check("standalone_start_url", manifest["display"] == "standalone" and manifest["start_url"] == "/")
            check("manifest_allowed_by_csp", "manifest-src 'self'" in public.headers.get("Content-Security-Policy", ""))
        elif path.endswith(".svg"):
            check("scalable_primary_icon", b"<svg" in public.content)
        else:
            import struct
            dimensions = (1206, 2622) if "launch-" in path else (180, 180)
            check("ios_png_dimensions_" + path, public.content.startswith(b"\x89PNG") and struct.unpack(">II", public.content[16:24]) == dimensions)
            if "launch-" in path:
                check("launch_image_cached", "max-age=31536000" in public.headers.get("Cache-Control", ""))
    payload = {"button": "unknown", "request_id": "0123456789abcdef01234567"}
    result = session.post(base + "/api/command", data=payload, timeout=5)
    check("csrf_required", result.status_code == 403)
    headers = {"X-Lamp-CSRF": before["csrf"]}
    result = session.post(base + "/api/command", data=payload, headers=headers, timeout=5)
    check("unknown_button_rejected", result.status_code == 400)
    result = session.post(base + "/api/command", data={"button":"power","request_id":"bad"}, headers=headers, timeout=5)
    check("malformed_request_id_rejected", result.status_code == 400)
    result = session.post(base + "/api/command", data=payload, headers={**headers,"Origin":"http://untrusted.invalid"}, timeout=5)
    check("cross_origin_rejected", result.status_code == 403)
    result = session.post(base + "/api/command", json=payload, headers=headers, timeout=5)
    check("unsupported_format_rejected", result.status_code == 415)
    result = session.get(base + "/api/command", timeout=5)
    check("get_cannot_transmit", result.status_code == 404)
    if not before["network"]["can_configure"]:
        result = session.post(base + "/api/wifi", data={"ssid":"", "password":""}, headers=headers, timeout=5)
        check("wifi_setup_not_exposed_on_lan", result.status_code == 403)
    after = session.get(base + "/api/status", timeout=5).json()
    check("negative_checks_did_not_transmit", before["attempts"] == after["attempts"])
    summary = {"url": base, "checks": checks, "before_attempts":before["attempts"], "after_attempts":after["attempts"],
               "rf":after["rf"],"network_connected":after["network"]["connected"]}
    path = ROOT / "docs/evidence" / (datetime.now().strftime("%Y%m%d_%H%M%S") + "_http-checks.json")
    path.write_text(json.dumps(summary,ensure_ascii=False,indent=2) + "\n",encoding="utf-8")
    print(f"PASS: {len(checks)} real HTTP checks; no RF commands sent -> {path}")

if __name__ == "__main__":
    main()
