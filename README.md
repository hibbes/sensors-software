# airrohr-firmware (hibbes-Fork)

> Ein Fork der [airrohr-Firmware von OK Lab Stuttgart](https://github.com/opendata-stuttgart/sensors-software) für moderne Sensor-Hardware (AHT20+BMP280) und lokale Wartung statt Cloud-Auto-Update.

**Status:** Aktive Wartung außerhalb des Upstream-Pfads. Upstream zeigt seit April 2024 keine Releases mehr, dieser Fork pflegt selbständig Hardware-Support, OTA-Tooling und Build-Toolchain.

## Was ist anders gegenüber Upstream

Hardware:

- ✅ **AHT20+BMP280-Combo-Boards** (z.B. GY-21P) als modernen Ersatz für DHT22+BMP180. AHT20 hat keine signifikante Eigenerwärmung wie DHT22 und liefert stabile Feuchte-Werte über Jahre.
- ✅ Original-Sensoren (DHT22, BMP180, BME280, HTU21D, SHT3x, SCD30, DS18B20, SDS011, PMS, NPM, IPS) bleiben unterstützt.

Tooling:

- ✅ **Lokales OTA-Update** via `POST /update`-HTTP-Endpoint. Firmware-Update per `curl` aus dem eigenen Netz, ohne USB.
- ✅ **Cfg-Backup/Restore** via `GET/POST /config.json`. Passwörter werden im Export maskiert, Roundtrip-fähig.
- ✅ **Konfigurierbarer Auto-Update-Server** (`cfg::ota_host`). Wer eigenes Hosting hat, pullt von dort statt Upstream.

Build:

- ✅ **espressif8266-Platform 4.2.1** (statt upstream-Pin auf 2.6.2 von 2020). Arduino-Core 3.1.2, lwIP 2.1.3, BearSSL aktualisiert.
- ✅ **4m1m-ldscript** (1MB SPIFFS + 1MB OTA-Reserve) statt 4m3m (3MB SPIFFS, kein OTA).
- ✅ Adafruit AHTX0-Lib v2.0.5 als Treiber-Dep.

## Hardware-Stack (typische Konfiguration)

| Komponente | Pin am NodeMCU | Bemerkung |
|---|---|---|
| **NodeMCU V2/V3** (ESP8266) | — | Host-Mikrocontroller |
| **SDS011** Feinstaubsensor | TX→D1 (GPIO5), RX→D2 (GPIO4) | UART, 5V-Versorgung über VIN |
| **AHT20+BMP280-Combo** (GY-21P) | SDA→D3 (GPIO0), SCL→D4 (GPIO2), 3V3, GND | I²C, Adressen 0x38 (AHT20) + 0x76 (BMP280) |
| Optional: OLED-Display | SDA/SCL teilen sich mit Combo | I²C 0x3C |

Die airrohr-Standard-I²C-Pinbelegung ist **nicht** D1/D2 wie bei generischen Wemos-D1-mini-Setups, sondern **D3+D4**. D1+D2 sind für SDS-UART reserviert.

## Quickstart

### Build

```bash
git clone https://github.com/hibbes/sensors-software
cd sensors-software
~/.local/bin/pio run -d airrohr-firmware -e nodemcuv2
# Output: airrohr-firmware/.pio/build/nodemcuv2/firmware.bin (~723 KB)
```

PlatformIO via venv falls nicht system-weit verfügbar:

```bash
python3 -m venv ~/.venv-pio
~/.venv-pio/bin/pip install platformio
ln -s ~/.venv-pio/bin/pio ~/.local/bin/pio
```

### Initial-Flash (USB)

```bash
~/.local/bin/pio run -d airrohr-firmware -e nodemcuv2 -t upload --upload-port /dev/ttyUSB0
```

Beim ersten Boot mit dem 4m1m-Layout startet der Sensor im AP-Mode (SSID `Feinstaubsensor-{chipid}`, Pass `airrohrcfg`). Über http://192.168.4.1/config das WLAN konfigurieren und speichern.

Falls Sensor schon mit Upstream-Firmware lief und SPIFFS Cfg da ist: vorher unbedingt `auto_update=0` setzen, sonst wird der Custom-Build innerhalb von Sekunden vom Upstream-OTA überschrieben (siehe [Häufige Stolperfallen](#häufige-stolperfallen)).

### Künftige Updates (OTA)

```bash
cd ~/projects/sensors-software
~/.local/bin/pio run -d airrohr-firmware -e nodemcuv2
curl -F "image=@airrohr-firmware/.pio/build/nodemcuv2/firmware.bin" http://<sensor-ip>/update
```

Der Sensor flasht sich, rebootet, neue Firmware läuft. Dauer typischerweise 15-25s über WLAN.

## Web-UI

Erreichbar auf `http://<sensor-ip>/`. Standard-IP per DHCP, vorgesehene Endpoints:

| Endpoint | Zweck |
|---|---|
| `/` | Übersicht (aktuelle Werte, Sensor-IDs, Links) |
| `/config` | Konfigurations-Form (klassisches HTML) |
| `/config.json` | Cfg-Roundtrip als JSON (siehe unten) |
| `/values` | Aktuelle Sensor-Werte als HTML-Tabelle |
| `/status` | Diagnose-Counter (WLAN, Sensor-Errors, Push-Returns) |
| `/serial` | Live-Debug-Stream (mit `/debug?lvl=5` aktivierbar) |
| `/data.json` | Letzte Mess-Daten als JSON |
| `/metrics` | Prometheus-Format-Endpoint |
| `/update` | OTA-Flash (POST `firmware.bin` als multipart) |
| `/reset` | Soft-Reset (POST = trigger, GET = Confirm-Page) |
| `/removeConfig` | Factory-Reset (Achtung: löscht SPIFFS) |

Auth: alle Endpoints folgen `cfg::www_basicauth_enabled`. Wenn an, dann mit `www_username`/`www_password`. Wenn aus (Default), offen für lokales Netz.

## Cfg-Backup/Restore via JSON

```bash
# Export (Passwörter werden maskiert)
curl http://<sensor-ip>/config.json > backup.json

# Import (partial-Update, leere Passwörter werden NICHT überschrieben)
curl -X POST -H "Content-Type: application/json" \
     -d @backup.json \
     http://<sensor-ip>/config.json
# Response: {"status":"saved, restarting","applied":N}
```

Praktisch:

- Vor SPIFFS-Wipe (z.B. ldscript-Wechsel): Export speichern, nach Re-Flash Import
- Roll-out auf mehrere Stationen: einmal Master-Cfg setzen, JSON exportieren, auf alle anderen pushen
- Versions-Kontrolle der Cfg möglich (Diff zwei JSON-Files)

Felder: 74 Cfg-Keys (siehe `airrohr-cfg.h` für die ConfigShape-Tabelle).

## Konfigurierbarer Auto-Update-Server

Im Web-UI Tab "Weitere Einstellungen" → Firmware → "Update-Host (leer = Default)". Wenn nicht-leer gesetzt, ersetzt es `firmware.sensor.community` als OTA-Quelle. Pfad-Struktur (`OTA_BASENAME/update/latest_<lang>.bin`) muss am eigenen Server gespiegelt sein.

Anwendung: bei mehreren Custom-Build-Stationen eigenen Hosting-Endpoint nutzen statt manuellem curl-OTA pro Station, oder Custom-Firmware-Pflege ohne Upstream-Zwang.

## Push-Targets (Daten-Sinks)

Folgende Targets werden im Web-UI Tab "APIs" konfiguriert:

| Target | URL-Schema | Anmerkung |
|---|---|---|
| **sensor.community** | `https://api.sensor.community/v1/push-sensor-data/` | Public-Map. Akzeptiert Pin-7 (DHT22-Format) und Pin-3 (BMP180-Format). AHT20+BMP280 müssen wegen API-Hardcoding als DHT22+BMP180 maskieren (siehe [Hibbes-Patch-Doku](HIBBES_PATCH.md)) |
| **Madavi.de** | `https://api-rrd.madavi.de/data.php` | Visualisierungs-Service, akzeptiert dieselben Keys wie sensor.community |
| **OpenSenseMap** | `https://ingress.opensensemap.org/boxes/<id>/data?luftdaten=1` | senseBox-API mit Server-seitig hardgecodeter Mapping-Tabelle. AHT20-Maskerade als DHT22 ist nötig. |
| **Custom HTTP** | beliebig (z.B. eigener PHP-Endpoint) | Volle JSON-Payload, kein Format-Constraint |
| **InfluxDB** | beliebig | Native InfluxDB-Line-Protocol-Push |
| **Wunderground PWS** | per `Custom HTTP` über Wrapper wie [fein2wunder](https://github.com/hibbes/fein2wunder) | WU akzeptiert Wetter-Daten, AqPM* werden zwar mitgesendet aber nicht öffentlich angezeigt (PurpleAir-Partnership-Lock) |

## Push-Format und Maskerade

airrohr-Firmware emittiert pro Sensor JSON-Werte mit Sensor-Prefix (z.B. `BMP_temperature`, `BME280_humidity`, `HTU21D_temperature`). Beim Push an sensor.community-API wird der Prefix string-mäßig gestrippt, sodass die public-API kanonische Keys (`temperature`, `humidity`, `pressure`) sieht.

Für AHT20+BMP280 gibt es kein API-Pin auf den öffentlichen Diensten. Dieser Fork emittiert daher AHT20-Werte als bare `temperature`+`humidity` (Pin 7, identisch zu DHT22) und BMP280 als `BMP_temperature`+`BMP_pressure` (Pin 3, identisch zu BMP180). Damit sehen alle Push-Targets sauber strukturierte Daten, ohne Server-seitige Mapping-Erweiterung. Trade-off: die Hardware-Identität in den JSON-Schlüsseln ist verbogen.

Details siehe [HIBBES_PATCH.md](HIBBES_PATCH.md).

## Häufige Stolperfallen

**Auto-Update überschreibt Custom-Firmware.** Wer eigene Builds flasht, MUSS vor dem Flash `cfg::auto_update=0` und `cfg::use_beta=0` setzen. Sonst pullt der Upstream-OTA innerhalb von Sekunden nach Boot die offizielle Firmware und überschreibt den Custom-Build. Das `/status` zeigt dann wieder die Upstream-Version. Workflow: Cfg setzen → reboot → flash, oder Cfg per `/config.json` POSTen.

**SPIFFS-Wipe bei ldscript-Wechsel.** Der Wechsel von `eagle.flash.4m3m.ld` (Upstream-Default, kein OTA) auf `eagle.flash.4m1m.ld` (Custom-Default mit OTA-Reserve) ändert die SPIFFS-Adress-Range. Beim ersten Boot mit neuem Layout ist das alte SPIFFS unlesbar, der Sensor fällt in den AP-Mode. Lösung: vorher Cfg-Backup via `/config.json`, nach Re-Flash importieren. Plus WLAN-Cred manuell (Passwort kann nicht exportiert werden).

**WLAN-Pass im Web-UI ist immer leer.** Aus Security-Gründen rendert das HTML-Form Passwort-Felder immer leer, auch wenn ein Passwort gespeichert ist. Wer leere Passwort-Felder mitschickt, überschreibt das gespeicherte Passwort NICHT (Form-Handler überspringt empty-strings). Gleiche Konvention im `/config.json`-POST-Handler.

**SDS011-Warmup-Errors in den ersten Cycles.** Nach Boot zeigt `/status` typisch 1-3 SDS011-Errors während die Hardware sich aufwärmt (Fan + Laser brauchen ~15s, dann 1-2 Cycles bis valide UART-Frames durchkommen). Erst nach ~5 Min Stabilen-Betrieb sind die Counter aussagekräftig.

**Pin-Konflikt I²C ↔ SDS-UART.** Die airrohr-Firmware konfiguriert `Wire.begin()` auf D3=SDA und D4=SCL (siehe `ext_def.h:118-119`). D1+D2 sind exklusiv für `PM_SERIAL_RX/TX` reserviert. Wer Combo-Sensoren auf D1+D2 anschließt blockiert SDS und der I²C antwortet auf den falschen Pins.

## Build-Footprint

Stand der `feature/aht20-support`-Branch nach allen Patches (nodemcuv2-env):

| Metrik | Wert |
|---|---|
| Flash | 68.9% (719 KB von 1044 KB) |
| RAM | 43.6% (35700 B von 81920 B) |
| Binary-Größe | 723 KB |
| Build-Zeit (clean) | ~4-5 Min |

## Roadmap

Aktuelle Issue-Tracker-Sicht: https://github.com/hibbes/sensors-software/issues

| Tag | Bedeutung |
|---|---|
| `next` | Geplant für eine der nächsten Sessions |
| `backlog` | Im Tracker für später |
| `bug` | Klassische Bugs (WiFi-Reconnect, BMx280-Race, SDS-Warmup) |
| `refactor` | Strukturelle Verbesserungen (.ino aufsplitten, JSON-Builder, i18n) |
| `performance` | Speed/Memory/Flash-Optimierung (TLS-Cache, Async-Sends, -Os) |
| `enhancement` | Neue Features (SCD4x/SEN5x-Treiber, AQI-Display) |
| `build` | Toolchain, CI |

## Repo-Branches

| Branch | Status |
|---|---|
| `feature/aht20-support` | Aktive Entwicklung, deployed auf Heimstation |
| `master` | Aktuell auf Upstream-Stand (Mirror des opendata-stuttgart-Master) |

Der Plan ist, `feature/aht20-support` mittelfristig als neuen Default-Branch zu setzen (siehe [HIBBES_PATCH.md](HIBBES_PATCH.md) für die Branch-Strategie-Diskussion).

## Begleit-Dokumente

- [HIBBES_PATCH.md](HIBBES_PATCH.md): Detail-Doku der einzelnen Patches mit Commit-Verweis
- [airrohr-firmware/Readme.md](airrohr-firmware/Readme.md): Original-Upstream-Doku (Hardware-Bilder, generelles airrohr-Konzept)
- [docs/superpowers/specs/](docs/superpowers/specs/): Design-Spec der AHT20-Integration
- [docs/superpowers/plans/](docs/superpowers/plans/): Implementations-Plan

## Lizenz

- Original-airrohr-Firmware unter Apache 2.0 (siehe [airrohr-firmware/Readme.md](airrohr-firmware/Readme.md) für Details).
- Hibbes-Patches in diesem Fork werden unter derselben Lizenz weitergeführt.

## Kontakt

Issues: https://github.com/hibbes/sensors-software/issues

Pull Requests willkommen, aber dieser Fork ist nicht als offizielle airrohr-Distribution gemeint, sondern eine Privat-Maintenance für eine konkrete Heimstation und eventuelle künftige Citizen-Science-Roll-outs.
