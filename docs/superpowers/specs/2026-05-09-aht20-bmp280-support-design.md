# Design: AHT20+BMP280 Combo-Sensor Support für airrohr

**Datum:** 2026-05-09
**Status:** Design approved, ready for implementation plan
**Owner:** Marek Czernohous (hibbes)

## Motivation

20 Schul-Feinstaubsensoren (Sensor.Community / airrohr-Kit, NodeMCU ESP8266 + SDS011 + DHT22) sollen von DHT22 auf AHT20+BMP280 umgerüstet werden. AHT20 hat keine signifikante Eigenerwärmung wie DHT22 und liefert stabilere Feuchte-Werte über Jahre.

Problem: airrohr-Firmware (`opendata-stuttgart/sensors-software`, Stand NRZ-2024-134-B5 / April 2024) hat **keinen** AHT10/AHT20/AHTx0-Support, weder in master, beta, noch in einem der 20 aktiven Forks. Die Combo-Boards (z.B. GY-21P) müssen also selbst eingearbeitet werden.

Datenfluss läuft nicht über api.sensor.community, sondern über einen eigenen PHP-Endpunkt (`hibbes/fein2wunder`), der nach Wunderground-PWS pusht und CSV archiviert. Damit fällt die typische "AHT20 muss als DHT22 maskieren weil API-Pin fehlt"-Politik weg, und wir können einen sauberen JSON-Vertrag wählen.

## Architektur

```
NodeMCU (airrohr-firmware @ hibbes/sensors-software)
   │  HTTP POST mit JSON
   │  Felder: SDS_P1, SDS_P2, AHT20_temperature, AHT20_humidity, BMP280_pressure
   ▼
fein2wunder /d.php (hibbes/fein2wunder)
   │  ?id=STATIONSID&key=APIKEY&t=4&h=4&p=3   (neue Modus-Codes)
   ├─► Wunderground PWS Upload
   └─► CSV-Archiv (data-SENSORID-DATUM.csv)
```

Zwei Repos, ein Patch-Set:

| Repo | Branch | Basis |
|---|---|---|
| `hibbes/sensors-software` | `feature/aht20-support` | upstream/master (NRZ-2024-134-B5) |
| `hibbes/fein2wunder` | `feature/aht20-support` | aktueller main |

## JSON-Vertrag (Firmware → fein2wunder)

Saubere neue Schlüssel, keine Maskerade unter BME280-Schlüsseln:

```json
{
  "esp8266id": "...",
  "software_version": "NRZ-2024-134-B5+aht20",
  "sensordatavalues": [
    {"value_type": "SDS_P1",            "value": "12.34"},
    {"value_type": "SDS_P2",            "value": "5.67"},
    {"value_type": "AHT20_temperature", "value": "22.45"},
    {"value_type": "AHT20_humidity",    "value": "48.20"},
    {"value_type": "BMP280_pressure",   "value": "100850"}
  ]
}
```

`BMP280_*`-Schlüssel werden bereits vom airrohr-Standard erzeugt, dort keine Änderung. Neu sind nur die `AHT20_*`-Schlüssel.

## Komponente A: sensors-software (Firmware)

**Treiber-Strategie:** AHTx0 als HTU21D-Klon. HTU21D ist ein 0x40 I²C-Sensor mit Temp+Humidity, in der airrohr-Firmware sauber als Feature-Flag mit Init/Read/Output integriert. Strukturell identisch zu dem was AHT20 braucht.

**Library:** `adafruit/Adafruit AHTX0` (Arduino-Lib, ~5KB Flash, fits ESP8266 alongside SDS011). API: `aht.begin()` / `aht.getEvent(&humidity, &temp)`.

**Touchpoints in `airrohr-firmware/airrohr-firmware.ino`** (~150 LOC):

- Include `<Adafruit_AHTX0.h>` und globale `Adafruit_AHTX0 aht20;`
- Cfg-Flag `cfg::aht20_read` (Default off)
- I²C-Init im Setup: `if (cfg::aht20_read) aht20.begin();`
- Sensor-Read im Main-Loop, gemittelt über `count_sends`
- JSON-Output unter `value_type` `AHT20_temperature` / `AHT20_humidity`
- Custom-API-Push: dieselben Werte
- HTML-UI: Checkbox "AHT20" neben der bestehenden BMP280-Checkbox
- Logging im Display-Loop (3 Zeilen Mittelwerte)

