# SPDX-License-Identifier: MIT
"""Regress idle TCP preconnections on a real ESP32 using read-only requests and invalid commands.
在真实 ESP32 上回归空闲 TCP 预连接阻塞；只读请求与非法按键，不发送射频。
"""
import argparse
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime
import json
from pathlib import Path
import socket
import time
from urllib.parse import urlparse
import requests
from requests.auth import HTTPDigestAuth

ROOT = Path(__file__).resolve().parent.parent

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--url", required=True)
    parser.add_argument("--access-file", type=Path, required=True)
    args = parser.parse_args()
    base = args.url.rstrip("/")
    target = urlparse(base)
    if target.scheme != "http" or not target.hostname or target.path not in ("", "/"):
        parser.error("请提供设备本地 http 地址")
    access = json.loads(args.access_file.read_text(encoding="utf-8"))["access"]
    admin = requests.Session(); admin.trust_env = False
    admin.auth = HTTPDigestAuth(access["username"], access["access_password"])
    initial = admin.get(base + "/api/status", timeout=8); initial.raise_for_status()
    before = initial.json()
    checks, measurements = [], []
    def check(name, passed):
        checks.append({"name":name,"passed":bool(passed)})
        if not passed: raise RuntimeError("检查失败：" + name)
    def peer():
        return socket.create_connection((target.hostname, target.port or 80), timeout=3)
    def fetch(path="/api/status", method="GET", **options):
        # Prepare auth headers on the main thread to avoid shared-Session races or an extra 401 masking latency.
        # 认证头在主线程提前生成，避免共享 Session 的线程竞争和额外的 401 掩盖等待。
        auth = admin.auth.build_digest_header(method, base + path)
        client = requests.Session(); client.trust_env = False
        start = time.perf_counter()
        response = client.request(method, base + path, headers={"Authorization":auth, **options.pop("headers", {})}, timeout=8, **options)
        return response, round((time.perf_counter() - start) * 1000, 1)
    for idle_count in (0, 1, 2):
        peers = [peer() for _ in range(idle_count)]
        try:
            if peers: time.sleep(.1)
            response, elapsed = fetch()
            measurements.append({"idle_connections":idle_count,"roundtrip_ms":elapsed,"http_status":response.status_code})
            check(f"status_with_{idle_count}_idle", response.status_code == 200)
            check(f"no_five_second_block_with_{idle_count}_idle", elapsed < (2000 if idle_count == 2 else 1500))
        finally:
            for value in peers: value.close()

    # Delay the first byte by 120 ms with another request queued; the original request must still complete.
    # 连接已建立后 120ms 才发首字节；另一真实请求排队，前者仍应正常完成。
    delayed = peer()
    auth = admin.auth.build_digest_header("GET", base + "/api/status")
    header = ("GET /api/status HTTP/1.1\r\nHost: " + target.netloc +
              "\r\nAuthorization: " + auth + "\r\nConnection: close\r\n\r\n").encode("ascii")
    def delayed_request():
        time.sleep(.12)
        delayed.sendall(header)
        data = b""
        while b"\r\n" not in data:
            part = delayed.recv(1024)
            if not part: break
            data += part
        return data.split(b"\r\n", 1)[0].decode("ascii", errors="replace")
    try:
        with ThreadPoolExecutor(max_workers=1) as pool:
            future = pool.submit(delayed_request)
            time.sleep(.02)
            response, elapsed = fetch()
            first_line = future.result(timeout=8)
        check("delayed_first_byte_still_completes", first_line.startswith("HTTP/1.1 200"))
        check("queued_request_after_delayed_client", response.status_code == 200 and elapsed < 1500)
        measurements.append({"scenario":"120ms delayed first byte with queued request","queued_roundtrip_ms":elapsed,"first_status":first_line})
    finally:
        delayed.close()

    idle = peer()
    try:
        time.sleep(.1)
        response, elapsed = fetch("/api/command", "POST",
            data={"button":"unknown","request_id":"0123456789abcdef01234567"},
            headers={"X-Lamp-CSRF":before["csrf"]})
        check("unknown_button_still_rejected_under_contention", response.status_code == 400 and elapsed < 1500)
    finally:
        idle.close()
    final = admin.get(base + "/api/status", timeout=8).json()
    check("idle_preconnect_guard_exercised", final["http"]["idle_preconnect_closed"] >= before["http"]["idle_preconnect_closed"] + 4)
    check("no_rf_transmission", before["attempts"] == final["attempts"])
    result = {"checks":checks,"measurements":measurements,"guard_count_before":before["http"]["idle_preconnect_closed"],
              "guard_count_after":final["http"]["idle_preconnect_closed"],"rf_attempts_unchanged":True,"rf":final["rf"]}
    output = ROOT / "docs/evidence" / (datetime.now().strftime("%Y%m%d_%H%M%S") + "_preconnect-checks.json")
    output.write_text(json.dumps(result,ensure_ascii=False,indent=2) + "\n",encoding="utf-8")
    print(json.dumps(result,ensure_ascii=False))
    print(f"PASS: {len(checks)} real preconnect checks -> {output}")

if __name__ == "__main__":
    main()
