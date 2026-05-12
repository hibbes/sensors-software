#!/usr/bin/env python3
"""
Generiert intl_<lang>.h Files aus tools/intl/master.csv.

Verwendung:
    python3 tools/intl/generate.py            # schreibt intl_<lang>.h direkt
    python3 tools/intl/generate.py --dry      # schreibt intl_<lang>.h.generated
    python3 tools/intl/generate.py --check    # exit 1 bei Drift gegen Quellen

master.csv-Schema:
    key,type,en,bg,br,cn,...

type ∈ {define, const}. const-Keys werden in `#ifdef INTL_DEFINE_VARIABLES`-
Block gewrappt (siehe Issue #18 Phase B). define-Keys bleiben außerhalb.

Logo: alle Sprachen außer `bg` inkludieren am Ende `airrohr-logo-common.h`.
intl_bg.h hat sein eigenes kyrillisches Logo direkt im File — das wird NICHT
über master.csv gepflegt (nicht übersetzbar, ist Binary-Daten). Der Generator
schreibt es deshalb nicht und das bestehende intl_bg.h muss separat gepflegt
werden.

Issue #18 Phase F-2 / Issue #9.
"""
from pathlib import Path
import csv
import re
import sys

INTL_DIR = Path(__file__).resolve().parent.parent.parent
MASTER_CSV = Path(__file__).resolve().parent / "master.csv"
BG_LOGO_FILE = Path(__file__).resolve().parent / "bg_logo.h.fragment"


def load_master():
    """Returns (langs, rows: list of (key, type, {lang: value}))."""
    with open(MASTER_CSV, encoding="utf-8") as f:
        reader = csv.reader(f)
        header = next(reader)
        langs = header[2:]
        rows = []
        for r in reader:
            if not r or not r[0]:
                continue
            key = r[0]
            kind = r[1]
            values = {langs[i]: r[2 + i] for i in range(len(langs))}
            rows.append((key, kind, values))
    return langs, rows


def preserve_header(target: Path) -> str:
    """Extract the leading comment block (Copyright, attribution, etc.) from
    an existing intl_<lang>.h, up to the first directive that we generate."""
    if not target.exists():
        return ""
    lines = target.read_text(encoding="utf-8").splitlines()
    header = []
    for line in lines:
        stripped = line.lstrip()
        if (
            stripped.startswith("#pragma once")
            or stripped.startswith("#define INTL_LANG")
            or stripped.startswith("#ifdef")
            or stripped.startswith("#include")
            or (stripped.startswith("#define") and "INTL_" in stripped)
        ):
            break
        header.append(line)
    while header and not header[-1].strip():
        header.pop()
    return "\n".join(header)


def render_intl_file(lang: str, rows, original: Path) -> str:
    """Render intl_<lang>.h contents."""
    out = []
    header = preserve_header(original)
    if header:
        out.append(header)
        out.append("")
    out.append("#pragma once")
    out.append("")
    out.append("// Übersetzungen aus tools/intl/master.csv generiert (Issue #18 Phase F-2).")
    out.append("// Generator: tools/intl/generate.py")
    out.append("// const-typed strings sind im #ifdef INTL_DEFINE_VARIABLES-Block; nur die")
    out.append("// .ino emittiert Definitionen, andere TUs sehen extern-Decls aus intl-decls.h.")
    out.append("")

    # Group defines and consts in original master order, but emit consts inside
    # a single ifdef/endif block.
    in_var_block = False
    for key, kind, values in rows:
        val = values.get(lang, "")
        if kind == "define":
            if in_var_block:
                out.append("#endif // INTL_DEFINE_VARIABLES")
                in_var_block = False
            out.append(f'#define {key} "{val}"')
        elif kind == "const":
            if not in_var_block:
                out.append("")
                out.append("#ifdef INTL_DEFINE_VARIABLES")
                in_var_block = True
            out.append(f'const char {key}[] PROGMEM = "{val}";')
    if in_var_block:
        out.append("#endif // INTL_DEFINE_VARIABLES")

    out.append("")
    # Logo
    if lang == "bg":
        # intl_bg.h hat sein eigenes kyrillisches Logo. Wir lesen es aus einem
        # gepinten Fragment-File und schreiben es hier ein.
        if BG_LOGO_FILE.exists():
            out.append(BG_LOGO_FILE.read_text(encoding="utf-8").rstrip())
        else:
            out.append('#include "./airrohr-logo-common.h"')
    else:
        out.append('#include "./airrohr-logo-common.h"')
    out.append("")
    return "\n".join(out)


def diff_summary(a: str, b: str) -> str:
    """Compare two intl-files semantically: extract (kind, key, value) tuples."""
    def extract(text):
        text = re.sub(r"^#ifdef\s+INTL_DEFINE_VARIABLES.*?$", "", text, flags=re.MULTILINE)
        text = re.sub(r"^#endif\s*//\s*INTL_DEFINE_VARIABLES.*?$", "", text, flags=re.MULTILINE)
        text = text.replace("\\\n", "")
        out = {}
        for line in text.splitlines():
            m = re.match(r'^\s*#define\s+(INTL_\w+)\s+"(.*)"\s*$', line)
            if m:
                out[m.group(1)] = ("define", m.group(2))
                continue
            m = re.match(r'^\s*const\s+char\s+(INTL_\w+)\[\]\s+PROGMEM\s*=\s*"(.*)"\s*;\s*$', line)
            if m:
                out[m.group(1)] = ("const", m.group(2))
        return out
    aa = extract(a)
    bb = extract(b)
    diffs = []
    for k in set(aa) | set(bb):
        if aa.get(k) != bb.get(k):
            diffs.append(f"{k}: {aa.get(k)} != {bb.get(k)}")
    return diffs


def main():
    mode = "write"
    if "--dry" in sys.argv:
        mode = "dry"
    elif "--check" in sys.argv:
        mode = "check"

    langs, rows = load_master()
    rc = 0
    for lang in langs:
        target = INTL_DIR / f"intl_{lang}.h"
        rendered = render_intl_file(lang, rows, target)

        if mode == "check":
            if not target.exists():
                print(f"MISSING {target.name}")
                rc = 1
                continue
            existing = target.read_text(encoding="utf-8")
            diffs = diff_summary(existing, rendered)
            if diffs:
                print(f"DRIFT  {target.name}: {len(diffs)} entries differ")
                for d in diffs[:5]:
                    print(f"        {d}")
                rc = 1
            else:
                print(f"OK     {target.name}")
        elif mode == "dry":
            out = target.with_suffix(".h.generated")
            out.write_text(rendered, encoding="utf-8")
            print(f"wrote  {out}")
        else:
            target.write_text(rendered, encoding="utf-8")
            print(f"wrote  {target}")
    return rc


if __name__ == "__main__":
    sys.exit(main())
