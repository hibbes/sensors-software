# AHT20+BMP280 Combo-Sensor Support — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** AHT20+BMP280-Combo-Boards (z.B. GY-21P) als DHT22-Ersatz in airrohr-Firmware integrieren, fein2wunder-Wrapper passend erweitern, sodass 20 Schul-Stationen auf saubere Sensoren umgerüstet werden können.

**Architecture:** AHT20 als HTU21D-Klon (gleiches I²C-T+H-Sensor-Pattern, andere Adresse 0x38). BMP280-Support ist im airrohr-Bestand bereits da, wird mitbenutzt. Eigene JSON-Schlüssel `AHT20_temperature`/`AHT20_humidity`/`BMP280_pressure`. fein2wunder bekommt neue Modus-Codes `t=4`, `h=4`, `p=3`.

**Tech Stack:** Arduino/PlatformIO für ESP8266, Adafruit AHTX0-Library v2.x, PHP 8 für fein2wunder, kein neues Test-Framework (Mock-JSON-curl-Tests genügen).

**Spec:** `docs/superpowers/specs/2026-05-09-aht20-bmp280-support-design.md`

**Repos:**
- `~/projects/sensors-software` — Branch `feature/aht20-support` (bereits angelegt + push)
- `~/projects/fein2wunder` — Branch `feature/aht20-support` muss in Task B0 angelegt werden

---

## File Structure

**sensors-software (Firmware-Patch):**

| Datei | Verantwortlichkeit | Touch |
|---|---|---|
| `airrohr-firmware/platformio.ini` | Build-Config + Lib-Deps | +1 dep line |
| `airrohr-firmware/ext_def.h` | Default-Konstanten der Cfg-Werte | +3 lines |
| `airrohr-firmware/airrohr-cfg.h` | Config-Enum + Persistence-Table | +3 lines |
| `airrohr-firmware/html-content.h` | PROGMEM Sensor-Name-String | +1 line |
| `airrohr-firmware/intl_de.h`, `intl_en.h` | Lokalisierte Labels | +1 line each |
| `airrohr-firmware/intl_*.h` (übrige 28) | Fallback-Labels | +1 line each |
| `airrohr-firmware/airrohr-firmware.ino` | Treiber-Plumbing, Init, Read, JSON, UI | ~30 Edits, ~150 LOC total |

**fein2wunder (PHP-Wrapper):**

| Datei | Verantwortlichkeit | Touch |
|---|---|---|
| `d.php` | URL-Param-Switch + CSV-Output | 3 Switch-Cases + Header + Row |
| `README.md` | Dokumentation der neuen Modi | Tabellen erweitern |
| `tests/test-aht20.sh` | Mock-JSON-Smoke-Test (neu) | new file |

---

# Phase A: Firmware (sensors-software)

## Task A0: Vorbereitung — Pre-Flight-Check

**Files:** none (Verification only)

- [ ] **Step 1: Branch und Working-Dir verifizieren**

```bash
cd ~/projects/sensors-software && git status && git branch --show-current
```

Expected: `feature/aht20-support`, working tree clean.

- [ ] **Step 2: PlatformIO-Verfügbarkeit prüfen**

```bash
which pio && pio --version
```

Expected: `pio` im PATH, Version ≥ 6.x. Falls fehlt, `pip install -U platformio` (User-Entscheidung).

- [ ] **Step 3: Notiz zum Build-Target**

Der ESP8266-Build-Env in `airrohr-firmware/platformio.ini` heißt `nodemcuv2`. Alle späteren Smoke-Builds nutzen `pio run -d airrohr-firmware -e nodemcuv2`.

---

## Task A1: Adafruit AHTX0 Library als Dependency

**Files:**
- Modify: `airrohr-firmware/platformio.ini`

- [ ] **Step 1: Aktuellen Lib-Block lesen**

```bash
command grep -n "^lib_deps\|adafruit" airrohr-firmware/platformio.ini | head -30
```

Notiere die `lib_deps` Sektion (in der Regel mehrzeilig).

- [ ] **Step 2: Adafruit_AHTX0 als Dependency eintragen**

In `airrohr-firmware/platformio.ini` in der gemeinsamen `[env]`- oder `[common]`-Sektion (dort wo auch `adafruit/Adafruit BMP280 Library` und `adafruit/Adafruit HTU21DF Library` stehen) hinzufügen:

```ini
    adafruit/Adafruit AHTX0 @ ^2.0.5
```

(Format identisch zu den umgebenden Lib-Lines.)

- [ ] **Step 3: Smoke-Build mit nur Dep-Change**

```bash
cd ~/projects/sensors-software && pio run -d airrohr-firmware -e nodemcuv2 2>&1 | tail -40
```

Expected: Build läuft durch (keine Code-Änderung). Bei Fehler "library not found" → Library-Spezifikator anpassen.

- [ ] **Step 4: Commit**

```bash
git add airrohr-firmware/platformio.ini
git commit -m "build: Adafruit AHTX0 dependency für AHT20-Support"
```

---

## Task A2: Default-Konstanten in ext_def.h

**Files:**
- Modify: `airrohr-firmware/ext_def.h:233` (nach HTU21D_API_PIN Block einfügen)

- [ ] **Step 1: Aktuelle HTU21D-Defines anschauen**

```bash
sed -n '230,238p' airrohr-firmware/ext_def.h
```

Expected:
```c
// HTU21D, temperature, humidity
#define HTU21D_READ 0
#define HTU21D_API_PIN 7
```

- [ ] **Step 2: AHT20-Defines unmittelbar nach HTU21D-Block einfügen**

