# SPDX-License-Identifier: MIT
"""Verify trust, revocation and reboot persistence on a real ESP32 without valid RF commands.
验证真实 ESP32 的设备信任、撤销及跨重启保留；不发送合法射频命令。
"""
import argparse
from datetime import datetime
import json
from pathlib import Path
import requests
from requests.auth import HTTPDigestAuth

ROOT = Path(__file__).resolve().parent.parent

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--url", required=True)
    parser.add_argument("--access-file", type=Path, required=True)
    parser.add_argument("--resume-file", type=Path, help="重启后的第二次检查，使用 build/private 的临时测试凭证")
    args = parser.parse_args()
    base = args.url.rstrip("/")
    access = json.loads(args.access_file.read_text(encoding="utf-8"))["access"]
    checks = []
    def check(name, passed):
        checks.append({"name":name, "passed":bool(passed)})
        if not passed: raise RuntimeError("检查失败：" + name)
    def client():
        s = requests.Session(); s.trust_env = False
        return s
    def remember():
        s = client()
        s.auth = HTTPDigestAuth(access["username"], access["access_password"])
        before = s.get(base + "/api/status", timeout=5).json()
        response = s.post(base + "/api/session/trust", data="", headers={
            "Content-Type":"application/x-www-form-urlencoded", "X-Lamp-CSRF":before["csrf"]}, timeout=5)
        check("remember_authenticated", response.status_code == 200)
        header = response.headers.get("Set-Cookie", "")
        check("cookie_flags", all(part in header for part in ("HttpOnly","SameSite=Strict","Path=/","Max-Age=31536000")) and "Domain=" not in header)
        cookie = s.cookies.get("lamp_device")
        check("random_cookie_256_bits", cookie is not None and len(cookie) == 64)
        s.auth = None
        return s, cookie, before
    def revoke(s, csrf):
        return s.post(base + "/api/session/forget", data="", headers={
            "Content-Type":"application/x-www-form-urlencoded","X-Lamp-CSRF":csrf}, timeout=5)

    if args.resume_file:
        saved = json.loads(args.resume_file.read_text(encoding="utf-8"))
        a = client()
        a.cookies.set("lamp_device", saved["cookie"], domain=saved["host"], path="/")
        response = a.get(base + "/api/status", timeout=5)
        check("cookie_survives_real_esp32_reboot", response.status_code == 200 and response.json()["auth"]["trusted_device"])
        state = response.json()
        before_attempts = state["attempts"]
        cookie = saved["cookie"]
        check("revoke_after_reboot", revoke(a, state["csrf"]).status_code == 200)
        replay = client()
        check("revoked_cookie_replay_denied", replay.get(base + "/api/status", headers={"Cookie":"lamp_device=" + cookie}, timeout=5).status_code == 401)
    else:
        unauth = client()
        check("unauth_cannot_enroll", unauth.post(base + "/api/session/trust", data="", timeout=5).status_code == 401)
        a, cookie, initial = remember()
        before_attempts = initial["attempts"]
        response = a.get(base + "/api/status", timeout=5)
        check("cookie_only_no_digest", response.status_code == 200 and response.json()["auth"]["trusted_device"] and "Authorization" not in response.request.headers)
        state = response.json()
        csrf = state["csrf"]
        response = a.get(base + "/", timeout=5)
        check("cookie_opens_control_page", response.status_code == 200 and "text/html" in response.headers.get("Content-Type",""))
        forged = ("b" if cookie[0] != "b" else "a") + cookie[1:]
        check("forged_cookie_rejected", unauth.get(base + "/api/status", headers={"Cookie":"lamp_device=" + forged}, timeout=5).status_code == 401)
        check("duplicate_cookie_rejected", unauth.get(base + "/api/status", headers={"Cookie":"lamp_device=" + cookie + "; lamp_device=" + cookie}, timeout=5).status_code == 401)
        check("cookie_still_needs_csrf", a.post(base + "/api/session/forget", data="", timeout=5).status_code == 403)
        h = {"Content-Type":"application/x-www-form-urlencoded","X-Lamp-CSRF":csrf}
        check("cookie_cross_origin_blocked", a.post(base + "/api/session/forget", data="", headers={**h,"Origin":"http://other.invalid"}, timeout=5).status_code == 403)
        check("remember_get_rejected", a.get(base + "/api/session/trust", timeout=5).status_code == 404)
        response = a.post(base + "/api/session/trust", data="", headers=h, timeout=5)
        check("repeat_trust_is_idempotent", response.status_code == 200 and "Set-Cookie" not in response.headers)
        b, token_b, _ = remember()
        check("distinct_device_tokens", token_b != cookie)
        check("second_device_cookie_only", b.get(base + "/api/status", timeout=5).status_code == 200)
        check("revoke_second_device", revoke(b, csrf).status_code == 200)
        check("second_revoked_cookie_denied", unauth.get(base + "/api/status", headers={"Cookie":"lamp_device=" + token_b}, timeout=5).status_code == 401)
        check("first_device_unaffected", a.get(base + "/api/status", timeout=5).status_code == 200)
        # Keep reboot-test tokens only in Git-ignored storage; public evidence contains no credentials.
        # 只保存到 Git 忽略目录，供实体 ESP32 重启后验证；公开证据不含凭证。
        from urllib.parse import urlparse
        private = ROOT / "build/private/trust-reboot-test.json"
        private.write_text(json.dumps({"cookie":cookie,"host":urlparse(base).hostname}) + "\n",encoding="utf-8")
    admin = client()
    admin.auth = HTTPDigestAuth(access["username"], access["access_password"])
    after = admin.get(base + "/api/status", timeout=5).json()
    check("no_rf_commands_sent", before_attempts == after["attempts"])
    suffix = "trust-reboot-checks" if args.resume_file else "trust-checks"
    output = ROOT / "docs/evidence" / (datetime.now().strftime("%Y%m%d_%H%M%S") + "_" + suffix + ".json")
    output.write_text(json.dumps({"checks":checks,"before_attempts":before_attempts,"after_attempts":after["attempts"],
        "rf":after["rf"]}, ensure_ascii=False, indent=2) + "\n",encoding="utf-8")
    print(f"PASS: {len(checks)} real trust checks; no RF commands sent -> {output}")

if __name__ == "__main__":
    main()
