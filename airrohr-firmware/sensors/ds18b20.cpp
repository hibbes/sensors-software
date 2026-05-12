/*
 * sensors/ds18b20.cpp — hibbes-Patch (Issue #7 Phase 1 Refactor)
 *
 * Liest den DS18B20-OneWire-Sensor (T-only) per DallasTemperature-Lib, schreibt
 * den Wert in das übergebene JSON-String-Reservoir und aktualisiert das globale
 * last_value_DS18B20_T. Robuster Retry-Loop (5 Versuche) gegen 85.0/-127.0-
 * Fehlwerte. Temp-Korrektur via readCorrectionOffset(cfg::temp_correction).
 *
 * Kontrakt: emittiert JSON-Schlüssel "DS18B20_temperature".
 *
 * Globals werden weiterhin in airrohr-firmware.ino definiert; hier nur
 * `extern`-Verweise. OneWire-Instance ist transitiv via DallasTemperature
 * verbunden, kommt aber als eigenes Symbol in der .ino vor.
 */

#include "ds18b20.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include "../utils.h"

// Globals definiert in airrohr-firmware.ino
extern OneWire oneWire;
extern DallasTemperature ds18b20;
extern float last_value_DS18B20_T;

namespace cfg {
	extern char temp_correction[];
}

void fetchSensorDS18B20(String &s)
{
	float t;
	debug_outln_verbose(F("R/ "), F("DS18B20"));

	//it's very unlikely (-127: impossible) to get these temperatures in reality. Most times this means that the sensor is currently faulty
	//try 5 times to read the sensor, otherwise fail
	const int MAX_ATTEMPTS = 5;
	int count = 0;
	do
	{
		ds18b20.requestTemperatures();
		//for now, we want to read only the first sensor
		t = ds18b20.getTempCByIndex(0);
		++count;
		debug_outln_info(F("DS18B20 trying...."));
	} while (count < MAX_ATTEMPTS && (isnan(t) || t >= 85.0f || t <= (-127.0f)));

	if (count == MAX_ATTEMPTS)
	{
		last_value_DS18B20_T = -128.0;
		debug_outln_error(F("DS18B20 read failed"));
	}
	else
	{
		last_value_DS18B20_T = t + readCorrectionOffset(cfg::temp_correction);
		add_Value2Json(s, F("DS18B20_temperature"), F("Temperature (°C): "), last_value_DS18B20_T);
	}
	debug_outln_info(F("----"));
	debug_outln_verbose(F("/R "), F("DS18B20"));
}

#include "../web/page_helpers.h"
#include "../html-content.h"

void render_ds18b20_values(String &page_content)
{
	add_table_t_value(page_content, FPSTR(SENSORS_DS18B20), FPSTR(INTL_TEMPERATURE), last_value_DS18B20_T);
	page_content += FPSTR(EMPTY_ROW);
}
