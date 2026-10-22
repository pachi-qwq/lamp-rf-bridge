# SPDX-License-Identifier: MIT
"""Analyze raw pulses without assuming a protocol or frame length.
只统计原始脉冲；分隔候选与时长簇均为分析线索，不预设协议或位数。
"""
import argparse
from collections import Counter
import csv
import json
from pathlib import Path
from statistics import median



def pwm_candidates(pulses, gap_us):
    """Find complementary PWM candidates without fixing bit count or repairing missing edges.
从实际观测到的长短互补脉冲中找候选帧，不固定数据位数、不修补缺失边沿。
"""
    pairs = []
    for i in range(len(pulses) - 1):
        high, low = pulses[i:i + 2]
        if high[0] != 1 or low[0] != 0:
            continue
        short, long = sorted((high[1], low[1]))
        if 200 <= short <= 700 and 800 <= long <= 1600 and 1300 <= short + long <= 1900:
            pairs.append((short, long))
    if len(pairs) < 8:
        return []
    short = median(value[0] for value in pairs)
    long = median(value[1] for value in pairs)

    def bit_at(index):
        if index < 0 or index + 1 >= len(pulses):
            return None
        high, low = pulses[index:index + 2]
        if high[0] != 1 or low[0] != 0:
            return None
        is_short = lambda value: short * 0.55 <= value <= short * 1.6
        is_long = lambda value: long * 0.65 <= value <= long * 1.35
        if is_long(high[1]) and is_short(low[1]):
            return "1"
        if is_short(high[1]) and is_long(low[1]):
            return "0"
        return None

    candidates = []
    for i in range(1, len(pulses)):
        if pulses[i][0] != 0 or pulses[i][1] < gap_us:
            continue
        sync_high = pulses[i - 1]
        if sync_high[0] != 1 or not short * 0.55 <= sync_high[1] <= short * 1.6:
            continue
        backwards = []
        cursor = i - 3
        while cursor >= 0:
            bit = bit_at(cursor)
            if bit is None:
                break
            backwards.append(bit)
            cursor -= 2
        if len(backwards) < 8:
            continue
        bits = "".join(reversed(backwards))
        candidates.append({
            "start_index": i - 1 - 2 * len(bits), "end_index": i,
            "bit_count": len(bits), "bits": bits,
            "hex": f"{int(bits, 2):0{(len(bits) + 3) // 4}X}",
            "sync_high_us": sync_high[1], "sync_low_us": pulses[i][1],
            "short_median_us": short, "long_median_us": long,
            "candidate_only": True,
        })
    return candidates


def summarize(path, gap_us):
    with path.open(encoding="utf-8", newline="") as source:
        rows = list(csv.DictReader(source))
    pulses = []
    for i, row in enumerate(rows):
        if int(row["index"]) != i or row["level"] not in ("0", "1"):
            raise ValueError(f"{path}: 索引/电平不连续")
        duration = int(row["duration_us"])
        if duration < 0:
            raise ValueError(f"{path}: 负持续时间")
        pulses.append((int(row["level"]), duration))
    # The first segment may be truncated; retain it in raw data but exclude it from timing statistics.
    # 首段可能由开始采样截断，保留在原始数据中，统计时排除。
    interior = pulses[1:]
    clusters = {}
    for level in [0, 1]:
        durations = [duration for value, duration in interior if value == level and 20 <= duration < gap_us]
        bins = Counter(round(value / 25) * 25 for value in durations)
        clusters[str(level)] = {"count": len(durations), "median_us": median(durations) if durations else None,
                               "top_25us_bins": bins.most_common(12)}
    runs = []
    current = []
    for value, duration in interior:
        if duration >= gap_us:
            if current:
                runs.append(current)
            current = []
        else:
            current.append((value, duration))
    if current:
        runs.append(current)
    return {"file": str(path), "pulse_count": len(pulses), "total_duration_us": sum(x[1] for x in pulses),
            "glitches_le_10us": sum(x[1] <= 10 for x in interior), "levels": clusters,
            "gap_candidate_us": gap_us, "run_pulse_counts": [len(run) for run in runs if len(run) >= 16],
            "pwm_candidates": pwm_candidates(pulses, gap_us), "protocol_confirmed": False}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("files", type=Path, nargs="+")
    parser.add_argument("--gap-us", type=int, default=5000)
    args = parser.parse_args()
    if args.gap_us <= 20:
        parser.error("--gap-us 必须大于 20")
    for path in args.files:
        print(json.dumps(summarize(path, args.gap_us), ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
