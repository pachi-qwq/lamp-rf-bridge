# SPDX-License-Identifier: MIT
"""Collect real Phase 2 pulses as CSV, metadata and original serial bytes.
从真实 Phase 2 串口采集原始脉冲，保存 CSV、元数据及原始串口字节。
"""
import argparse
import csv
from datetime import datetime
import json
from pathlib import Path
import re
import sys
import time

import serial


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", required=True)
    parser.add_argument("--label", required=True)
    parser.add_argument("--seconds", type=float, default=2)
    parser.add_argument("--wait", type=float, default=30)
    parser.add_argument("--trigger", type=float, default=-70, help="RSSI 触发门限 dBm（典型校准）")
    parser.add_argument("--immediate", action="store_true")
    parser.add_argument("--frequency-khz", type=int, default=433920)
    parser.add_argument("--profile", choices=["default", "wide", "narrow", "fast"], default="default")
    parser.add_argument("--output-dir", type=Path, default=Path("captures"))
    args = parser.parse_args()
    if not re.fullmatch(r"[A-Za-z0-9_-]{1,48}", args.label):
        parser.error("--label 仅允许 1..48 位字母、数字、下划线及连字符")
    if not 0.1 <= args.seconds <= 10 or not 0 <= args.wait <= 60 or not -120 <= args.trigger <= 0:
        parser.error("seconds 需为 0.1..10，wait 为 0..60，trigger 为 -120..0")
    if not 430000 <= args.frequency_khz <= 437000:
        parser.error("本阶段接收探测限制在 430000..437000 kHz")
    args.output_dir.mkdir(parents=True, exist_ok=True)
    stamp = datetime.now().astimezone().strftime("%Y%m%d_%H%M%S_%f")
    base = args.output_dir / f"{stamp}_{args.label}"
    connection = serial.Serial(port=None, baudrate=115200, timeout=0.25)
    connection.dtr = connection.rts = False
    connection.port = args.port
    connection.open()
    words = []
    header = None
    armed = None
    with base.with_suffix(".serial.log").open("xb") as raw_log:
        def receive(deadline):
            while time.monotonic() < deadline:
                raw = connection.readline(32768)
                if not raw:
                    continue
                raw_log.write(raw)
                raw_log.flush()
                line = raw.decode("utf-8", errors="replace").strip()
                if not line.startswith("{"):
                    continue
                value = json.loads(line)
                if value.get("type") == "error":
                    raise RuntimeError(f"固件报告错误：{value}")
                return value
            raise TimeoutError("等待串口响应超时；原始字节已保存")

        try:
            connection.reset_input_buffer()
            connection.write(f"tune {args.frequency_khz} {args.profile}\n".encode("ascii"))
            deadline = time.monotonic() + 8
            settings = receive(deadline)
            if settings.get("type") != "status" or not settings.get("ready") or settings.get("marcstate") != 13:
                raise RuntimeError(f"接收初始化未通过：{settings}")
            print(json.dumps(settings, ensure_ascii=False), flush=True)
            if args.immediate:
                command = f"capture {round(args.seconds * 1000)}\n"
            else:
                command = f"arm {round(args.seconds * 1000)} {round(args.wait * 1000)} {round(args.trigger * 2)}\n"
            connection.write(command.encode("ascii"))
            deadline = time.monotonic() + args.wait + args.seconds + 45
            while True:
                message = receive(deadline)
                kind = message.get("type")
                if kind == "armed":
                    armed = message
                    print("ARMED " + json.dumps(message, ensure_ascii=False), flush=True)
                elif kind == "capture_begin":
                    if header is not None:
                        raise ValueError("重复 capture_begin")
                    header = message
                elif kind == "pulses":
                    if header is None or message.get("offset") != len(words):
                        raise ValueError("脉冲块偏移不连续；不能将丢失数据当作成功采集")
                    values = message.get("items")
                    if not isinstance(values, list) or any(type(x) is not int or not 0 <= x <= 0xFFFFFFFF for x in values):
                        raise ValueError("脉冲编码无效")
                    words.extend(values)
                elif kind == "capture_end":
                    if header is None or message.get("count") != len(words) or header.get("count") != len(words):
                        raise ValueError("原始脉冲数量不一致")
                    break
                else:
                    raise ValueError(f"捕获期间出现未预期消息：{message}")
        finally:
            connection.close()

    metadata = {
        "label": args.label,
        "captured_at": datetime.now().astimezone().isoformat(timespec="seconds"),
        "port": args.port,
        "settings": settings,
        "armed": armed,
        "capture": header,
        "boundary_segments_may_be_partial": True,
        "remote_attribution_confirmed": False,
    }
    with base.with_suffix(".json").open("x", encoding="utf-8") as output:
        json.dump(metadata, output, ensure_ascii=False, indent=2)
        output.write("\n")
    with base.with_suffix(".csv").open("x", encoding="utf-8", newline="") as output:
        writer = csv.writer(output)
        writer.writerow(["index", "level", "duration_us", "segment"])
        for i, value in enumerate(words):
            writer.writerow([i, value & 1, value >> 1, "pre" if i < header["pre_count"] else "post"])
    print(json.dumps({
        "csv": str(base.with_suffix(".csv")),
        "metadata": str(base.with_suffix(".json")),
        "pulse_count": len(words),
        "reason": header["reason"],
        "triggered": header["triggered"],
        "rssi_peak_dbm": header["rssi_peak_dbm_x2"] / 2,
        "overflow": header["overflow"],
        "ambiguous_edges": header["ambiguous_edges"],
        "zero_durations": header["zero_durations"],
    }, ensure_ascii=False), flush=True)
    # Retain evidence; return nonzero for truncation, SPI errors or a missing trigger.
    # 全部证据保留；截断、SPI 错误与未触发均用非零退出码明确区分。
    return 0 if words and header["triggered"] and not header["overflow"] and header["reason"] == "duration" and header["idle_error"] == 0 else 2


if __name__ == "__main__":
    sys.exit(main())
