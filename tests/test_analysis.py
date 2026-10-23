# SPDX-License-Identifier: MIT
"""Regress retained real captures offline; never simulate hardware or create fake capture logs.
只用项目中留存的真实采样做离线回归，不模拟硬件或生成假采集日志。
"""
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "scripts"))
from analyze_rf import summarize


class RealCaptureAnalysisTests(unittest.TestCase):
    def test_first_capture_keeps_incomplete_prefix_separate(self):
        value = summarize(ROOT / "captures/20261021_224740_328966_power_on_wait_extended.csv", 5000)
        self.assertEqual(value["pulse_count"], 188)
        self.assertFalse(value["protocol_confirmed"])
        self.assertEqual([(p["bit_count"], p["hex"]) for p in value["pwm_candidates"]],
                         [(21, "1FFF02"), (24, "FFFF02")])

    def test_second_capture_repeats_same_payload(self):
        value = summarize(ROOT / "captures/20261021_225220_396139_power_on_02_wait1.csv", 5000)
        full = [p for p in value["pwm_candidates"] if p["bit_count"] == 24]
        self.assertEqual(value["pulse_count"], 254)
        self.assertEqual(len(full), 3)
        self.assertEqual({p["hex"] for p in full}, {"FFFF08"})
        self.assertTrue(all(p["candidate_only"] for p in full))

    def test_empty_background_does_not_produce_frame(self):
        value = summarize(ROOT / "captures/20261021_224543_482947_idle_default.csv", 5000)
        self.assertEqual(value["pulse_count"], 0)
        self.assertEqual(value["pwm_candidates"], [])


    def test_labelled_buttons_have_distinct_repeated_full_frames(self):
        import json
        manifest = json.loads((ROOT / "captures/index.json").read_text(encoding="utf-8"))
        observed = []
        for button in manifest["buttons"]:
            with self.subTest(button=button["button"]):
                expected = button["observed_frame_hex"]
                observed.append(expected)
                for sample in button["samples"]:
                    path = ROOT / "captures" / sample["source_csv"]
                    value = summarize(path, 5000)
                    frames = [p for p in value["pwm_candidates"] if p["bit_count"] == 24]
                    self.assertGreaterEqual(len(frames), 2)
                    self.assertEqual({p["hex"] for p in frames}, {expected})
                    self.assertEqual(len(frames), sample["complete_frame_count"])
                    self.assertFalse(sample["overflow"])
        self.assertEqual(len(set(observed)), 7)

    def test_selected_frames_preserve_original_samples(self):
        import csv
        import json
        manifest = json.loads((ROOT / "captures/index.json").read_text(encoding="utf-8"))
        for button in manifest["buttons"]:
            with self.subTest(button=button["button"]):
                frame = button["selected_full_frame"]
                with (ROOT / "captures" / frame["source_csv"]).open(newline="") as f:
                    original = list(csv.DictReader(f))
                with (ROOT / "captures" / frame["file"]).open(newline="") as f:
                    selected = list(csv.DictReader(f))
                self.assertEqual(len(selected), 50)
                source = original[frame["start_index"]:frame["end_index"] + 1]
                self.assertEqual([(r["level"], r["duration_us"]) for r in selected],
                                 [(r["level"], r["duration_us"]) for r in source])
                self.assertEqual([int(r["source_index"]) for r in selected],
                                 list(range(frame["start_index"], frame["end_index"] + 1)))



if __name__ == "__main__":
    unittest.main()
