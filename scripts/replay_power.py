# SPDX-License-Identifier: MIT
"""Inspect real Phase 3 serial status or send one captured command without automatic retries.
通过真实串口检查 Phase 3，或只发送一次已标定按键命令；不会自动重试。
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
    parser.add_argument("--button", choices=("power", "brightness_up", "brightness_down", "cct_up", "cct_down", "night_mode", "timer_10min"), default="power")
    parser.add_argument("--send", action="store_true", help="明确执行一次 --button 指定的命令；默认只查状态")
    parser.add_argument("--reset", action="store_true", help="只检查时允许复位 ESP32 并保存启动日志")
    parser.add_argument("--check-guards", action="store_true", help="测试未知命令拒绝，不包含合法发送命令")
    args = parser.parse_args()
    if args.send and (args.reset or args.check_guards):
        parser.error("--send 不能和复位或保护检查合并")
    now = datetime.now().astimezone()
    prefix = ROOT / "docs/evidence" / (now.strftime("%Y%m%d_%H%M%S_%f") + (("_replay_" + args.button) if args.send else "_phase3_health"))
    prefix.parent.mkdir(parents=True, exist_ok=True)
    records = []
    result = {"started_at": now.isoformat(), "port": args.port, "send_requested": args.send,
              "operator_feedback": None, "button": args.button if args.send else None, "raw_log": prefix.with_suffix(".serial.log").name}
    image = ROOT / ".pio/build/phase3/firmware.bin"
    result["firmware_sha256"] = hashlib.sha256(image.read_bytes()).hexdigest()
    port = serial.Serial(port=None, baudrate=115200, timeout=0.1)
    port.dtr = False
    port.rts = False
    port.port = args.port
    port.open()
    try:
        with prefix.with_suffix(".serial.log").open("xb") as raw:
            def read_for(seconds, stop_type=None):
                until = time.monotonic() + seconds
                while time.monotonic() < until:
                    line = port.readline()
                    if not line:
                        continue
                    raw.write(line)
                    raw.flush()
                    print(line.decode("utf-8", errors="replace").rstrip(), flush=True)
                    try:
                        message = json.loads(line)
                    except (ValueError, UnicodeDecodeError):
                        continue
                    if not isinstance(message, dict):
                        continue
                    records.append(message)
                    if message.get("type") == stop_type:
                        return message
                if stop_type:
                    raise RuntimeError("等待真实串口响应超时：" + stop_type)

            def query(line, response="status"):
                port.write((line + "\n").encode("ascii"))
                port.flush()
                return read_for(3, response)

            if args.reset:
                port.rts = True
                time.sleep(0.1)
                port.rts = False
                read_for(1.5)
            else:
                read_for(0.3)
            before = query("status")
            result["before"] = before
            if not (before.get("phase") == 3 and before.get("ready") is True and
                    before.get("bus_error") == 0 and before.get("marcstate") == 1):
                raise RuntimeError("设备未处于 Phase 3 可发送的 IDLE 状态；未发送命令")
            if args.check_guards:
                attempts = before["attempts"]
                for command in ("send unknown", "send power extra", "send power 99"):
                    response = query(command, "error")
                    if response.get("message") != "unknown_command":
                        raise RuntimeError("未知命令未被拒绝")
                states = [query("status") for _ in range(10)]
                if not all(s["attempts"] == attempts and s["marcstate"] == 1 and
                           s["ready"] and s["bus_error"] == 0 for s in states):
                    raise RuntimeError("拒绝检查后状态或发送计数异常")
                result["guard_checks"] = {"rejected_commands": 3, "idle_status_samples": 10,
                                          "attempts_unchanged": True}
            if args.send:
                if not before.get("cooldown_ready"):
                    raise RuntimeError("设备仍在冷却，本次未发送")
                # Write a valid command once; retain timeout/error evidence without retrying.
                # 只写一次合法命令；超时或错误仅保留证据，绝不重发。
                response = query("send " + args.button, "tx_result")
                result["tx_result"] = response
                after = query("status")
                result["after"] = after
                if not (response.get("button") == args.button and response.get("waveform_completed") is True and
                        response.get("final_marcstate") == 1 and
                        response.get("attempt") == before["attempts"] + 1 and
                        after.get("attempts") == before["attempts"] + 1 and
                        after.get("marcstate") == 1 and after.get("ready")):
                    raise RuntimeError("发射过程或结束状态未通过检查；没有自动重发")
            result["host_checks_passed"] = True
    except Exception as error:
        result["host_checks_passed"] = False
        result["error"] = str(error)
        raise
    finally:
        port.close()
        result["serial_records"] = records
        prefix.with_suffix(".json").write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
        print("Evidence:", prefix, flush=True)

if __name__ == "__main__":
    main()
