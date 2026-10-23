# SPDX-License-Identifier: MIT
"""Generate constants from retained real CSV frames; --check verifies provenance without hardware.
从留存的真实按键 CSV 生成固件常量；--check 检查来源一致性，不访问硬件。
"""
import argparse
import csv
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

def render():
    manifest = json.loads((ROOT / "captures/index.json").read_text(encoding="utf-8"))
    buttons = manifest["buttons"]
    arrays = []
    entries = []
    for button in buttons:
        selected = button["selected_full_frame"]
        source = ROOT / "captures" / selected["file"]
        with source.open(newline="") as f:
            rows = list(csv.DictReader(f))
        with (ROOT / "captures" / selected["source_csv"]).open(newline="") as f:
            raw = list(csv.DictReader(f))
        assert button["operator_identified_key"] and len(rows) == 50
        for i, row in enumerate(rows):
            original = raw[selected["start_index"] + i]
            assert int(row["source_index"]) == selected["start_index"] + i
            assert (row["level"], row["duration_us"]) == (original["level"], original["duration_us"])
            assert int(row["level"]) == (1 if i % 2 == 0 else 0)
        bits = "".join("1" if int(rows[i]["duration_us"]) > int(rows[i + 1]["duration_us"])
                       else "0" for i in range(0, 48, 2))
        assert f"{int(bits, 2):06X}" == button["observed_frame_hex"]
        digest = hashlib.sha256(source.read_bytes()).hexdigest()
        symbol = "kFrame_" + button["button"]
        pulses = "\n".join(f"    {{{row['duration_us']}, {row['level']}}}," for row in rows)
        arrays.append(
            f'// Source: captures/{selected["file"]}, original indices {selected["start_index"]}..{selected["end_index"]}.\n'
            f'// 来源 captures/{selected["file"]}，原始索引 {selected["start_index"]}..{selected["end_index"]}\n'
            f'// SHA256 {digest}\ninline constexpr Pulse {symbol}[] = {{\n{pulses}\n}};\n'
        )
        repeat = "replay_config::kAdjustmentRepeats" if button["button"] in ("brightness_up", "brightness_down", "cct_up", "cct_down") else "replay_config::kRepeats"
        entries.append(f'    {{"{button["button"]}", "{button["observed_frame_hex"]}", {symbol}, {repeat}}},')
    return (
        '// SPDX-License-Identifier: MIT\n#pragma once\n#include <cstring>\n#include "replay_plan.h"\n\nnamespace replay {\n'
        '// Generated from real samples by scripts/generate_replay_frames.py; durations are unchanged.\n'
        '// 由 scripts/generate_replay_frames.py 生成，逐项保留真实采样，不作理想化处理。\n'
        + "\n".join(arrays) +
        '\n/** @brief Captured command name, display hex, 50 measured pulses and repeat count.\n'
        ' * 已采集命令名、显示用十六进制、50 段实测脉冲与重复数。 */\n'
        'struct Frame { const char* name; const char* hex; const Pulse* pulses; uint8_t repeats; };\n'
        'inline constexpr Frame kFrames[] = {\n' + "\n".join(entries) + '\n};\n'
        '/** @brief Find an exact built-in command name; unknown names return nullptr.\n'
        ' * 按精确名称查找内置命令；未知名称返回 nullptr。\n'
        ' * @param name Non-null NUL-terminated name. / 非空且 NUL 结尾的名称。\n'
        ' * @return Static frame pointer or nullptr. / 静态帧指针或 nullptr。 */\n'
        'inline const Frame* findFrame(const char* name) {\n'
        '  for (const auto& frame : kFrames) if (std::strcmp(name, frame.name) == 0) return &frame;\n'
        '  return nullptr;\n}\n'
        '}  // namespace replay\n'
    )

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    target = ROOT / "include/remote_frames.h"
    content = render()
    if args.check:
        if target.read_text(encoding="utf-8") != content:
            raise SystemExit("按键常量与实测 CSV 不一致")
        print("PASS: 七个固件波形与真实 CSV 及原始采样逐项一致")
    else:
        target.write_text(content, encoding="utf-8", newline="\n")
        print(target)

if __name__ == "__main__":
    main()
