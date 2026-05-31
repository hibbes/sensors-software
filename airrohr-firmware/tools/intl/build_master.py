#!/usr/bin/env python3
"""
Forward-engineer master.csv aus den existierenden 30 intl_<lang>.h-Files.

Master-Schema:
    key,type,<lang1>,<lang2>,...
    INTL_LANG,define,EN,DE,BG,...
    INTL_X,define,"English text","Deutscher Text",...
    INTL_Y,const,"English string","Deutsche Zeichenkette",...

type ist "define" oder "const" (eine eindeutige Wahl pro Key, im Master
festgelegt durch das Schema von intl_en.h).

Issue #18 Phase F-2 / Issue #9.
"""
from pathlib import Path
import re
import csv
import sys

INTL_DIR = Path(__file__).resolve().parent.parent.parent
MASTER_LANG = "en"
MASTER_FILE = INTL_DIR / "intl_en.h"

# Allow an optional trailing // line comment after the closing quote so that
# a line like `#define INTL_X "hello"  // comment` is parsed as the string body
# "hello" rather than falling through to the NOSTR fallback (which would capture
# the quotes and comment verbatim and corrupt the round-trip).
DEFINE_RE = re.compile(r'^\s*#define\s+(INTL_\w+)\s+"(.*)"\s*(?://.*)?$')
DEFINE_NOSTR_RE = re.compile(r"^\s*#define\s+(INTL_\w+)\s+(.+?)\s*$")
CONST_RE = re.compile(
    r'^\s*const\s+char\s+(INTL_\w+)\[\]\s+PROGMEM\s*=\s*"(.*)"\s*;\s*(?://.*)?$'
)


def parse_intl_file(path: Path):
    """Returns OrderedDict {key: (type, value)} keeping key insertion order
    and dropping INTL_DEFINE_VARIABLES guards."""
    out = {}
    text = path.read_text(encoding="utf-8")
    text = re.sub(r"^#ifdef\s+INTL_DEFINE_VARIABLES.*?$", "", text, flags=re.MULTILINE)
    text = re.sub(r"^#endif\s*//\s*INTL_DEFINE_VARIABLES.*?$", "", text, flags=re.MULTILINE)
    text = text.replace("\\\n", "")
    for line in text.splitlines():
        m = DEFINE_RE.match(line)
        if m:
            out[m.group(1)] = ("define", m.group(2))
            continue
        m = CONST_RE.match(line)
        if m:
            out[m.group(1)] = ("const", m.group(2))
            continue
        m = DEFINE_NOSTR_RE.match(line)
        if m and m.group(1) not in out:
            value = m.group(2)
            # A quote in the NOSTR branch means the strict string regex failed
            # to parse a string-define — typically a trailing comment the regex
            # didn't anticipate or adjacent C string concatenation ("a" "b").
            # Capturing it verbatim would silently corrupt the CSV round-trip,
            # so warn and skip instead of storing the garbled value.
            if '"' in value:
                print(
                    f"WARN  {path.name}: skipping {m.group(1)} — NOSTR fallback "
                    f"captured a quote, value not a clean #define string: {value!r}",
                    file=sys.stderr,
                )
                continue
            out[m.group(1)] = ("define", value)
    return out


def lang_code(path: Path) -> str:
    """intl_de.h → 'de'"""
    name = path.stem
    return name.replace("intl_", "")


def main():
    files = sorted(p for p in INTL_DIR.glob("intl_*.h") if p.name != "intl_template.h")
    if not files:
        print("No intl_*.h found", file=sys.stderr)
        return 1

    master = parse_intl_file(MASTER_FILE)
    if not master:
        print(f"Master {MASTER_FILE} parsed empty", file=sys.stderr)
        return 1

    # Build column order: en first, then others alphabetically
    langs = [MASTER_LANG]
    for path in files:
        code = lang_code(path)
        if code != MASTER_LANG and code not in langs:
            langs.append(code)

    rows_per_lang = {lang: parse_intl_file(INTL_DIR / f"intl_{lang}.h") for lang in langs}

    out_path = Path(__file__).resolve().parent / "master.csv"
    with open(out_path, "w", encoding="utf-8", newline="") as f:
        w = csv.writer(f, quoting=csv.QUOTE_ALL)
        w.writerow(["key", "type"] + langs)
        for key, (kind, _) in master.items():
            row = [key, kind]
            for lang in langs:
                vt = rows_per_lang[lang].get(key)
                if vt is None:
                    row.append("")
                else:
                    row.append(vt[1])
            w.writerow(row)

    print(f"Wrote {out_path} — {len(master)} keys × {len(langs)} langs")
    return 0


if __name__ == "__main__":
    sys.exit(main())