In `airrohr-firmware/ext_def.h` direkt nach Zeile 233 (`#define HTU21D_API_PIN 7`):

```c
// AHT20, temperature, humidity (DHT22-Ersatz, oft in Combo mit BMP280)
#define AHT20_READ 0
#define AHT20_API_PIN 7
```

(API_PIN=7 ist identisch zu HTU21D — derselbe sensor.community-Slot. Fein2wunder ignoriert es ohnehin, aber wenn jemand später auch zu sensor.community pusht, hat es einen Pin.)

- [ ] **Step 3: Compile-Check**

```bash
pio run -d airrohr-firmware -e nodemcuv2 2>&1 | tail -20
```

Expected: Build OK (nur Defines, kein Code-Verbrauch).

- [ ] **Step 4: Commit**

```bash
git add airrohr-firmware/ext_def.h
git commit -m "feat: AHT20_READ und AHT20_API_PIN Defaults"
```

---

## Task A3: Cfg-Enum + Persistence in airrohr-cfg.h

**Files:**
- Modify: `airrohr-firmware/airrohr-cfg.h:37` (Enum-Einträge), `:109` (Key-String), `:181` (Table)

- [ ] **Step 1: Aktuellen HTU21D-Eintrag im Enum finden**

```bash
sed -n '30,50p' airrohr-firmware/airrohr-cfg.h
```

- [ ] **Step 2: AHT20-Enum-Eintrag direkt nach `Config_htu21d_read` einfügen**

In `airrohr-firmware/airrohr-cfg.h:37` nach `Config_htu21d_read,`:

```c
	Config_aht20_read,
```

- [ ] **Step 3: Cfg-Key-String einfügen**

In `airrohr-firmware/airrohr-cfg.h:109` (nach der HTU21D-Zeile):

```c
static constexpr char CFG_KEY_AHT20_READ[] PROGMEM = "aht20_read";
```

- [ ] **Step 4: Cfg-Table-Eintrag einfügen**

In `airrohr-firmware/airrohr-cfg.h:181` (direkt nach HTU21D-Zeile):

```c
	{ Config_Type_Bool, 0, CFG_KEY_AHT20_READ, &cfg::aht20_read },
```

- [ ] **Step 5: Compile-Check (erwarte Fehler — `cfg::aht20_read` noch nicht deklariert)**

```bash
pio run -d airrohr-firmware -e nodemcuv2 2>&1 | tail -20
```

Expected: Compile-Error `'aht20_read' is not a member of 'cfg'`. Das ist OK — wird in Task A4 behoben.

- [ ] **Step 6: Commit**

```bash
git add airrohr-firmware/airrohr-cfg.h
git commit -m "feat: cfg::aht20_read Enum, Key, Table-Eintrag"
```

---

## Task A4: Sensor-Name + i18n-Konstanten

**Files:**
- Modify: `airrohr-firmware/html-content.h:37` (PROGMEM-String)
- Modify: `airrohr-firmware/intl_de.h:32`, `intl_en.h:32` (handgepflegt)
- Modify: `airrohr-firmware/intl_bg.h`, `intl_br.h`, `intl_cn.h`, `intl_cz.h`, `intl_dk.h`, `intl_ee.h`, `intl_es.h`, `intl_fi.h`, `intl_fr.h`, `intl_gr.h`, `intl_hr.h`, `intl_hu.h`, `intl_it.h`, `intl_jp.h`, `intl_lt.h`, `intl_lu.h`, `intl_lv.h`, `intl_nl.h`, `intl_pl.h`, `intl_pt.h`, `intl_ro.h`, `intl_rs.h`, `intl_ru.h`, `intl_se.h`, `intl_si.h`, `intl_sk.h`, `intl_tr.h`, `intl_ua.h` (Fallback)
- Modify: `airrohr-firmware/intl_template.h` (Template aktualisieren)

- [ ] **Step 1: Aktuelles HTU21D-Pattern in html-content.h anschauen**

```bash
sed -n '35,40p' airrohr-firmware/html-content.h
```

Expected: `const char SENSORS_HTU21D[] PROGMEM = "HTU21D";`

- [ ] **Step 2: SENSORS_AHT20 PROGMEM-String einfügen**

In `airrohr-firmware/html-content.h:37` (direkt nach HTU21D-Zeile):

```c
const char SENSORS_AHT20[] PROGMEM = "AHT20";
```

- [ ] **Step 3: INTL_AHT20 in intl_de.h einfügen**

In `airrohr-firmware/intl_de.h:32` (direkt nach INTL_HTU21D-Zeile):

```c
const char INTL_AHT20[] PROGMEM = "AHT20 ({t}, {h})";
```

- [ ] **Step 4: INTL_AHT20 in intl_en.h einfügen**

In `airrohr-firmware/intl_en.h:32`:

```c
const char INTL_AHT20[] PROGMEM = "AHT20 ({t}, {h})";
```

- [ ] **Step 5: INTL_AHT20 in allen übrigen 28 Sprach-Files als generischer Fallback**

Bash-Helfer für die Bulk-Edit (verwendet HTU21D-Zeile als Anker):

