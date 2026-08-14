#!/usr/bin/env python3
"""
Cross-platform webpanel asset builder.

Python equivalent of html_converter.sh for Windows/macOS/Linux.
"""

from __future__ import annotations

import argparse
import gzip
import re
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Iterable, List


ROOT = Path(__file__).resolve().parent.parent
WEB = ROOT / "webpanel"
INC = ROOT / "include"

JS_SRC = WEB / "js" / "index.js"
CSS_SRC = WEB / "css" / "styles.css"
HTML_SRC = WEB / "index.html"

JS_TMP = WEB / "js" / "index.tmp.js"
HTML_TMP = WEB / "index.tmp.html"
HTML_MIN = WEB / "index.min.html"
JS_MIN = WEB / "js" / "index.min.js"
CSS_MIN = WEB / "css" / "styles.min.css"

HTML_GZ = WEB / "index.min.html.gz"
JS_GZ = WEB / "js" / "index.min.js.gz"
CSS_GZ = WEB / "css" / "styles.min.css.gz"


def _strip_inline_comment(line: str) -> str:
    for mark in (";", "#"):
        idx = line.find(mark)
        if idx != -1:
            line = line[:idx]
    return line.rstrip()


def _read_extra_version(lines: Iterable[str]) -> str:
    in_extra = False
    for raw in lines:
        line = raw.strip()
        if line == "[extra]":
            in_extra = True
            continue
        if line.startswith("["):
            in_extra = False
        if not in_extra:
            continue
        clean = _strip_inline_comment(raw).strip()
        m = re.match(r"^version\s*=\s*(.+)$", clean)
        if m:
            return m.group(1).strip()
    return ""


def _require_cmd(cmd: str) -> None:
    if shutil.which(cmd) is None:
        print(f"Missing required command: {cmd}", file=sys.stderr)
        sys.exit(1)


def _resolve_cmd(cmd: str) -> str:
    resolved = shutil.which(cmd)
    return resolved if resolved is not None else cmd


def _run(cmd: List[str]) -> None:
    subprocess.run([_resolve_cmd(cmd[0]), *cmd[1:]], cwd=ROOT, check=True)


def _gzip_deterministic(src: Path, dst: Path) -> None:
    data = src.read_bytes()
    # Deterministic gzip stream (no timestamp/filename metadata).
    # Match legacy `gzip -n` behavior as closely as possible:
    # - compression level 6 (default)
    # - OS field set to Unix (0x03) to keep output stable vs previous assets.
    gz = gzip.compress(data, compresslevel=6, mtime=0)
    if len(gz) > 9:
        gz = gz[:9] + b"\x03" + gz[10:]
    dst.write_bytes(gz)


def _to_header_bytes(name: str, data: bytes) -> str:
    lines: List[str] = []
    lines.append("#include <Arduino.h>")
    lines.append(f"const uint8_t {name}[] PROGMEM = {{")

    for pos in range(0, len(data), 12):
        chunk = data[pos : pos + 12]
        line = "  " + ", ".join(f"0x{b:02x}" for b in chunk)
        if pos + len(chunk) < len(data):
            line += ","
        lines.append(line)

    lines.append("};")
    lines.append(f"unsigned int {name}_len = {len(data)};")
    lines.append("")
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(description="Build/minify webpanel assets and generate PROGMEM headers.")
    parser.parse_args()

    for cmd in ("html-minifier", "uglifyjs", "uglifycss"):
        _require_cmd(cmd)

    print("== Building webpanel assets ==")
    print("")
    print(f"ROOT: {ROOT}")
    print("")

    pio_lines = (ROOT / "platformio.ini").read_text(encoding="utf-8").splitlines()
    asset_version = _read_extra_version(pio_lines)
    if not asset_version:
        print("Could not read [extra] version from platformio.ini", file=sys.stderr)
        return 1

    print(f"ASSET_VERSION: {asset_version}")
    print("")

    # 1) Prepare JS temp: drop first line, inject baseUrl.
    js_lines = JS_SRC.read_text(encoding="utf-8").splitlines(keepends=True)
    JS_TMP.write_text("let baseUrl = \"\";\n" + "".join(js_lines[1:]), encoding="utf-8")

    # 1b) Inject cache-busting version in HTML.
    html_text = HTML_SRC.read_text(encoding="utf-8")
    HTML_TMP.write_text(html_text.replace("__ASSET_VERSION__", asset_version), encoding="utf-8")

    # 2) Minify assets.
    _run(["html-minifier", "--collapse-whitespace", str(HTML_TMP), "-o", str(HTML_MIN)])
    _run(["uglifyjs", "--compress", "--mangle", "-o", str(JS_MIN), str(JS_TMP)])
    with CSS_MIN.open("w", encoding="utf-8", newline="\n") as css_out:
        subprocess.run([_resolve_cmd("uglifycss"), str(CSS_SRC)], cwd=ROOT, check=True, text=True, stdout=css_out)

    # 3) Gzip (deterministic).
    _gzip_deterministic(HTML_MIN, HTML_GZ)
    _gzip_deterministic(JS_MIN, JS_GZ)
    _gzip_deterministic(CSS_MIN, CSS_GZ)

    # 4) Convert to PROGMEM headers.
    (INC / "IndexHtml.h").write_text(_to_header_bytes("index_html", HTML_GZ.read_bytes()), encoding="utf-8")
    (INC / "IndexJs.h").write_text(_to_header_bytes("index_js", JS_GZ.read_bytes()), encoding="utf-8")
    (INC / "StylesMinCss.h").write_text(_to_header_bytes("styles_min_css", CSS_GZ.read_bytes()), encoding="utf-8")

    # 5) Cleanup temporary/minified artifacts.
    for f in (JS_TMP, HTML_TMP, HTML_MIN, JS_MIN, CSS_MIN, HTML_GZ, JS_GZ, CSS_GZ):
        if f.exists():
            f.unlink()

    print("== Done. Generated: include/IndexHtml.h include/IndexJs.h include/StylesMinCss.h ==")
    print("")
    return 0


if __name__ == "__main__":
    sys.exit(main())
