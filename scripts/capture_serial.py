# SPDX-License-Identifier: MIT
"""Save real USB serial output without injecting data or simulating a device.
保存真实 USB 串口输出；不注入数据，不模拟设备。
"""
import argparse
from datetime import datetime
from pathlib import Path
import time

import serial


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", required=True)
    parser.add_argument("--seconds", type=float, default=25)
    parser.add_argument("--log", type=Path, default=Path("build/logs/serial.log"))
    args = parser.parse_args()
    if not 0 < args.seconds <= 300:
        parser.error("--seconds 必须大于 0 且不超过 300")
    args.log.parent.mkdir(parents=True, exist_ok=True)
    connection = serial.Serial(port=None, baudrate=115200, timeout=0.25)
    # Set control lines before opening; do not toggle EN/BOOT or erase Flash.
    # 打开前预置控制线；不主动切换 EN/BOOT，不擦写 Flash。
    connection.dtr = False
    connection.rts = False
    connection.port = args.port
    connection.open()
    print(f"Capturing {args.port} for {args.seconds:g}s -> {args.log}", flush=True)
    try:
        with args.log.open("wb") as output:
            started = time.monotonic()
            while time.monotonic() - started < args.seconds:
                raw = connection.readline()
                if raw:
                    output.write(raw)
                    output.flush()
                    print(raw.decode("utf-8", errors="replace").rstrip(), flush=True)
    finally:
        connection.close()
    print(f"Capture finished at {datetime.now().astimezone().isoformat(timespec='seconds')}", flush=True)


if __name__ == "__main__":
    main()