```bash
for f in airrohr-firmware/intl_bg.h airrohr-firmware/intl_br.h airrohr-firmware/intl_cn.h \
         airrohr-firmware/intl_cz.h airrohr-firmware/intl_dk.h airrohr-firmware/intl_ee.h \
         airrohr-firmware/intl_es.h airrohr-firmware/intl_fi.h airrohr-firmware/intl_fr.h \
         airrohr-firmware/intl_gr.h airrohr-firmware/intl_hr.h airrohr-firmware/intl_hu.h \
         airrohr-firmware/intl_it.h airrohr-firmware/intl_jp.h airrohr-firmware/intl_lt.h \
         airrohr-firmware/intl_lu.h airrohr-firmware/intl_lv.h airrohr-firmware/intl_nl.h \
         airrohr-firmware/intl_pl.h airrohr-firmware/intl_pt.h airrohr-firmware/intl_ro.h \
         airrohr-firmware/intl_rs.h airrohr-firmware/intl_ru.h airrohr-firmware/intl_se.h \
         airrohr-firmware/intl_si.h airrohr-firmware/intl_sk.h airrohr-firmware/intl_tr.h \
         airrohr-firmware/intl_ua.h airrohr-firmware/intl_template.h; do
  if ! command grep -q "INTL_AHT20" "$f"; then
    sed -i '/^const char INTL_HTU21D\[\] PROGMEM/a const char INTL_AHT20[] PROGMEM = "AHT20 ({t}, {h})";' "$f"
  fi
done
```

(Achtung: das Snippet nutzt `sed -i`. Bei jeder Datei wird die neue Zeile direkt nach der HTU21D-Zeile eingefügt. Falls eine Datei kein HTU21D hat, schlägt das fehl — das wäre ein Fehler in der Datei selbst und müsste manuell korrigiert werden.)

- [ ] **Step 6: Verifikation, dass alle 30 Dateien INTL_AHT20 haben**

```bash
command grep -l "INTL_AHT20" airrohr-firmware/intl_*.h | wc -l
```

Expected: `31` (28 Sprachfiles + intl_de.h + intl_en.h + intl_template.h). Bei einem niedrigeren Wert: `command grep -L "INTL_AHT20" airrohr-firmware/intl_*.h` zeigt die Lücken.

- [ ] **Step 7: Compile-Check**

```bash
pio run -d airrohr-firmware -e nodemcuv2 2>&1 | tail -20
```

Expected: weiterhin Compile-Error wegen fehlendem `cfg::aht20_read` (Task A5 fixt das).

- [ ] **Step 8: Commit**

```bash
git add airrohr-firmware/html-content.h airrohr-firmware/intl_*.h
git commit -m "feat: SENSORS_AHT20 + INTL_AHT20 Labels (alle Sprachen)"
```

---

## Task A5: Driver-Plumbing in airrohr-firmware.ino

**Files:**
- Modify: `airrohr-firmware/airrohr-firmware.ino:107` (include)
- Modify: `airrohr-firmware/airrohr-firmware.ino:163` (cfg-flag declaration)
- Modify: `airrohr-firmware/airrohr-firmware.ino:254` (init-failed flag)
- Modify: `airrohr-firmware/airrohr-firmware.ino:325` (Adafruit_AHTX0 instance)
- Modify: `airrohr-firmware/airrohr-firmware.ino:458` (last-value globals)

- [ ] **Step 1: Include-Zeile direkt nach `#include <Adafruit_HTU21DF.h>` einfügen**

Suche `#include <Adafruit_HTU21DF.h>` (Zeile ~106) und füge direkt darunter:

```cpp
#include <Adafruit_AHTX0.h>
```

- [ ] **Step 2: cfg-Flag direkt nach `bool htu21d_read = HTU21D_READ;` einfügen**

In Zeile ~163, direkt nach `bool htu21d_read = HTU21D_READ;`:

```cpp
	bool aht20_read = AHT20_READ;
```

(Beachte: Tab-Einrückung wie umgebende Zeilen!)

- [ ] **Step 3: Init-failed-Flag direkt nach `htu21d_init_failed` einfügen**

In Zeile ~254, direkt nach `bool htu21d_init_failed = false;`:

```cpp
bool aht20_init_failed = false;
```

- [ ] **Step 4: Adafruit_AHTX0-Instance direkt nach `Adafruit_HTU21DF htu21d;` einfügen**

In Zeile ~325, direkt nach `Adafruit_HTU21DF htu21d;`:

```cpp
/*****************************************************************
 * AHT20 declaration                                             *
 *****************************************************************/
Adafruit_AHTX0 aht20;
```

- [ ] **Step 5: Last-Value-Globals direkt nach `last_value_HTU21D_H` einfügen**

In Zeile ~458, direkt nach `float last_value_HTU21D_H = -1.0;`:

```cpp
float last_value_AHT20_T = -128.0;
float last_value_AHT20_H = -1.0;
```

- [ ] **Step 6: Compile-Check**

```bash
pio run -d airrohr-firmware -e nodemcuv2 2>&1 | tail -30
```

Expected: build OK. `cfg::aht20_read` wird jetzt erkannt, Variablen sind deklariert, aber noch nirgends benutzt → Warnings für ungenutzte Variablen sind OK in diesem Zwischenschritt.

- [ ] **Step 7: Commit**

```bash
git add airrohr-firmware/airrohr-firmware.ino
git commit -m "feat: AHT20 Treiber-Plumbing (Include, cfg-Flag, Globals)"
```

---

## Task A6: Setup-Init für AHT20

**Files:**
- Modify: `airrohr-firmware/airrohr-firmware.ino:5640` (nach HTU21D-Setup-Block)

- [ ] **Step 1: HTU21D-Setup-Block als Vorlage anschauen**

```bash
sed -n '5630,5645p' airrohr-firmware/airrohr-firmware.ino
```

Erwartete Form:
```cpp
	if (cfg::htu21d_read)
	{
		debug_outln_info(F("Read HTU21D..."));
		// HTU21D Wire.begin(D3,D4); .. on NodeMCU 1.0
		// HTU21D Wire.begin();.. on WeMos D1 mini
		if (!htu21d.begin() && htu21d.readHumidity() < 1.0f)
		{
			debug_outln_error(F("Check HTU21D wiring"));
			htu21d_init_failed = true;
		}
	}
```

