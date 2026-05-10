/*
 * sensors/aht20.cpp — hibbes-Patch (Issue #7 Refactor Phase 1, AHT20-Pilot)
 *
 * Liest den AHT20-Combo-Sensor (I²C 0x38) per Adafruit_AHTX0-Lib, schreibt
 * die Werte in das übergebene JSON-String-Reservoir und aktualisiert die
 * globalen last_value_AHT20_T/H.
 *
 * Kontrakt: emittiert JSON-Schlüssel "temperature" und "humidity" (DHT22-
 * Maskerade-Format) — siehe HIBBES_PATCH.md Abschnitt 2 für die Begründung.
 *
 * Globals werden weiterhin in airrohr-firmware.ino definiert; hier nur
 * `extern`-Verweise.
 */

#include "aht20.h"
#include <Adafruit_AHTX0.h>
#include "../utils.h"

// Globals definiert in airrohr-firmware.ino
extern Adafruit_AHTX0 aht20;
extern float last_value_AHT20_T;
extern float last_value_AHT20_H;

void fetchSensorAHT20(String &s)
{
	debug_outln_verbose(F("Sensor reading "), F("AHT20"));

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
		// Maskerade als DHT22 für Sensor.Community + OpenSenseMap-luftdaten-Parser
		last_value_AHT20_T = temp_event.temperature;
		last_value_AHT20_H = humidity_event.relative_humidity;
		add_Value2Json(s, F("temperature"), F("Temperature (°C): "), last_value_AHT20_T);
		add_Value2Json(s, F("humidity"), F("Humidity (%): "), last_value_AHT20_H);
	}
	debug_outln_info(F("----"));

	debug_outln_verbose(F("Sensor end "), F("AHT20"));
}
