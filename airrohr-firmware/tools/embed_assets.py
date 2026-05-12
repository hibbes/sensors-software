#!/usr/bin/env python3
"""
Embed CSS/JS-Files aus web/assets/ als PROGMEM-Konstanten in
web/assets_generated.{h,cpp}.

Wird aus dem PlatformIO-extra_scripts vor jedem Build aufgerufen.

Issue #18 Phase D — Asset-Pipeline.

Minifizierung passiert bei CSS einfach: alle Newlines + leading whitespace raus.
Bei JS bleibt der Source-Text 1:1, weil das ESP8266 sonst zu fragil mit
parsing wird (Closure-Compiler oder terser wäre Overhead nur für 0.3 KB).
"""

from pathlib import Path
import re
import sys


def minify_css(text: str) -> str:
    """Reduziere CSS auf eine Zeile, droppe alle line-leading whitespace
    und zwischenzeilige Komentare. Behält Funktionalität, spart ~30%."""
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.DOTALL)
    text = re.sub(r"\n\s*", "", text)
    return text.strip()


def to_c_literal(text: str) -> str:
    """C-Escape: Backslash zuerst, dann quotes, then non-printable chars."""
    out = []
    for ch in text:
        if ch == "\\":
            out.append("\\\\")
        elif ch == '"':
            out.append('\\"')
        elif ch == "\n":
            out.append("\\n")
        elif ch == "\r":
            out.append("\\r")
        elif ch == "\t":
            out.append("\\t")
        else:
            out.append(ch)
    return '"' + "".join(out) + '"'


ASSETS_DIR = Path(__file__).resolve().parent.parent / "web" / "assets"
OUT_H = ASSETS_DIR.parent / "assets_generated.h"
OUT_CPP = ASSETS_DIR.parent / "assets_generated.cpp"


def main():
    files = sorted(ASSETS_DIR.glob("*.css")) + sorted(ASSETS_DIR.glob("*.js"))
    if not files:
        print("No assets found.", file=sys.stderr)
        return 1

    header_lines = [
        "/**",
        " * GENERATED FROM web/assets/*.{css,js} via tools/embed_assets.py",
        " * Do not edit directly. Re-run the embed step instead.",
        " *",
        " * Issue #18 Phase D — Asset-Pipeline.",
        " */",
        "#pragma once",
        "",
        "#include <Arduino.h>",
        "",
    ]
    cpp_lines = [
        "/**",
        " * GENERATED FROM web/assets/*.{css,js} via tools/embed_assets.py",
        " * Do not edit directly. Re-run the embed step instead.",
        " */",
        '#include "./assets_generated.h"',
        "",
    ]

    for path in files:
        stem = path.stem.replace("-", "_").upper()
        ext = path.suffix.lstrip(".").upper()
        symbol = f"ASSET_{stem}_{ext}"
        len_symbol = f"{symbol}_LEN"
        text = path.read_text(encoding="utf-8")

        if path.suffix == ".css":
            text = minify_css(text)

        literal = to_c_literal(text)
        header_lines.append(f"extern const char {symbol}[] PROGMEM;")
        header_lines.append(f"extern const size_t {len_symbol};")
        cpp_lines.append(f"const char {symbol}[] PROGMEM = {literal};")
        cpp_lines.append(f"const size_t {len_symbol} = sizeof({symbol}) - 1;")
        print(f"embed: {path.name} -> {symbol} ({len(text)} B)")

    header_lines.append("")
    cpp_lines.append("")
    OUT_H.write_text("\n".join(header_lines), encoding="utf-8")
    OUT_CPP.write_text("\n".join(cpp_lines), encoding="utf-8")
    print(f"wrote: {OUT_H} + {OUT_CPP}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