- [ ] **Step 2: AHT20-Setup-Block direkt nach HTU21D-Block einfügen**

Direkt nach der schließenden `}` des HTU21D-Setup-Blocks (~Zeile 5642):

```cpp
	if (cfg::aht20_read)
	{
		debug_outln_info(F("Read AHT20..."));
		if (!aht20.begin())
		{
			debug_outln_error(F("Check AHT20 wiring"));
			aht20_init_failed = true;
		}
	}
```

(`aht20.begin()` gibt `true` zurück, wenn der I²C-Sensor unter 0x38 erreichbar ist. Anders als HTU21D braucht es keinen extra-Humidity-Plausibilitätstest, weil die Adafruit-Lib die Init-Sequenz ehrlich validiert.)

- [ ] **Step 3: Compile-Check**

```bash
pio run -d airrohr-firmware -e nodemcuv2 2>&1 | tail -20
```

Expected: Build OK.

- [ ] **Step 4: Commit**

```bash
git add airrohr-firmware/airrohr-firmware.ino
git commit -m "feat: AHT20 Setup-Init mit Wiring-Diagnose"
```

---

## Task A7: fetchSensorAHT20-Funktion

**Files:**
- Modify: `airrohr-firmware/airrohr-firmware.ino:3306` (nach fetchSensorHTU21D)

- [ ] **Step 1: fetchSensorHTU21D als Vorlage anschauen**

```bash
sed -n '3283,3308p' airrohr-firmware/airrohr-firmware.ino
```

- [ ] **Step 2: fetchSensorAHT20 direkt nach fetchSensorHTU21D einfügen**

Direkt nach der schließenden `}` von `fetchSensorHTU21D` (~Zeile 3306):

```cpp
/*****************************************************************
 * read AHT20 sensor values                                      *
 *****************************************************************/
static void fetchSensorAHT20(String &s)
{
	debug_outln_verbose(FPSTR(DBG_TXT_START_READING), FPSTR(SENSORS_AHT20));

	sensors_event_t humidity_event;
	sensors_event_t temp_event;
	if (!aht20.getEvent(&humidity_event, &temp_event))
	{
		last_value_AHT20_T = -128.0;
		last_value_AHT20_H = -1.0;
		debug_outln_error(F("AHT20 read failed"));
	}
	else
	{
		last_value_AHT20_T = temp_event.temperature;
		last_value_AHT20_H = humidity_event.relative_humidity;
		add_Value2Json(s, F("AHT20_temperature"), FPSTR(DBG_TXT_TEMPERATURE), last_value_AHT20_T);
		add_Value2Json(s, F("AHT20_humidity"), FPSTR(DBG_TXT_HUMIDITY), last_value_AHT20_H);
	}

	debug_outln_verbose(FPSTR(DBG_TXT_END_READING), FPSTR(SENSORS_AHT20));
}
```

- [ ] **Step 3: Compile-Check**

```bash
pio run -d airrohr-firmware -e nodemcuv2 2>&1 | tail -20
```

Expected: Build OK. Funktion ist definiert, aber noch nicht aufgerufen → "defined but not used"-Warning ist akzeptabel.

- [ ] **Step 4: Commit**

```bash
git add airrohr-firmware/airrohr-firmware.ino
git commit -m "feat: fetchSensorAHT20 Read-Funktion"
```

---

## Task A8: Send-Loop-Integration

**Files:**
- Modify: `airrohr-firmware/airrohr-firmware.ino:6176` (nach HTU21D-Send-Block)

- [ ] **Step 1: HTU21D-Send-Block als Vorlage anschauen**

```bash
sed -n '6168,6180p' airrohr-firmware/airrohr-firmware.ino
```

Erwartet:
```cpp
		if (cfg::htu21d_read && (!htu21d_init_failed))
		{
			// getting temperature and humidity (optional)
			fetchSensorHTU21D(result);
			data += result;
			sum_send_time += sendSensorCommunity(result, HTU21D_API_PIN, FPSTR(SENSORS_HTU21D), "HTU21D_");
			result = emptyString;
		}
```

- [ ] **Step 2: AHT20-Send-Block direkt nach HTU21D-Block einfügen**

Direkt nach der schließenden `}` des HTU21D-Send-Blocks (~Zeile 6177):

```cpp
		if (cfg::aht20_read && (!aht20_init_failed))
		{
			// getting temperature and humidity from AHT20
			fetchSensorAHT20(result);
			data += result;
			sum_send_time += sendSensorCommunity(result, AHT20_API_PIN, FPSTR(SENSORS_AHT20), "AHT20_");
			result = emptyString;
		}
```

- [ ] **Step 3: Compile-Check**

```bash
pio run -d airrohr-firmware -e nodemcuv2 2>&1 | tail -30
```

Expected: Build OK, jetzt ohne "unused"-Warnings.

- [ ] **Step 4: Commit**

```bash
git add airrohr-firmware/airrohr-firmware.ino
git commit -m "feat: AHT20 in Send-Loop integriert (JSON-Output, Custom-API)"
```

---

## Task A9: Web-UI Form-Checkbox

**Files:**
- Modify: `airrohr-firmware/airrohr-firmware.ino:1764` (nach HTU21D-Checkbox)

- [ ] **Step 1: HTU21D-Checkbox als Vorlage**

```bash
sed -n '1760,1770p' airrohr-firmware/airrohr-firmware.ino
```

Erwartet: `add_form_checkbox_sensor(Config_htu21d_read, FPSTR(INTL_HTU21D));`

