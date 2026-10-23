# SPDX-License-Identifier: MIT
"""Check repository links, bilingual structure, generated assets and source/configuration hygiene.
检查仓库链接、双语结构、生成资源及源码和配置规范。
"""
import ast
import configparser
import importlib.util
import json
from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys
import tokenize
from urllib.parse import unquote

import yaml
from openapi_spec_validator import validate

ROOT = Path(__file__).resolve().parent.parent
GENERATED = {"web_assets.h", "remote_frames.h"}
CJK = re.compile(r"[\u3400-\u9fff]")
checks = 0


def check(condition, message):
    global checks
    if not condition:
        raise RuntimeError(message)
    checks += 1


def load_json(path):
    return json.loads(path.read_text(encoding="utf-8"))


def public_files():
    raw = subprocess.check_output(
        ["git", "ls-files", "-z", "--cached", "--others", "--exclude-standard"], cwd=ROOT
    )
    return sorted({ROOT / item.decode("utf-8") for item in raw.split(b"\0")
                   if item and (ROOT / item.decode("utf-8")).is_file()})


def prose(text):
    return re.sub(r"^```.*?^```\s*$", "", text, flags=re.M | re.S)


def anchor_ids(path):
    text = path.read_text(encoding="utf-8")
    ids = set(re.findall(r'<a\s+id="([^"]+)"', text))
    counts = {}
    for title in re.findall(r"^#{1,6}\s+(.+?)\s*$", prose(text), re.M):
        title = re.sub(r"<[^>]*>|[`*_]", "", title)
        slug = re.sub(r"[^\w -]", "", title.lower()).replace(" ", "-")
        count = counts.get(slug, 0)
        ids.add(slug if count == 0 else f"{slug}-{count}")
        counts[slug] = count + 1
    return ids


def check_links(paths):
    for path in paths:
        text = prose(path.read_text(encoding="utf-8"))
        targets = re.findall(r"\]\(([^\s)]+)(?:\s+[^)]*)?\)", text)
        targets += re.findall(r"^\[[^]]+\]:\s+(\S+)", text, re.M)
        for target in targets:
            if re.match(r"^[a-zA-Z][a-zA-Z0-9+.-]*:", target):
                continue
            filename, _, fragment = unquote(target.strip("<>")).partition("#")
            resolved = (path.parent / filename).resolve() if filename else path
            check(resolved.is_relative_to(ROOT) and resolved.exists(),
                  f"Broken/outside link: {path.relative_to(ROOT)} -> {target}")
            if fragment and resolved.suffix == ".md":
                check(fragment in anchor_ids(resolved), f"Missing anchor: {path} -> {target}")


def check_pairs():
    en = {path.name for path in (ROOT / "docs/en").glob("*.md")}
    zh = {path.name for path in (ROOT / "docs/zh-CN").glob("*.md")}
    check(en == zh and len(en) >= 10, "Documentation language/file coverage differs")
    pairs = [(ROOT / "docs/en" / name, ROOT / "docs/zh-CN" / name) for name in sorted(en)]
    for stem in ["README", "CONTRIBUTING", "SECURITY", "CODE_OF_CONDUCT",
                 "THIRD_PARTY_NOTICES", "CHANGELOG", "captures/README", "docs/evidence/README"]:
        pairs.append((ROOT / (stem + ".md"), ROOT / (stem + ".zh-CN.md")))
    for a, b in pairs:
        left, right = a.read_text(encoding="utf-8"), b.read_text(encoding="utf-8")
        check(re.findall(r"<!-- section: ([\w-]+) -->", left) ==
              re.findall(r"<!-- section: ([\w-]+) -->", right), f"Section coverage differs: {a}")
        check([len(x) for x in re.findall(r"^(#{1,6}) ", left, re.M)] ==
              [len(x) for x in re.findall(r"^(#{1,6}) ", right, re.M)],
              f"Heading coverage differs: {a}")
        pattern = r"^```(powershell|sh|bash|python|javascript|json)\n(.*?)^```"
        check(re.findall(pattern, left, re.M | re.S) == re.findall(pattern, right, re.M | re.S),
              f"Runnable examples differ: {a}")


