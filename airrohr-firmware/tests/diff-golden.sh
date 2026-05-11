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

for path in "${PAGES[@]}"; do
  name="${path:-root}"
  curl -s -o "$TMPDIR/$name.html" --max-time 8 "http://$IP/$path" || {
    echo "FAIL  /$path  curl error" >&2
    fails=$((fails + 1))
    continue
  }
  if cmp -s "$TMPDIR/$name.html" "$GOLDEN_DIR/$name.html"; then
    echo "OK    /$path  byte-identisch"
  else
    ws_diff=$(diff -wB "$TMPDIR/$name.html" "$GOLDEN_DIR/$name.html" || true)
    if [ -z "$ws_diff" ]; then
      echo "WS    /$path  nur whitespace"
    else
      echo "DIFF  /$path  inhaltlich abweichend"
      diff -u "$GOLDEN_DIR/$name.html" "$TMPDIR/$name.html" | head -30 >&2
      fails=$((fails + 1))
    fi
  fi
done

if [ $fails -gt 0 ]; then
  echo "FAIL: $fails Pages weichen ab"
  exit 1
fi
echo "PASS: alle 8 Pages identisch (oder reine Whitespace-Diffs)"