- [ ] **Step 2: AHT20-Checkbox direkt darunter einfügen**

In Zeile ~1765:

```cpp
	add_form_checkbox_sensor(Config_aht20_read, FPSTR(INTL_AHT20));
```

- [ ] **Step 3: Compile-Check**

```bash
pio run -d airrohr-firmware -e nodemcuv2 2>&1 | tail -20
```

Expected: Build OK.

- [ ] **Step 4: Commit**

```bash
git add airrohr-firmware/airrohr-firmware.ino
git commit -m "feat: AHT20-Checkbox in Sensor-Web-UI"
```

---

## Task A10: Status-Page-Anzeige

**Files:**
- Modify: `airrohr-firmware/airrohr-firmware.ino:2200` (nach HTU21D-Status-Block)

- [ ] **Step 1: HTU21D-Status-Block als Vorlage**

```bash
sed -n '2193,2205p' airrohr-firmware/airrohr-firmware.ino
```

- [ ] **Step 2: AHT20-Status-Block direkt nach HTU21D-Block einfügen**

Direkt nach der schließenden `}` des HTU21D-Status-Blocks (~Zeile 2201):

```cpp
	if (cfg::aht20_read)
	{
		add_table_t_value(FPSTR(SENSORS_AHT20), FPSTR(INTL_TEMPERATURE), last_value_AHT20_T);
		add_table_h_value(FPSTR(SENSORS_AHT20), FPSTR(INTL_HUMIDITY), last_value_AHT20_H);
		dew_point_temp = dew_point(last_value_AHT20_T, last_value_AHT20_H);
		add_table_value(FPSTR(SENSORS_AHT20), FPSTR(INTL_DEW_POINT), isnan(dew_point_temp) ? "-" : String(dew_point_temp, 1), unit_T);
		page_content += FPSTR(EMPTY_ROW);
	}
```

(Achtung: prüfe ob HTU21D-Block einen `EMPTY_ROW`-Aufruf hat oder nicht. Gleiches Verhalten kopieren. Falls HTU21D ohne EMPTY_ROW endet, AHT20 auch ohne.)

- [ ] **Step 3: Compile-Check**

```bash
pio run -d airrohr-firmware -e nodemcuv2 2>&1 | tail -20
```

Expected: Build OK.

- [ ] **Step 4: Commit**

```bash
git add airrohr-firmware/airrohr-firmware.ino
git commit -m "feat: AHT20-Werte in Status-Page-Anzeige"
```

---

## Task A11: Debug-Print + Selected-Sensor-Source

**Files:**
- Modify: `airrohr-firmware/airrohr-firmware.ino:2891` (Debug-Print)
- Modify: `airrohr-firmware/airrohr-firmware.ino:5012` (sensor-source Mapping)
- Modify: `airrohr-firmware/airrohr-firmware.ino:5067` (combined-sensor-on guard)

- [ ] **Step 1: Debug-Print direkt nach HTU21D-Print einfügen**

In Zeile ~2892, direkt nach `debug_outln_info_bool(F("HTU21D: "), cfg::htu21d_read);`:

```cpp
	debug_outln_info_bool(F("AHT20: "), cfg::aht20_read);
```

- [ ] **Step 2: Selected-Sensor-Source-Block einfügen**

Suche den Block (~Zeile 5008):
```cpp
	if (cfg::htu21d_read)
	{
		h_sensor = t_sensor = FPSTR(SENSORS_HTU21D);
		t_value = last_value_HTU21D_T;
		h_value = last_value_HTU21D_H;
	}
```

Direkt nach der schließenden `}` einfügen:

```cpp
	if (cfg::aht20_read)
	{
		h_sensor = t_sensor = FPSTR(SENSORS_AHT20);
		t_value = last_value_AHT20_T;
		h_value = last_value_AHT20_H;
	}
```

(Reihenfolge: AHT20 nach HTU21D bedeutet: wenn beide an sind, gewinnt AHT20 als selected source. Pragmatisch sinnvoll, weil AHT20 unser bevorzugter neuer Sensor ist.)

- [ ] **Step 3: Combined-sensor-on guard erweitern**

Suche Zeile ~5067:
```cpp
	if (cfg::dht_read || cfg::ds18b20_read || cfg::htu21d_read || cfg::bmp_read || cfg::bmx280_read || cfg::sht3x_read)
```

Erweitere zu:

```cpp
	if (cfg::dht_read || cfg::ds18b20_read || cfg::htu21d_read || cfg::aht20_read || cfg::bmp_read || cfg::bmx280_read || cfg::sht3x_read)
```

- [ ] **Step 4: Compile-Check**

```bash
pio run -d airrohr-firmware -e nodemcuv2 2>&1 | tail -30
```

Expected: Build OK, alle Touchpoints integriert.

- [ ] **Step 5: Commit**

```bash
git add airrohr-firmware/airrohr-firmware.ino
git commit -m "feat: AHT20 Debug-Output, Selected-Source, combined-sensor-guard"
```

---

## Task A12: Firmware-Smoke-Build (Final)

**Files:** none (Verification)

- [ ] **Step 1: Saubere Komplett-Build**

```bash
cd ~/projects/sensors-software && pio run -d airrohr-firmware -e nodemcuv2 -t clean && pio run -d airrohr-firmware -e nodemcuv2 2>&1 | tail -30
```

Expected: `[SUCCESS]` mit Flash-Usage-Anzeige. Ziel: Flash < 95% (sonst kommt OTA in Bedrängnis).

- [ ] **Step 2: Firmware-Binary lokalisieren**

```bash
ls -la airrohr-firmware/.pio/build/nodemcuv2/firmware.bin
```

