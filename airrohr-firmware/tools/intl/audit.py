#!/usr/bin/env python3
"""
Auditiert die 31 intl_<lang>.h-Files gegen intl_en.h (Master).

Findet:
- Fehlende INTL_*-Keys pro Sprache
- Extra INTL_*-Keys, die nicht im Master stehen
- Mixed Patterns (#define INTL_X vs. const char INTL_X[] PROGMEM)
- Drift in der Reihenfolge der Keys

Issue #18 Phase F / Issue #9 i18n-Konsolidierung.
"""

from pathlib import Path
import re
import sys
from collections import OrderedDict

INTL_DIR = Path(__file__).resolve().parent.parent.parent
MASTER = INTL_DIR / "intl_en.h"

DEFINE_RE = re.compile(r"^\s*#define\s+(INTL_\w+)\s+(.+?)\s*$")
CONST_RE = re.compile(
    r"^\s*const\s+char\s+(INTL_\w+)\[\]\s+PROGMEM\s*=\s*(.+?);\s*$",
    re.DOTALL,
)


def parse(path: Path):
    """Return (defines, consts) — both OrderedDict of key→raw_value_or_literal."""
    defines = OrderedDict()
    consts = OrderedDict()
    text = path.read_text(encoding="utf-8")
    # Drop the INTL_DEFINE_VARIABLES guards so we see all definitions
    text = re.sub(r"^#ifdef\s+INTL_DEFINE_VARIABLES.*?$", "", text, flags=re.MULTILINE)
    text = re.sub(r"^#endif\s*//\s*INTL_DEFINE_VARIABLES.*?$", "", text, flags=re.MULTILINE)

    # Collapse backslash-continuations
    text = text.replace("\\\n", "")
    for line in text.splitlines():
        m = DEFINE_RE.match(line)
        if m:
            defines[m.group(1)] = m.group(2)
            continue
        m = CONST_RE.match(line)
        if m:
            consts[m.group(1)] = m.group(2)
    return defines, consts


def main():
    master_def, master_const = parse(MASTER)
    master_keys = OrderedDict.fromkeys(list(master_def) + list(master_const))
    print(f"Master ({MASTER.name}): {len(master_def)} #defines + {len(master_const)} const-char = {len(master_keys)} keys")
    print()

    rc = 0
    for path in sorted(INTL_DIR.glob("intl_*.h")):
        if path.name in ("intl_template.h",):
            continue
        lang_def, lang_const = parse(path)
        lang_keys = OrderedDict.fromkeys(list(lang_def) + list(lang_const))
        missing = [k for k in master_keys if k not in lang_keys]
        extra = [k for k in lang_keys if k not in master_keys]
        define_in_const_master = [k for k in master_def if k in lang_const]
        const_in_define_master = [k for k in master_const if k in lang_def]

        n_def = len(lang_def)
        n_const = len(lang_const)
        ok = not missing and not extra and not define_in_const_master and not const_in_define_master
        status = "OK " if ok else "DRIFT"
        print(f"{status}  {path.name:<22}  #define={n_def}  const={n_const}  missing={len(missing)}  extra={len(extra)}")
        if missing:
            print(f"        missing: {', '.join(missing[:6])}{' ...' if len(missing) > 6 else ''}")
        if extra:
            print(f"        extra:   {', '.join(extra[:6])}{' ...' if len(extra) > 6 else ''}")
        if define_in_const_master:
            print(f"        mixed (master #define → here const): {', '.join(define_in_const_master[:6])}")
        if const_in_define_master:
            print(f"        mixed (master const → here #define): {', '.join(const_in_define_master[:6])}")
        if not ok:
            rc = 1
    return rc


if __name__ == "__main__":
    sys.exit(main())
