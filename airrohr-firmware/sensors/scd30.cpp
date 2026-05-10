/*
 * sensors/scd30.cpp — hibbes-Patch (Issue #7 Phase 1 Refactor)
 *
 * Liest den Sensirion SCD30 (CO2 + T + H, I²C 0x61) per
 * SparkFun_SCD30_Arduino_Library, schreibt die Werte in das übergebene
 * JSON-String-Reservoir und aktualisiert die globalen last_value_SCD30_*.
 *
 * Kontrakt: emittiert JSON-Schlüssel "SCD30_temperature",
 * "SCD30_humidity" und "SCD30_co2_ppm".
 *
 * Globals werden weiterhin in airrohr-firmware.ino definiert; hier nur
 * `extern`-Verweise.
 */

#include "scd30.h"
#include <SparkFun_SCD30_Arduino_Library.h>
#include "../utils.h"

// Globals definiert in airrohr-firmware.ino
extern SCD30 scd30;
extern float last_value_SCD30_T;
extern float last_value_SCD30_H;
extern uint16_t last_value_SCD30_CO2;

void fetchSensorSCD30(String &s)
{
	debug_outln_verbose(F("Sensor reading "), F("SCD30"));

	const auto t = scd30.getTemperature();
	const auto h = scd30.getHumidity();
	const auto c = scd30.getCO2();

	if (isnan(h) || isnan(t) || isnan(c))
	{
		last_value_SCD30_T = -128.0;
		last_value_SCD30_H = -1.0;
		last_value_SCD30_CO2 = 0;
		debug_outln_error(F("SCD30 read failed"));
	}
	else
	{
		last_value_SCD30_T = t;
		last_value_SCD30_H = h;
		last_value_SCD30_CO2 = c;
		add_Value2Json(s, F("SCD30_temperature"), F("Temperature (°C): "), last_value_SCD30_T);
		add_Value2Json(s, F("SCD30_humidity"), F("Humidity (%): "), last_value_SCD30_H);
		add_Value2Json(s, F("SCD30_co2_ppm"), F("CO2 (ppm): "), last_value_SCD30_CO2);
	}
	debug_outln_info(F("----"));
	debug_outln_verbose(F("Sensor end "), F("SCD30"));
}