Erwartet: Datei vorhanden, ca. 500-700 KB.

- [ ] **Step 3: Push des kompletten Branches**

```bash
git push origin feature/aht20-support
```

---

# Phase B: fein2wunder (PHP-Wrapper)

## Task B0: Branch und Test-Helper anlegen

**Files:**
- Create: `~/projects/fein2wunder/tests/test-aht20.sh`
- Create: `~/projects/fein2wunder/tests/fixtures/payload-aht20.json`

- [ ] **Step 1: Branch anlegen**

```bash
cd ~/projects/fein2wunder && git checkout -b feature/aht20-support
```

- [ ] **Step 2: Test-Verzeichnis anlegen**

```bash
mkdir -p tests/fixtures
```

- [ ] **Step 3: Mock-JSON-Payload schreiben**

Datei `tests/fixtures/payload-aht20.json`:

```json
{
  "esp8266id": "12345",
  "software_version": "NRZ-2024-134-B5+aht20",
  "sensordatavalues": [
    {"value_type": "SDS_P1", "value": "12.34"},
    {"value_type": "SDS_P2", "value": "5.67"},
    {"value_type": "AHT20_temperature", "value": "22.45"},
    {"value_type": "AHT20_humidity", "value": "48.20"},
    {"value_type": "BMP280_pressure", "value": "100850"},
    {"value_type": "samples", "value": "1234"},
    {"value_type": "min_micro", "value": "180"},
    {"value_type": "max_micro", "value": "210"},
    {"value_type": "signal", "value": "-65dBm"}
  ]
}
```

- [ ] **Step 4: Test-Runner-Script schreiben**

Datei `tests/test-aht20.sh`:

```bash
#!/usr/bin/env bash
# Smoke-Test für AHT20-Modi (t=4, h=4, p=3) in d.php.
# Ruft d.php via PHP-Built-in-Server auf, prüft CSV-Output.
set -euo pipefail

cd "$(dirname "$0")/.."
PORT=8089

# d.php-Verzeichnis als Root, php-CLI im Hintergrund
php -S "127.0.0.1:${PORT}" >/dev/null 2>&1 &
SERVER_PID=$!
trap "kill ${SERVER_PID} 2>/dev/null || true" EXIT
sleep 1

# Mock-Sensor-Header + JSON-Body posten
RESP=$(curl -s -X POST \
    -H "Content-Type: application/json" \
    -H "X-Sensor: testsensor-aht20" \
    --data-binary @tests/fixtures/payload-aht20.json \
    "http://127.0.0.1:${PORT}/d.php?id=TESTID&key=DUMMY&alt=200&t=4&h=4&p=3")

# CSV-File suchen (heutiges Datum)
TODAY=$(date -u +%Y-%m-%d)
CSV="data/data-testsensor-aht20-${TODAY}.csv"
if [ ! -f "$CSV" ]; then
    echo "FAIL: CSV-File nicht erstellt: $CSV"
    exit 1
fi

# Letzte Zeile prüfen: AHT20-Temp (22.45) und AHT20-Humidity (48.20) müssen in den neuen Spalten stehen
LAST=$(tail -1 "$CSV")
echo "CSV-Zeile: $LAST"

if ! echo "$LAST" | command grep -q "22.45"; then
    echo "FAIL: AHT20_temperature (22.45) nicht in CSV-Zeile gefunden"
    exit 1
fi
if ! echo "$LAST" | command grep -q "48.20"; then
    echo "FAIL: AHT20_humidity (48.20) nicht in CSV-Zeile gefunden"
    exit 1
fi
if ! echo "$LAST" | command grep -q "100850"; then
    echo "FAIL: BMP280_pressure (100850) nicht in CSV-Zeile gefunden"
    exit 1
fi

echo "OK: alle Werte in CSV gelandet"
```

- [ ] **Step 5: Script ausführbar machen + Test ausführen (erwarte FAIL)**

```bash
chmod +x tests/test-aht20.sh
./tests/test-aht20.sh
```

Expected: FAIL — d.php kennt `t=4`, `h=4`, `p=3` noch nicht, also fallen die Werte aus dem Switch und landen NICHT in CSV. Das ist der "RED"-Teil von TDD.

- [ ] **Step 6: Commit der Test-Infrastruktur**

```bash
git add tests/
git commit -m "test: Smoke-Test-Harness für AHT20-Modi (RED-Phase)"
```

---

## Task B1: t=4 Switch-Case (AHT20-Temperatur)

**Files:**
- Modify: `~/projects/fein2wunder/d.php:84-94` (Temp-Switch)

- [ ] **Step 1: Aktueller Temp-Switch**

Zeilen 84-94 enthalten:
```php
$tParam = isset($_GET['t']) ? intval($_GET['t']) : 1;
switch ($tParam) {
    case 2:  // BMP180-Temperatursensor
        $values['wtemperature'] = $values['BMP_temperature'] ?? $values['temperature'] ?? null;
        break;
    case 3:  // BME280-Temperatursensor
        $values['wtemperature'] = $values['BME280_temperature'] ?? $values['temperature'] ?? null;
        break;
    default: // DHT22 (Standard)
        $values['wtemperature'] = $values['temperature'] ?? null;
        break;
}
```

- [ ] **Step 2: case 4 für AHT20 vor `default:` einfügen**

In `d.php`, im Temp-Switch, nach `case 3:`-Block und vor `default:`:

```php
    case 4:  // AHT20-Temperatursensor
        $values['wtemperature'] = $values['AHT20_temperature'] ?? $values['temperature'] ?? null;
        break;
```