def check_config():
    config = configparser.ConfigParser(interpolation=None)
    config.read(ROOT / "platformio.ini", encoding="utf-8")
    check(config["platformio"]["default_envs"] == "phase4", "Default environment changed")
    check(config["env:esp32dev"]["platform"] == "espressif32@6.12.0", "Platform pin changed")
    check(config["env:esp32dev"]["board"] == "esp32dev", "Board changed")
    check("-Werror" in config["env:esp32dev"]["build_src_flags"], "Source warnings not fatal")
    for name in ["esp32dev", "phase2", "phase3", "phase4"]:
        check("build_src_filter" in config[f"env:{name}"], f"Missing source filter: {name}")
    config_text = (ROOT / "include/config.h").read_text(encoding="utf-8")
    for pin, value in {"Csn":5, "Sck":18, "Mosi":23, "Miso":19, "Gdo0":26, "Gdo2":27}.items():
        check(re.search(rf"k{pin}\s*=\s*{value}\s*;", config_text), f"Pin mismatch: {pin}")
    workflow = yaml.load((ROOT / ".github/workflows/ci.yml").read_text(encoding="utf-8"),
                         Loader=yaml.BaseLoader)
    check(set(workflow["on"]) == {"push", "pull_request", "workflow_dispatch"}, "Unsafe CI events")
    check(workflow["permissions"] == {"contents":"read"}, "CI permissions are not minimal")
    for job in workflow["jobs"].values():
        check("timeout-minutes" in job, "CI job lacks timeout")
        for step in job["steps"]:
            if "uses" in step:
                check(re.fullmatch(r"actions/[\w-]+@[0-9a-f]{40}", step["uses"]),
                      f"Unpinned CI action: {step['uses']}")
    for path in (ROOT / ".github/ISSUE_TEMPLATE").glob("*.yml"):
        data = yaml.safe_load(path.read_text(encoding="utf-8"))
        if path.name != "config.yml":
            check(bool(data["name"] and data["description"] and data["body"]), f"Invalid form: {path}")
            check(len({item["id"] for item in data["body"] if "id" in item}) ==
                  len([item for item in data["body"] if "id" in item]), f"Duplicate form ID: {path}")


def check_api():
    spec = load_json(ROOT / "docs/api.openapi.json")
    check(spec["openapi"] == "3.1.0", "Wrong API schema version")
    validate(spec)
    check(True, "OpenAPI schema validation")
    text = (ROOT / "src/network_app.cpp").read_text(encoding="utf-8")
    actual = {(path, method.lower()) for path, method in
              re.findall(r'server\.on\("(/api/[^"]+)",\s*HTTP_(GET|POST)', text)}
    declared = {(path, method) for path, methods in spec["paths"].items() for method in methods}
    check(actual == declared and len(actual) == 8, "Source/OpenAPI routes differ")
    def refs(value):
        if isinstance(value, dict):
            for key, item in value.items():
                if key == "$ref":
                    check(item.startswith("#/"), "External schema reference is not checked")
                    current = spec
                    for part in item[2:].split("/"):
                        current = current[part]
                else:
                    refs(item)
        elif isinstance(value, list):
            for item in value:
                refs(item)
    refs(spec)
    for language in ("en", "zh-CN"):
        text = (ROOT / f"docs/{language}/api.md").read_text(encoding="utf-8")
        for path, method in actual:
            check(method.upper() + " " + path in text, f"Undocumented API: {language} {path}")


def check_sources(paths):
    cpp = [p for p in paths if p.suffix in {".h", ".cpp"} and p.name not in GENERATED]
    formatter = Path(sys.executable).parent / ("clang-format.exe" if sys.platform == "win32" else "clang-format")
    command = str(formatter) if formatter.is_file() else shutil.which("clang-format")
    check(command is not None, "Install requirements-dev.txt for clang-format")
    subprocess.run([command, "--dry-run", "--Werror", *map(str, cpp)], cwd=ROOT, check=True)
    for path in paths:
        # Keep unmodified third-party files byte-exact; their own license/provenance applies.
        # 未改动的第三方文件保留原始字节，使用其自身许可及来源记录。
        if path.is_relative_to(ROOT / "web/vendor"):
            continue
        if path.suffix not in {".py", ".cpp", ".h", ".js", ".css", ".html", ".md", ".yml", ".json"}:
            continue
        text = path.read_text(encoding="utf-8")
        check(text.endswith("\n"), f"Missing final newline: {path}")
        check(not any(line.rstrip() != line for line in text.splitlines()),
              f"Trailing whitespace: {path}")
        if path.suffix == ".py":
            tree = ast.parse(text, filename=str(path))
            for node in ast.walk(tree):
                if isinstance(node, (ast.Module, ast.ClassDef, ast.FunctionDef, ast.AsyncFunctionDef)):
                    doc = ast.get_docstring(node)
                    if doc:
                        check(bool(CJK.search(doc) and re.search(r"[A-Za-z]{3}", doc)),
                              f"Non-bilingual docstring: {path}")
        if path.suffix in {".cpp", ".h"}:
            check("SPDX-License-Identifier: MIT" in text, f"Missing first-party SPDX: {path}")
            for block in re.findall(r"/\*\*.*?\*/", text, re.S):
                check(bool(CJK.search(block) and re.search(r"@brief\s+[A-Za-z]", block)),
                      f"Non-bilingual Doxygen: {path}")
        if path.suffix == ".json":
            load_json(path)
    for path in (ROOT / "scripts").glob("*.py"):
        with tokenize.open(path) as stream:
            ast.parse(stream.read(), filename=str(path))