**Touchpoints in anderen Dateien:**

| Datei | Änderung |
|---|---|
| `airrohr-firmware/ext_def.h` | Default-Konstante für `cfg::aht20_read = false` |
| `airrohr-firmware/intl_de.h`, `intl_en.h` | `INTL_AHT20`-Label-Konstante handgepflegt |
| `airrohr-firmware/intl_*.h` (übrige ~28) | `INTL_AHT20` als generischer Fallback `"AHT20"` |
| `airrohr-firmware/platformio.ini` | Dependency `adafruit/Adafruit AHTX0@^2.0` |

**I²C-Adresse:** 0x38 (AHT20 fest). Keine Konflikte mit airrohr-Bestand:
- HTU21D 0x40
- SHT31 0x44
- DNMS 0x55
- SCD30 0x61
- SPS30 0x69
- BMP280 0x76 oder 0x77 (Auto-Fallback im Bestand)
- Display-OLED 0x3C
- AHT20 0x38 — **frei**

**BMP280-Coexistenz:** BMP280 ist bereits Standard im airrohr-Code. Der Combo-User aktiviert in der Web-UI zwei Checkboxes (AHT20 neu + BMP280 bestehend). BMP280 liefert weiterhin `BMP280_temperature` und `BMP280_pressure`, AHT20 liefert `AHT20_temperature` und `AHT20_humidity`. Beide Temp-Werte werden parallel gesendet, fein2wunder pickt sich das gewünschte über `t=`-Schalter. Stationen, die nur einen Combo-Sensor haben, schalten `t=4` (AHT20-Temp) und ignorieren BMP280-Temp.

## Komponente B: fein2wunder (PHP-Wrapper)

**Aktueller Stand:**

| Schalter | Werte | Sensor |
|---|---|---|
| `t=` | 1 / 2 / 3 | DHT22 / BMP180 / BME280 |
| `h=` | 1 / 3 | DHT22 / BME280 |
| `p=` | 1 / 2 | BMP180 / BME280 |

**Neue Modi:**

| Schalter | Neu | JSON-Quellfeld | Beschreibung |
|---|---|---|---|
| `t=4` | ✅ | `AHT20_temperature` | Temp aus AHT20 |
| `h=4` | ✅ | `AHT20_humidity` | Humidity aus AHT20 |
| `p=3` | ✅ | `BMP280_pressure` | Pressure aus BMP280 stand-alone |

**Touchpoints in `d.php`:**

- Switch-Case in der Sensor-Auswahl-Logik um drei neue Cases erweitert (`case 4` bei t/h, `case 3` bei p)
- Validierung: `t=4` ohne `AHT20_temperature` im Payload → klare Error-Antwort statt silent fail
- CSV-Header erweitern um `AHT20_temperature`, `AHT20_humidity`, `BMP280_pressure` (zusätzliche Spalten am Ende, alte Spalten bleiben für Rückwärts-Kompatibilität alter CSVs)

**Beispiel-URL für eine umgerüstete Station:**
```
/d.php?id=STATIONSID&key=APIKEY&alt=<Höhe ü. NN>&t=4&h=4&p=3
```

## Test- und Roll-out-Strategie

**Phase 1 — Build-Smoke (lokal, ohne Hardware)**
- `pio run -e nodemcuv2` läuft sauber
- Keine Hardware nötig, nur Compile-Validation

**Phase 2 — Hardware-Test auf einer Station**
- Eine der 20 Stationen wird Test-Kandidat (die mit Combo-Board auf dem Tisch)
- Verkabelung: 3V3, GND, D1=SCL, D2=SDA
- Firmware via OTA (`/update`) flashen
- Beide Checkboxes aktivieren (AHT20 + BMP280)
- fein2wunder mit `t=4&h=4&p=3` aufrufen
- 24h Plausibilitäts-Check: Sensor-Web-UI vs. Wunderground vs. CSV