- [ ] **Step 3: Compile-Smoke (php -l)**

```bash
php -l d.php
```

Expected: `No syntax errors detected in d.php`.

---

## Task B2: h=4 Switch-Case (AHT20-Humidity)

**Files:**
- Modify: `~/projects/fein2wunder/d.php:96-105` (Humidity-Switch)

- [ ] **Step 1: case 4 für AHT20 vor `default:` einfügen**

Im `d.php`-Humidity-Switch, vor `default:`:

```php
    case 4:  // AHT20-Feuchtigkeit
        $values['whumidity'] = $values['AHT20_humidity'] ?? $values['humidity'] ?? null;
        break;
```

- [ ] **Step 2: Compile-Smoke**

```bash
php -l d.php
```

Expected: keine Syntax-Errors.

---

## Task B3: p=3 Switch-Case (BMP280-Druck stand-alone)

**Files:**
- Modify: `~/projects/fein2wunder/d.php:107-116` (Druck-Switch)

- [ ] **Step 1: case 3 für BMP280 vor `default:` einfügen**

Im `d.php`-Druck-Switch, vor `default:`:

```php
    case 3:  // BMP280-Druck stand-alone (nur Druck, ohne Humidity)
        $rawPressure = $values['BMP280_pressure'] ?? $values['BMP_pressure'] ?? null;
        break;
```

- [ ] **Step 2: Compile-Smoke**

```bash
php -l d.php
```

Expected: keine Syntax-Errors.

---

## Task B4: CSV-Header und Row erweitern

**Files:**
- Modify: `~/projects/fein2wunder/d.php:254-260` (Header)
- Modify: `~/projects/fein2wunder/d.php:264-288` (Row)

- [ ] **Step 1: Header-Zeile um drei neue Spalten erweitern**

In `d.php` die Header-Zeile (~Zeile 254-260):

```php
if (!file_exists($filename)) {
    file_put_contents($filename, implode(";", [
        "Time", "Altitude", "Temp", "Humidity", "Dew",
        "BMP_temp", "BMP_pressure", "BMP_calibrate", "Pressure_inHg",
        "BME280_temp", "BME280_humidity", "BME280_pressure",
        "AHT20_temp", "AHT20_humidity", "BMP280_pressure",
        "PM10", "PM2_5", "Samples", "Min_cycle", "Max_cycle",
        "Signal", "wTemp", "wHumidity", "wPressure_hPa", "WU_ID", "WU_Response"
    ]) . "\n");
}
```

(Drei neue Spalten zwischen BME280-Block und PM10. Position so gewählt, dass alte CSVs vorne unverändert bleiben.)

- [ ] **Step 2: Row-Daten um drei neue Felder erweitern**

In `d.php` die Row (~Zeile 264-288), zwischen `$values['BME280_pressure']` und `$values['SDS_P1']`:

```php
$csvRow = array_map('csvEscape', [
    $now,
    $values['altitude']         ?? '',
    $values['temperature']      ?? '',
    $values['humidity']         ?? '',
    $values['dew']              ?? '',
    $values['BMP_temperature']  ?? '',
    $values['BMP_pressure']     ?? '',
    $values['bmpcalibrate']     ?? '',
    $values['baroinch'],
    $values['BME280_temperature'] ?? '',
    $values['BME280_humidity']    ?? '',
    $values['BME280_pressure']    ?? '',
    $values['AHT20_temperature']  ?? '',
    $values['AHT20_humidity']     ?? '',
    $values['BMP280_pressure']    ?? '',
    $values['SDS_P1']  ?? '',
    $values['SDS_P2']  ?? '',
    $values['samples'] ?? '',
    $values['min_micro'] ?? '',
    $values['max_micro'] ?? '',
    $values['signal']   ?? '',
    $values['wtemperature'] ?? '',
    $values['whumidity']    ?? '',
    $values['wpressure']    ?? '',
    $values['id']           ?? '',
    $resp
]);
```

- [ ] **Step 3: Compile-Smoke**

```bash
php -l d.php
```

Expected: keine Syntax-Errors.

---

## Task B5: Test-Run + Verifikation

**Files:** none (Verification)

- [ ] **Step 1: Vorhandene Test-CSV vom B0-RED-Lauf löschen**

```bash
rm -rf data/
```

(Der RED-Lauf hat ein CSV erstellt, das jetzt verfälschend wäre.)

- [ ] **Step 2: Test ausführen**

```bash
./tests/test-aht20.sh
```

Expected: `OK: alle Werte in CSV gelandet`. Wenn FAIL → debugge mit `cat data/data-testsensor-aht20-*.csv` und prüfe welche Werte fehlen.

- [ ] **Step 3: Edge-Case: kein t-Parameter (Default-Verhalten unverändert)**

```bash
# Alte URL ohne t/h/p — muss weiterhin DHT22-Pfad nehmen
RESP=$(curl -s -X POST \
    -H "Content-Type: application/json" \
    -H "X-Sensor: testsensor-legacy" \
    --data-binary @tests/fixtures/payload-aht20.json \
    "http://127.0.0.1:8089/d.php?id=TESTID&key=DUMMY&alt=200")
# (Server muss laufen — Test-Script startet einen, aber nur kurz; manuell starten:)
# php -S 127.0.0.1:8089 &
```

(Optional, kann auch im erweiterten Smoke-Test in Task B7 abgedeckt werden.)

- [ ] **Step 4: Commit der drei Switch-Cases + CSV-Erweiterung**

```bash
git add d.php
git commit -m "feat: AHT20-Modi (t=4, h=4) und BMP280-only-Druck (p=3)"
```

---

## Task B6: README.md aktualisieren

