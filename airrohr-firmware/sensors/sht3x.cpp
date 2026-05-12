/*
 * sensors/sht3x.cpp — hibbes-Patch (Issue #7 Phase 1 Refactor)
 *
 * Liest den SHT3x-Combo-Sensor (I²C 0x44) per Adafruit_SHT31-Lib, schreibt die
 * Werte in das übergebene JSON-String-Reservoir und aktualisiert die globalen
 * last_value_SHT3X_T/H.
 *
 * Kontrakt: emittiert JSON-Schlüssel "SHT3X_temperature" und "SHT3X_humidity".
 *
 * Globals werden weiterhin in airrohr-firmware.ino definiert; hier nur
 * `extern`-Verweise.
 */

#include "sht3x.h"
#include <Adafruit_SHT31.h>
#include "../utils.h"

// Globals definiert in airrohr-firmware.ino
extern Adafruit_SHT31 sht3x;
extern float last_value_SHT3X_T;
extern float last_value_SHT3X_H;

void fetchSensorSHT3x(String &s)
{
	debug_outln_verbose(F("R/ "), F("SHT3x"));

	const auto t = sht3x.readTemperature();
	const auto h = sht3x.readHumidity();
	if (isnan(h) || isnan(t))
	{
		last_value_SHT3X_T = -128.0;
		last_value_SHT3X_H = -1.0;
		debug_outln_error(F("SHT3X read failed"));
	}
	else
	{
		last_value_SHT3X_T = t;
		last_value_SHT3X_H = h;
		add_Value2Json(s, F("SHT3X_temperature"), F("Temperature (°C): "), last_value_SHT3X_T);
		add_Value2Json(s, F("SHT3X_humidity"), F("Humidity (%): "), last_value_SHT3X_H);
	}
	debug_outln_info(F("----"));
	debug_outln_verbose(F("/R "), F("SHT3x"));
}

#include "../web/page_helpers.h"
#include "../html-content.h"
#include <cmath>

void render_sht3x_values(String &page_content)
{
	add_table_t_value(page_content, FPSTR(SENSORS_SHT3X), FPSTR(INTL_TEMPERATURE), last_value_SHT3X_T);
	add_table_h_value(page_content, FPSTR(SENSORS_SHT3X), FPSTR(INTL_HUMIDITY), last_value_SHT3X_H);
	float dew = dew_point(last_value_SHT3X_T, last_value_SHT3X_H);
	add_table_row_from_value(page_content, FPSTR(SENSORS_SHT3X), FPSTR(INTL_DEW_POINT),
							 isnan(dew) ? "-" : String(dew, 1), "°C");
	page_content += FPSTR(EMPTY_ROW);
}
