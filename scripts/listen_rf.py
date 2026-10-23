# SPDX-License-Identifier: MIT
"""Renew untriggered listening windows to reduce missed manual button presses.
等待一次操作；无触发时自动续期，减少人工按键与监听窗口错过的机会。
"""
import argparse
import json
from pathlib import Path
import re
import subprocess
import sys


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", required=True)
    parser.add_argument("--label", required=True)
    parser.add_argument("--windows", type=int, default=6)
    parser.add_argument("--seconds", type=float, default=1)
    parser.add_argument("--trigger", type=float, default=-80)
    parser.add_argument("--frequency-khz", type=int, default=433920)
    parser.add_argument("--profile", choices=["default", "wide", "narrow", "fast"], default="default")
    args = parser.parse_args()
    if not re.fullmatch(r"[A-Za-z0-9_-]{1,32}", args.label) or not 1 <= args.windows <= 10:
        parser.error("label 需为 1..32 位字母/数字/下划线/连字符；windows 为 1..10")
    root = Path(__file__).resolve().parent.parent
    for attempt in range(1, args.windows + 1):
        label = f"{args.label}_wait{attempt}"
        print(f"Listening window {attempt}/{args.windows}", flush=True)
        command = [
            sys.executable, str(root / "scripts/collect_rf.py"), "--port", args.port,
            "--label", label, "--seconds", str(args.seconds), "--wait", "60",
            "--trigger", str(args.trigger), "--frequency-khz", str(args.frequency_khz),
            "--profile", args.profile,
        ]
        result = subprocess.run(command, cwd=root)
        if result.returncode == 0:
            return 0
        captures = sorted((root / "captures").glob(f"*_{label}.json"))
        if not captures or json.loads(captures[-1].read_text(encoding="utf-8"))["capture"]["reason"] != "trigger_timeout":
            return result.returncode
    print("No RF trigger; every listening window has been saved.", flush=True)
    return 2


if __name__ == "__main__":
    sys.exit(main())
