# Historical verification evidence

[简体中文](README.zh-CN.md)

The JSON records here describe earlier real-device checks and operator observations.
The historical hardware records are not new CI results and do not certify the cleaned-up binary.
[open-source-audit.json](open-source-audit.json) is the original local-cleanup snapshot;
[review-remediation.json](review-remediation.json) records the later repository/CI/privacy review.
Failed and before-fix records remain failed; later passing files do not rewrite them.
The [testing guide](../en/testing.md) identifies the relevant passing sets and limitations.

Public JSON copies redact personal LAN addresses, device suffixes and MACs while retaining
check names, booleans, counts, timing and original operator quotations. Original files and byte-exact
serial logs were preserved locally in ignored build/private before publication cleanup.
Raw serial logs are excluded because future records may expose access credentials.
The baseline had no commits, so no committed history was rewritten or deleted.

The later review found no further credential/device identifiers in the tracked files or reachable
initial commits. Short technical feedback and intentional lamp RF fixtures remain public. Only the 43 evidence-array references in historical-phase3/phase4-validation.json were
normalized after relocation; each record_maintenance field describes the change. Outcomes and
quotations are unchanged.
