#!/bin/bash
# Vergleicht Live-Station-Output gegen tests/golden/.
# Akzeptanz: identisch oder reine Whitespace-Diffs.
#
# Aufruf: ./diff-golden.sh <sensor-ip>   (default 192.168.1.251)

set -e
IP="${1:-192.168.1.251}"
GOLDEN_DIR="$(cd "$(dirname "$0")/golden" && pwd)"
TMPDIR="$(mktemp -d)"
trap 'rm -rf "$TMPDIR"' EXIT

PAGES=("" "config" "values" "status" "debug" "wifi" "removeConfig" "reset")
fails=0

# Dynamische Felder, die zwischen Runs natürlich variieren — strip vor Vergleich:
# - Build-Datum aus dem Header (z.B. "(May 12 2026)")
# - Live-Sensor-Werte (Zahlen in <td class='r'>-Spalten in /values + /status)
# - WiFi-Signal/Quality
# - Uptime/Age-Anzeigen
# - WebUI-Snapshot vor erstem Mess-Zyklus ("Noch ... Sekunden bis...")
normalize() {
  # perl für lazy-Quantifier in multi-line Pattern (sed kann das nicht)
  perl -0pe '
    s/\(([A-Z][a-z]{2} [ 0-9]{1,2} [0-9]{4})\)/(BUILDDATE)/g;
    s|<td class=.r.>.*?</td>|<td class=\x27r\x27>VALUE</td>|gs;
    s|<b style=.color:red.>Noch \d+ Sekunden[^<]*</b>|<b>FIRST-CYCLE-COUNTDOWN</b>|g;
    s|<b>\d+ Sekunden seit[^<]*</b>|<b>SINCE-LAST</b>|g;
    s|<pre id=.slog.[^>]*>.*?</pre>|<pre id=\x27slog\x27>LOG</pre>|gs;
  ' "$1"
}

for path in "${PAGES[@]}"; do
  name="${path:-root}"
  curl -s -o "$TMPDIR/$name.html" --max-time 8 "http://$IP/$path" || {
    echo "FAIL  /$path  curl error" >&2
    fails=$((fails + 1))
    continue
  }
  if cmp -s "$TMPDIR/$name.html" "$GOLDEN_DIR/$name.html"; then
    echo "OK    /$path  byte-identisch"
    continue
  fi
  normalize "$TMPDIR/$name.html" > "$TMPDIR/$name.norm"
  normalize "$GOLDEN_DIR/$name.html" > "$TMPDIR/$name.golden.norm"
  if cmp -s "$TMPDIR/$name.norm" "$TMPDIR/$name.golden.norm"; then
    echo "NORM  /$path  identisch nach Normalisierung (Build-Date / Live-Werte)"
  else
    ws_diff=$(diff -wB "$TMPDIR/$name.norm" "$TMPDIR/$name.golden.norm" || true)
    if [ -z "$ws_diff" ]; then
      echo "WS    /$path  nur whitespace (nach Normalisierung)"
    else
      echo "DIFF  /$path  inhaltlich abweichend"
      diff -u "$TMPDIR/$name.golden.norm" "$TMPDIR/$name.norm" | head -30 >&2
      fails=$((fails + 1))
    fi
  fi
done

if [ $fails -gt 0 ]; then
  echo "FAIL: $fails Pages weichen ab"
  exit 1
fi
echo "PASS: alle 8 Pages identisch (byte-identisch, whitespace-only oder nur dynamic content)"
