# Plan-Triage 2026-07-19 (Nachzuegler)

Diese Repos fehlten in der Haupt-Triage vom selben Tag; der neue
Repo-Drift-Check hat sie am ersten Lauf gefunden. Verfahren identisch:
Klassifikation gegen den CODE, jedes DONE adversarial gegengeprueft.

## docs/superpowers/plans/2026-05-09-aht20-bmp280-implementation.md

**Urteil: PARTIAL** (Konfidenz high)

Plan zur Integration von AHT20+BMP280-Combo-Boards in die airrohr-Firmware plus fein2wunder-Erweiterung (t=4/h=4/p=3). Phase A (Firmware) und Phase B (fein2wunder) sind vollständig implementiert, gemerged und live deployt; eine Test-Station läuft seit 09.05.2026 produktiv mit Wunderground-Push. Offen ist nur der Roll-out auf die 19 weiteren Stationen; der Upstream-PR (C4) wurde am 10.05.2026 per Posture-Pivot bewusst aufgegeben (Upstream tot seit April 2024).

Belege: Phase A komplett: Commits 96cbae1 (platformio.ini: adafruit/Adafruit AHTX0@2.0.5, Zeile 63), 6f3d2de/72dcf39 (ext_def.h:250 AHT20_READ, airrohr-cfg.h:39 Config_aht20_read, :116 CFG_KEY_AHT20_READ, :193 Table-Eintrag), 6749266, 03dae79, a71e50f (fetchSensorAHT20), 0786e2b (Send-Loop), 12c3d6e (Web-UI-Checkbox), 23e351e (Status-Page), 30bf697 (Debug/Selected-Source/Guard); INTL_AHT20 in 31/31 intl_*.h; Branch feature/aht20-support ist Ancestor von HEAD (fix/issues-batch2); Code inzwischen weiterentwickelt nach airrohr-firmware/sensors/aht20.{cpp,h} (Commit 34b11a7) und web/pages/config.cpp. Phase B komplett in ~/projects/fein2wunder auf master: Commits 8c88e6b (tests/test-aht20.sh + tests/fixtures/payload-aht20.json, beide vorhanden), 4dac310 (d.php case 4/case 4/case 3 in Zeilen 91/105/119, CSV-Spalten AHT20_temp/AHT20_humidity/BMP280_pressure Zeile 274/294f), 04cf266 (README-Modi). Phase C1/C2: Live-Deployment 09.05.2026 dokumentiert (HIBBES_PATCH.md: "live deployed auf einer Heimstation, Felderfahrung läuft"; Station IAU617, WU_Response=success; Folge-Commits ba42bd2 Maskerade, 2fa5c19 OTA belegen Feldbetrieb). C4 aufgegeben: HIBBES_PATCH.md Zeile 110ff "Stand 2026-05-10 zielt er nicht mehr auf Upstream-Merge", Commit 1222903 Posture-Pivot. fein2wunder selbst am 10.05. archiviert (fcebdce) und durch built-in Wunderground-Push ersetzt (bbd4d06, closes #16).

Offen:
- Task C3: Roll-out auf die 19 weiteren Stationen (kein Bulk-OTA-Skript, keine Umbau-Belege gefunden; nur die eine Test-Station läuft). Tooling dafür existiert inzwischen (cfg::ota_host, /config.json-Backup/Restore).
- Task C4 (Upstream-PR an opendata-stuttgart): nicht offen, sondern bewusst verworfen (Posture-Pivot 2026-05-10, Upstream seit April 2024 inaktiv). Sollte im Status-Header als "aufgegeben" statt "offen" stehen.
- Hinweis: Phase B (fein2wunder) ist erledigt, aber seit 10.05.2026 durch den built-in Wunderground-Push der Firmware ersetzt; fein2wunder-Repo ist archiviert.