**Files:**
- Modify: `~/projects/fein2wunder/README.md`

- [ ] **Step 1: Optionale-URL-Parameter-Tabelle erweitern**

In `README.md` die Tabelle "Optionale URL-Parameter":

| Parameter | Beschreibung |
|-----------|-------------|
| `alt=360` | Stationshöhe in Metern über NN (für Druckkorrektur auf Meereshöhe) |
| `bmpc=1.01234` | Manueller Druckkorrekturfaktor (Alternative zu `alt`) |
| `t=1\|2\|3\|4` | Temperatursensor: 1=DHT22 (Standard), 2=BMP180, 3=BME280, **4=AHT20** |
| `h=1\|3\|4` | Feuchtigkeitssensor: 1=DHT22 (Standard), 3=BME280, **4=AHT20** |
| `p=1\|2\|3` | Drucksensor: 1=BMP180 (Standard), 2=BME280, **3=BMP280 stand-alone** |

- [ ] **Step 2: Sensor-Tabelle erweitern**

Im Abschnitt "Unterstützte Sensoren":

| Sensor | Temperatur | Feuchtigkeit | Druck |
|--------|-----------|-------------|-------|
| DHT22 | ✅ (Standard) | ✅ (Standard) | — |
| BMP180 | ✅ (`t=2`) | — | ✅ (Standard) |
| BME280 | ✅ (`t=3`) | ✅ (`h=3`) | ✅ (`p=2`) |
| **AHT20** | ✅ (`t=4`) | ✅ (`h=4`) | — |
| **BMP280** | — | — | ✅ (`p=3`) |
| SDS011 | — | — | — (PM2.5/PM10) |

- [ ] **Step 3: Beispiel-URL für AHT20+BMP280-Combo ergänzen**

Unter den bestehenden Beispielen:

```
/d.php?id=STATIONSID&key=APIKEY&alt=360&t=4&h=4&p=3
```

(AHT20 für Temp+Feuchte, BMP280 für Druck — der Standard-Fall für GY-21P-Combo-Boards als DHT22-Ersatz.)

- [ ] **Step 4: Commit**

```bash
git add README.md
git commit -m "docs: AHT20- und BMP280-Modi in README"
```

---

## Task B7: Final Smoke-Test + Push

**Files:** none (Verification)

- [ ] **Step 1: Saubere Test-Umgebung**

```bash
rm -rf data/
```

- [ ] **Step 2: Test-Suite laufen lassen**

```bash
./tests/test-aht20.sh
```

Expected: `OK: alle Werte in CSV gelandet`.

- [ ] **Step 3: Push**

```bash
git push -u origin feature/aht20-support
```

- [ ] **Step 4: PR-Vorbereitung dokumentieren (kein Auto-PR)**

Datei `tests/incoming.log.gitignore-marker`:

```bash
# .gitignore-Eintrag für lokale Test-Artefakte
echo -e "data/\nincoming.log\ntests/data/" >> .gitignore
git add .gitignore
git commit -m "chore: gitignore für Test-Artefakte"
git push
```

---

# Phase C: Roll-out (Operations, kein Code)

Diese Tasks sind Hardware- und Operations-Schritte. Sie werden vom User händisch durchgeführt und nicht durch Implementierungs-Subagenten.

## Task C1: Eine Test-Station umrüsten

- [ ] DHT22 ablöten / abziehen
- [ ] AHT20+BMP280-Combo verkabeln (3V3, GND, D1=SCL, D2=SDA)
- [ ] Firmware-OTA-Update via `/update` (firmware.bin aus Phase A12)
- [ ] In Web-UI: Checkbox AHT20 aktivieren, BMP280 (vermutlich schon aktiv) prüfen
- [ ] Custom-API-URL in der Web-UI: `/d.php?id=STATIONSID&key=APIKEY&alt=<Höhe>&t=4&h=4&p=3`

## Task C2: 24h-Plausibilitäts-Check

- [ ] Sensor-Web-UI öffnen, aktuelle AHT20-Werte ablesen
- [ ] In `data/data-<station>-*.csv` die letzten 24 Zeilen prüfen
- [ ] In Wunderground-PWS-Dashboard den Verlauf prüfen
- [ ] Plausibilitätskorridor: AHT20-Temp 0,5–1°C niedriger als historischer DHT22 (kein Eigen-Heating)

## Task C3: Roll-out auf 19 weitere Stationen

- [ ] Skript für Bulk-OTA-Push schreiben (curl-Schleife über Hostnamen)
- [ ] Pro Station Hardware tauschen (1/Tag oder Block-Tausch)
- [ ] Pro Station Custom-API-URL umstellen auf `t=4&h=4&p=3`

## Task C4: Upstream-PR (4-8 Wochen später)

- [ ] Felderfahrung sammeln (mindestens 4 Wochen Stabilität)
- [ ] Forum-Vorabpost auf forum.sensor.community
- [ ] PR `hibbes:feature/aht20-support` → `opendata-stuttgart:master`
- [ ] Nur Firmware-Teil, fein2wunder bleibt im Privat-Fork

---

# Erfolgs-Kriterien

1. ✅ `pio run -e nodemcuv2` läuft ohne Errors, mit Flash-Auslastung < 95%
2. ✅ `./tests/test-aht20.sh` in fein2wunder gibt `OK: alle Werte in CSV gelandet`
3. ✅ Eine Test-Station meldet 24h plausible Werte über fein2wunder an Wunderground und CSV
4. ✅ Roll-out auf 20 Stationen mit Datenverlust < 2h pro Station
5. ✅ (späte Phase) Upstream-PR an opendata-stuttgart eingereicht
