# SPDX-License-Identifier: MIT
"""Bundle/compress a single-response page with exact CSP hashes; --check validates generated assets.
将页面打包为单次响应并压缩嵌入 Flash；生成精确 CSP 哈希，--check 核对产物。
"""
import argparse
import base64
import gzip
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

def bundle():
    html = (ROOT / "web/index.html").read_text(encoding="utf-8")
    css = (ROOT / "web/style.css").read_text(encoding="utf-8")
    license_text = (ROOT / "web/vendor/blueimp-md5-LICENSE.txt").read_text(encoding="utf-8")
    script = "/*! blueimp-md5@2.19.0\n" + license_text + "\n*/\n" + "\n".join((ROOT / path).read_text(encoding="utf-8") for path in ("web/vendor/md5-2.19.0.min.js", "web/auth.js", "web/app.js"))
    style_tag = '<link rel="stylesheet" href="/style.css">'
    script_tag = '<script defer src="/app.js"></script>'
    if html.count(style_tag) != 1 or html.count(script_tag) != 1 or html.count("</body>") != 1:
        raise ValueError("首屏打包标记不匹配")
    if "</style" in css.lower() or "</script" in script.lower():
        raise ValueError("资源中含有会提前终止内联标签的内容")
    html = html.replace(style_tag, "<style>" + css + "</style>")
    html = html.replace(script_tag, "")
    html = html.replace("</body>", "<script>" + script + "</script>\n</body>")
    digest = lambda text: "'sha256-" + base64.b64encode(hashlib.sha256(text.encode("utf-8")).digest()).decode("ascii") + "'"
    # Avoid unsafe-inline: scripts/styles must match the generated hashes byte for byte.
    # 不使用 unsafe-inline：脚本与样式必须逐字节匹配生成时的哈希。
    csp = ("default-src 'none'; script-src 'self' " + digest(script) +
           "; style-src 'self' " + digest(css) +
           "; img-src 'self' data:; connect-src 'self'; manifest-src 'self'; "
           "form-action 'self'; frame-ancestors 'none'; base-uri 'none'")
    return html.encode("utf-8"), csp

def render():
    index, csp = bundle()
    chunks = ['// SPDX-License-Identifier: MIT\n#pragma once\n#include <Arduino.h>\nnamespace web_assets {\n']
    chunks.append("inline constexpr const char* kCsp = " + json.dumps(csp) + ";\n")
    for name, symbol in (("index.html", "kIndex"), ("style.css", "kStyle"), ("app.js", "kScript"), ("manifest.webmanifest", "kManifest"), ("icon.svg", "kIcon"), ("apple-touch-icon.png", "kAppleIcon"), ("launch-1206x2622-v1.png", "kLaunch")):
        raw = index if name == "index.html" else (ROOT / "web" / name).read_bytes()
        if name.endswith(".png"):
            data = raw
        else:
            # Python 3.11 delegates mtime=0 to zlib; normalize its OS byte for cross-platform output.
            # Python 3.11 的 mtime=0 使用 zlib；统一 OS 字节以保证跨平台产物一致。
            data = bytearray(gzip.compress(raw, compresslevel=9, mtime=0))
            data[9] = 255
            data = bytes(data)
        lines = [",".join(f"0x{b:02x}" for b in data[i:i+24]) for i in range(0, len(data), 24)]
        chunks.append(f"// Generated from web/{name}: {len(raw)} bytes -> {len(data)} stored bytes.\n"
                      f"// 由 web/{name} 生成：{len(raw)} 字节 -> 存储 {len(data)} 字节。\n")
        chunks.append(f"inline const uint8_t {symbol}[] PROGMEM = {{\n" + ",\n".join(lines) + "\n};\n")
    chunks.append("}  // namespace web_assets\n")
    return "".join(chunks)

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    target = ROOT / "include/web_assets.h"
    content = render()
    if args.check:
        if not target.is_file() or target.read_text(encoding="utf-8") != content:
            raise SystemExit("静态页面产物过期，请运行 scripts/embed_web.py")
        print("PASS: 七份 Web 资源、单响应首屏及 CSP 哈希与固件一致")
    else:
        target.write_text(content, encoding="utf-8", newline="\n")
        print("Embedded single-response page and web assets")

if __name__ == "__main__":
    main()
