# hibbes-Fork-Patches für AHT20+BMP280-Combo + lokale OTA

Dieser Fork von [opendata-stuttgart/sensors-software](https://github.com/opendata-stuttgart/sensors-software) erweitert die airrohr-Firmware um Support für **AHT20+BMP280-Combo-Boards** (z.B. GY-21P) als Ersatz für DHT22+BMP180, plus lokale OTA-Updates über das eigene WLAN.

Branch: `feature/aht20-support`
Basis: `upstream/master` (Stand NRZ-2024-135)
Status: live deployed auf einer Heimstation, Felderfahrung läuft

## Was die Patches machen

### 1. AHT20+BMP280 als HTU21D-Klon-Pattern

Adafruit_AHTX0-Lib (v2.0.5 exakt gepinnt) für I²C-Sensor auf Adresse `0x38`. Komplett strukturell nach HTU21D-Vorlage in der Firmware:

- `airrohr-firmware/airrohr-firmware.ino`: Include, cfg-Flag `cfg::aht20_read`, Globals, `fetchSensorAHT20()`, Send-Loop-Block, Web-UI-Checkbox, Status-Page, Debug-Output, Selected-Source-Logik, Combined-Sensor-Guard
- `airrohr-firmware/ext_def.h`: `AHT20_READ` und `AHT20_API_PIN`-Defaults
- `airrohr-firmware/airrohr-cfg.h(.py)`: Cfg-Enum + Persistence-Table + Generator-Sync
- `airrohr-firmware/html-content.h`: `SENSORS_AHT20`-PROGMEM
- `airrohr-firmware/intl_*.h` (alle 31): `INTL_AHT20`-Label

BMP280 wird vom airrohr-Bestand ohnehin erkannt (`bmx280_read`-Toggle, der zur Laufzeit zwischen BME280 und BMP280 unterscheidet); kein neuer Treiber-Code nötig dort.

### 2. Maskerade als DHT22+BMP180

OpenSenseMap und Sensor.Community kennen die `AHT20_*`/`BMP280_*`-JSON-Keys nicht (server-seitig hardgecodete Mapping-Tabellen). Damit die Werte in den öffentlichen Karten/Dashboards landen, emittieren `fetchSensorAHT20` und der BMP280-Branch von `fetchSensorBMX280` jetzt **bare DHT22+BMP180-Format**:

| Hardware | Emitted JSON-Keys | Pin |
|---|---|---|
| AHT20 (T+H) | `temperature`, `humidity` | 7 (DHT22-Slot) |
| BMP280 (T+P) | `BMP_temperature`, `BMP_pressure` | 3 (BMP180-Slot) |

Der Send-Loop für BMP280 nutzt `prefix = "BMP_"` statt `"BMP280_"`, damit der prefix-strip die Keys auf bare `temperature`/`pressure` für Sensor.Community Pin-3 reduziert.

**Trade-off**: ehrliche Sensor-Identität in den JSON-Schlüsseln verloren. Im Gegenzug funktionieren alle Push-Targets (SC, OSM, Madavi, Wunderground via fein2wunder, eigene Custom-API) ohne Server-seitige Mapping-Erweiterung.

### 3. Lokaler OTA-Server

ESP8266HTTPUpdateServer eingehängt am `/update`-Endpoint. Auth folgt `cfg::www_basicauth_enabled` — wenn die Web-UI Basic-Auth nutzt, dann auch /update; sonst offen für lokales Netz.

```bash
curl -F "image=@firmware.bin" http://<sensor-ip>/update
# (mit Auth: curl --user admin:pw -F image=@firmware.bin ...)
```

Voraussetzung: `auto_update=0` UND `use_beta=0` müssen in der Sensor-Cfg aus sein — sonst überschreibt der upstream-OTA-Pull innerhalb von Sekunden den eigenen Build.

### 4. Build-Anpassungen

| Was | Warum |
|---|---|
| `platform_version = espressif8266@4.2.1` (war 2.6.2) | 3-Jahre-Sprung an Toolchain: Arduino-Core 2.7.3 → 3.1.2, lwIP 2.1.2 → 2.1.3, BearSSL-TLS-Updates. lwIP-2.2-Stabilität, WLAN-Reconnect-Fixes. |
| `EspSoftwareSerial @ ^6.16.1` exakt gepinnt | v8 hat `perform_work()` umstrukturiert (airrohr ruft das aktiv im Loop für SDS/NPM-UART-Pump). Pin auf 6.x = Linker-symbol verfügbar. |
| `board_build.ldscript = eagle.flash.4m1m.ld` (war 4m3m.ld) | OTA braucht ~1MB Reserve, sonst ESP8266 `Update.begin()` → `ERROR[4]: Not Enough Space`. SPIFFS schrumpft von 3MB auf 1MB, das reicht airrohr easy. |
| `-D NO_GLOBAL_SERIAL=0` aus `build_flags` entfernt | ESP8266HTTPUpdateServer linkt gegen globalen `Serial`. Kostet ~5KB Flash. |
| Adafruit AHTX0 als Lib-Dep | Treiber für AHT20 (~5KB Flash) |

Build-Footprint nach allen Patches (nodemcuv2): **Flash 68.9% / RAM 43.6% / Binary 723 KB**.

### 5. /config.json GET+POST (Cfg-Backup/Restore)

Zwei neue HTTP-Endpoints für Cfg-Roundtrip ohne Web-UI-Form-Frickelei:

```bash
# Export aktuelle Cfg (Passwörter werden als leere Strings maskiert)
curl http://<sensor-ip>/config.json > backup.json

# Import (partial-Update möglich, leere Passwörter werden NICHT überschrieben)
curl -X POST -H "Content-Type: application/json" -d @backup.json http://<sensor-ip>/config.json
# Response: {"status":"saved, restarting","applied":N}
```

Iteriert die ConfigShape-Tabelle, gleiche Type-Logik wie HTML-Form-Handler. Praktisch für Backup vor SPIFFS-Wipe (z.B. ldscript-Wechsel) oder Roll-out auf mehrere Stationen ohne 60+ Form-Felder pro Sensor manuell auszufüllen.

### 6. Konfigurierbarer Auto-Update-Server (`cfg::ota_host`)

Neues Cfg-Feld erlaubt eigenen OTA-Server (statt hardcoded `firmware.sensor.community`). Im Web-UI Tab "Weitere Einstellungen" → Firmware unterhalb der Sprach-Auswahl als "Update-Host (leer = Default)" sichtbar. Wenn nicht-leer gesetzt, ersetzt es `FW_DOWNLOAD_HOST` in `fwDownloadStream()`.

Pfad-Struktur (`OTA_BASENAME/update/latest_<lang>.bin`) muss am eigenen Server gespiegelt sein. Port + SSL unverändert (443 + CA-cert-verify).

Anwendung: bei mehreren Custom-Build-Stationen (z.B. AHT20-Roll-out) eigener Hosting-Endpoint statt manueller curl-OTA pro Station.

## Quickstart (eigene Combo-Hardware in Betrieb)

```bash
# Build
cd ~/projects/sensors-software
~/.local/bin/pio run -d airrohr-firmware -e nodemcuv2

# Initial-Flash (USB)
~/.local/bin/pio run -d airrohr-firmware -e nodemcuv2 -t upload --upload-port /dev/ttyUSB0

# Beim ersten Boot mit 4m1m-Layout: Sensor in AP-Mode "Feinstaubsensor-{chipid}"
# AP-Pass: airrohrcfg (per ext_def.h:15)
# WLAN-Cfg über http://192.168.4.1/config eintragen + speichern.

# Künftige Updates: OTA über WLAN
curl -F "image=@airrohr-firmware/.pio/build/nodemcuv2/firmware.bin" http://<sensor-ip>/update
```

Hardware-Anschluss der GY-21P-Combo am NodeMCU (airrohr-Standard-I²C-Pinout):
- VIN/VCC → 3V3
- GND → GND
- SCL → **D4** (GPIO2)
- SDA → **D3** (GPIO0)

Web-UI nach Boot: AHT20 + BME280-Sammeloption (`bmx280_read`) aktivieren, DHT22 + BMP180 deaktivieren.

## Upstream-PR-Strategie

Die Patches sind in drei Klassen unterteilt:

| Klasse | Commits | Upstream-tauglich? |
|---|---|---|
| **Sauberer AHT20-Support** | bis `e5e9e65` (DBG_TXT_SEP-fix) | Ja — clean keys (`AHT20_temperature` etc.), kein Verbiegen |
| **Generische Verbesserungen** | `d8caccf` (/config.json), `5371def` (cfg::ota_host), `edda924` (platform-bump) | Ja — könnten als eigenständige PRs eingereicht werden, lösen häufige User-Pains |
| **Hibbes-Deployment-spezifisch** | `ba42bd2` (Maskerade) und `2fa5c19` (OTA-Server) | Nein — verändert öffentliches Verhalten, würde Upstream-Reviewer rejecten |

Wenn Upstream-PRs später angegangen werden, jede Klasse einzeln branchen vom passenden Vorgänger-Commit.

## Referenzen

- Spec: `docs/superpowers/specs/2026-05-09-aht20-bmp280-support-design.md`
- Implementation Plan: `docs/superpowers/plans/2026-05-09-aht20-bmp280-implementation.md`
- fein2wunder-Wrapper-Patches: [hibbes/fein2wunder@feature/aht20-support](https://github.com/hibbes/fein2wunder/tree/feature/aht20-support) (PHP-d.php-Endpoint, t=4/h=4/p=3-Modi für CSV-Archiv-Treue, OSM/SC laufen über Maskerade)
