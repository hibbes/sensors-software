# Refactor-Baseline (Issue #18)

Stand vor Beginn der HTML-Modularisierung, gemessen auf Branch `refactor/html-modularization` von `feature/aht20-support`.

## Build-Footprint nodemcuv2 (de-Default)

| Metrik | Wert |
|---|---|
| Build-Zeit | 70.7 s |
| RAM | 35 808 / 81 920 Bytes (43.7 %) |
| Flash | 726 619 / 1 044 464 Bytes (69.6 %) |
| `firmware.bin` | 726.6 KB |
| Zeilen `airrohr-firmware.ino` | 5445 |

## Live-Station 192.168.1.251

Golden HTML-Snapshots in `golden/` per `curl -s http://192.168.1.251/<page>`:

> **Platzhalter statt echter Daten:** Die Snapshots `config.html`/`debug.html`
> enthalten KEINE echten Geräte-Werte mehr. Gerätespezifische Felder sind durch
> klare Platzhalter ersetzt (`EXAMPLE-SSID`, `EXAMPLE-STATION`, `key=REDACTED`,
> senseboxid = lauter Nullen, `host_custom=example.com`). Beim Neu-Erfassen der
> Golden-Snapshots muss die Station mit eben diesen Platzhalter-Werten
> konfiguriert sein, sonst schlägt `diff-golden.sh` fehl (es vergleicht gegen den
> Live-Output). Der UI-Hilfetext „Station-ID (z.B. IAU617)" bleibt erhalten — er
> ist fest im Firmware-Template und kein Gerätewert.

| Page | Bytes | HTTP |
|---|---|---|
| `/` (root) | 1705 | 200 |
| `/config` | 18 844 | 200 |
| `/values` | 2318 | 200 |
| `/status` | 2835 | 200 |
| `/debug` | 2161 | 200 |
| `/wifi` | 34 | 200 (kein Scan aktiv) |
| `/removeConfig` | 1523 | 200 |
| `/reset` | 1510 | 200 |

## Akzeptanz nach Refactor

Bei laufender Station: `./diff-golden.sh 192.168.1.251` muss „PASS: alle 8 Pages identisch (oder reine Whitespace-Diffs)" liefern. Bei Build-only: Flash ≤ 70.5 %, RAM ≤ 44.5 % als Korridor.
