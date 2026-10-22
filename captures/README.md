# Retained RF samples

[简体中文](README.zh-CN.md)

These CSV files were collected from the owner's lamp remote during the recorded 2026-10-21
session. They are real input fixtures, not simulated hardware responses. The
[index](index.json) identifies calibrated buttons, unlabelled debug records and exact source ranges
for the selected [frames](frames/). Its Phase 2 replay flag is historical, not the latest lamp result.

Git preserves CSV bytes so the generated waveform provenance hashes remain valid. The bundled
commands are intentionally public lamp-control samples, not authentication credentials; no address
field interpretation or compatibility with another device is established. Do not reuse them as
security-sensitive access-control examples.

New local captures are ignored by default. Before adding a fixture deliberately, remove personal
device identifiers and unrelated traffic, confirm you may publish it, and retain authentic timing
and source indices. Never fabricate or silently quantize a capture. Raw serial logs remain local
because they can contain credentials/identifiers. See the [RF guide](../docs/en/rf-protocol.md).