def check_comment_languages(paths):
    neutral = {"Hz", "MHz", "Baud", "APB", "us", "ms", "SHA256", "SIDLE", "SRX",
               "SI", "SO", "CHIP_RDYn", "v"}
    for path in paths:
        if path.is_relative_to(ROOT / "web/vendor"):
            continue
        if path.suffix not in {".cpp", ".h", ".py", ".js", ".css", ".html", ".ps1", ".ini"}:
            continue
        text = path.read_text(encoding="utf-8")
        entries = []
        if path.suffix == ".py":
            import io
            lines = text.splitlines(keepends=True)
            offsets = [0]
            for line in lines:
                offsets.append(offsets[-1] + len(line))
            for token in tokenize.generate_tokens(io.StringIO(text).readline):
                if token.type == tokenize.COMMENT:
                    entries.append((offsets[token.start[0]-1] + token.start[1],
                                    offsets[token.end[0]-1] + token.end[1], token.string))
        elif path.suffix in {".ps1", ".ini"}:
            marker = "#" if path.suffix == ".ps1" else ";"
            for match in re.finditer(r"(?m)^\s*" + re.escape(marker) + r"[^\n]*", text):
                entries.append((match.start(), match.end(), match.group()))
        else:
            pattern = r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|//[^\n]*|/\*[\s\S]*?\*/|<!--[\s\S]*?-->'
            for match in re.finditer(pattern, text):
                if match.group().startswith(("//", "/*", "<!--")):
                    entries.append((match.start(), match.end(), match.group()))
        merged = []
        for start, end, value in entries:
            if merged and not text[merged[-1][1]:start].strip():
                merged[-1] = (merged[-1][0], end, merged[-1][2] + "\n" + value)
            else:
                merged.append((start, end, value))
        for start, _, value in merged:
            if "SPDX-License-Identifier" in value and len(value.splitlines()) == 1:
                continue
            if re.fullmatch(r"//\s*namespace(?:\s+\w+)?", value.strip()):
                continue
            words = set(re.findall(r"[A-Za-z_]+", value))
            if not CJK.search(value) and words <= neutral:
                continue
            check(bool(CJK.search(value) and re.search(r"[A-Za-z]{3}", value)),
                  f"Unpaired meaningful comment: {path}:{text.count(chr(10), 0, start) + 1}")


def check_assets():
    for name in ("embed_web", "generate_replay_frames"):
        path = ROOT / "scripts" / (name + ".py")
        sys.path.insert(0, str(ROOT / "scripts"))
        spec = importlib.util.spec_from_file_location(name, path)
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        target = "web_assets.h" if name == "embed_web" else "remote_frames.h"
        check((ROOT / "include" / target).read_text(encoding="utf-8") == module.render(),
              f"Stale generated header: {target}")
    manifest = load_json(ROOT / "web/manifest.webmanifest")
    check(manifest["display"] == "standalone", "Manifest display changed")
    for icon in manifest["icons"]:
        check((ROOT / "web" / icon["src"].lstrip("/")).is_file(), "Missing manifest icon")
    for name, expected in [("apple-touch-icon.png",(180,180)),("launch-1206x2622-v1.png",(1206,2622))]:
        raw = (ROOT / "web" / name).read_bytes()
        check(raw[:8] == b"\x89PNG\r\n\x1a\n" and struct.unpack(">II",raw[16:24]) == expected,
              f"PNG dimensions differ: {name}")


def check_evidence():
    """Validate archived JSON references without changing observed outcomes.
    校验归档 JSON 的引用，不改变已记录的观察结果。
    """
    for path in (ROOT / "docs/evidence").glob("historical-*.json"):
        for item in load_json(path).get("evidence", []):
            reference = item if isinstance(item, str) else (item.get("file") if isinstance(item, dict) else None)
            check(isinstance(reference, str), f"Invalid evidence reference in {path.name}")
            target = (path.parent / reference).resolve()
            check(target.is_relative_to(ROOT) and target.is_file(),
                  f"Missing historical evidence reference: {path.name}: {reference}")


def main():
    paths = public_files()
    check_links([p for p in paths if p.suffix == ".md"])
    check_pairs()
    check_config()
    check_api()
    check_sources(paths)
    check_comment_languages(paths)
    check_assets()
    check_evidence()
    check(not any(p.suffix == ".log" for p in paths), "Raw log included in public file set")
    print(f"PASS: {checks} repository checks; {len(paths)} public files; no hardware access")


if __name__ == "__main__":
    main()