**Phase 3 — Vergleichsmessung (optional, ~3 Tage)**
- Parallel zweite Station mit DHT22 daneben für Drift-Kalibrierung
- Optional, AHT20 ist gut spezifiziert; nur sinnvoll wenn historische Reihen lückenlos bleiben sollen

**Phase 4 — Roll-out auf die restlichen 19**
- Einmal `firmware.bin` aus PlatformIO bauen
- Auf alle `/update`-Endpoints pushen (Curl-Schleife über die Station-Hostnamen)
- DHT22 → Combo-Board manuell tauschen, eine pro Tag oder im Block
- Pro Station: t/h/p-Schalter im Custom-API-URL-Pfad auf `t=4&h=4&p=3` umsetzen (siehe Risiko "Custom-API-URL-Migration" für Roll-out-Mechanik)

**Phase 5 — Upstream-PR (Wochen später)**
- Nach 4-8 Wochen Felderfahrung sauberes PR-Branch gegen `opendata-stuttgart/sensors-software`
- Nur der Firmware-Teil; fein2wunder bleibt im Privat-Fork
- Forum-Vorab-Post auf forum.sensor.community zur Vermeidung von Bikeshed-Tod

## Risiken und Edge-Cases

- **I²C-Adressen-Kollision** — 0x38 ist im airrohr-Bestand unbenutzt, also sicher.
- **BMP280-Adresse 0x77 vs. 0x76** — airrohr hat bereits Auto-Fallback. GY-21P-Combo sitzt auf 0x76, kein Eingriff nötig.
- **Wunderground-PWS-Konsistenz** — AHT20-Temp ist trockene Sensor-Temp ohne Eigen-Heating-Offset. Erwarteter Sprung von 0.5–1°C nach unten gegenüber DHT22-Historie. Bei 20 Stationen über die Zeit eine sichtbare Konsistenz-Frage. Mitigation: in der Wunderground-Station-Beschreibung Sensor-Tausch und Datum dokumentieren.
- **OTA-Flash-Größe** — Adafruit_AHTX0 fügt ~5KB hinzu. ESP8266-OTA braucht doppelten Flash für sich selbst. Smoke-Test in Phase 1 wird das sichtbar machen, falls knapp.
- **Custom-API-URL-Migration** — die `t=`/`h=`/`p=`-Schalter in der Sensor-Web-UI sind pro Station gespeichert. Roll-out muss diese 20 URLs umsetzen. Skript-Pfad: HTTP POST gegen `/config.json` oder Bulk-Update via SPIFFS-Image-Reflash.
- **AHT20-Lib-Deps** — Adafruit_AHTX0 hängt von Adafruit_BusIO + Adafruit_Sensor ab. Beide sind ohnehin als BMP280-Lib-Dep schon präsent, also kein neuer Footprint.

## Out of Scope

- ESP32-Targets (airrohr-firmware hat ESP32-Support, aber unsere 20 Stationen sind alle ESP8266; ESP32-Variante kann später trivial nachgezogen werden, alle Code-Pfade sind bereits doppelt geführt)
- Andere AHT-Varianten (AHT10, AHT15, AHT21). Adafruit_AHTX0-Lib unterstützt sie zwar transparent, aber wir testen nur AHT20.
- Migration historischer DHT22-Reihen — die DHT22-Werte bleiben in den alten CSVs, die neuen AHT20-Werte landen ab Tausch-Datum in den neuen Spalten.
- Kalibrierungs-Korrektur-Faktoren in der Firmware (gibt's nicht in airrohr-Bestand, würde Inkonsistenz mit anderen Sensoren erzeugen).

## Erfolgs-Kriterien

1. Firmware kompiliert ohne Warnings/Errors gegen ESP8266-Target.
2. Eine Test-Station meldet 24h lang plausible AHT20+BMP280-Werte an fein2wunder.
3. fein2wunder routet korrekt: `t=4&h=4&p=3` → AHT20-T, AHT20-H, BMP280-P landen in Wunderground-PWS und CSV.
4. Roll-out auf 20 Stationen abgeschlossen ohne Datenverlust >2h pro Station.
5. (Späte Phase) Upstream-PR an opendata-stuttgart eingereicht und review-ready.
