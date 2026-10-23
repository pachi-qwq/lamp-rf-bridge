# SPDX-License-Identifier: MIT
"""Read real Phase 4 serial status; keep credential-bearing records in ignored build/private.
读取真实 Phase 4 串口状态；含访问密码的记录仅写入 Git 忽略的 build/private。
"""
import argparse
from datetime import datetime
import hashlib
import json
from pathlib import Path
import time
import serial

ROOT = Path(__file__).resolve().parent.parent

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", default="COM4")
    parser.add_argument("--reset", action="store_true")
    args = parser.parse_args()
    prefix = ROOT / "build/private" / datetime.now().strftime("%Y%m%d_%H%M%S_phase4")
    prefix.parent.mkdir(parents=True, exist_ok=True)
    result = {"firmware_sha256": hashlib.sha256((ROOT / ".pio/build/phase4/firmware.bin").read_bytes()).hexdigest()}
    port = serial.Serial(port=None, baudrate=115200, timeout=0.1)
    port.dtr = False; port.rts = False; port.port = args.port
    port.open()
    try:
        with prefix.with_suffix(".serial.log").open("xb") as log:
            def read(seconds, expected=None):
                deadline = time.monotonic() + seconds
                while time.monotonic() < deadline:
                    line = port.readline()
                    if not line: continue
                    log.write(line); log.flush()
                    print(line.decode("utf-8", errors="replace").rstrip(), flush=True)
                    try: value = json.loads(line)
                    except (ValueError, UnicodeDecodeError): continue
                    if isinstance(value, dict) and value.get("type") == expected:
                        return value
                if expected: raise RuntimeError("串口响应超时：" + expected)
            if args.reset:
                port.rts = True; time.sleep(0.1); port.rts = False
            read(1.8 if args.reset else 0.3)
            port.write(b"status\n"); port.flush()
            result["status"] = read(3, "status")
            if result["status"].get("phase") != 4 or not result["status"].get("ready") or result["status"].get("marcstate") != 1:
                raise RuntimeError("Phase 4 射频状态未通过")
            port.write(b"access\n"); port.flush()
            result["access"] = read(3, "access")
            result["http_url"] = "http://192.168.4.1/"
            print("First setup URL:", result["http_url"], flush=True)
    finally:
        port.close()
        prefix.with_suffix(".json").write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
        print("Private evidence:", prefix)

if __name__ == "__main__":
    main()
