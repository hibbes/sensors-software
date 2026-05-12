/*
 * sensors/htu21d.cpp — hibbes-Patch (Issue #7 Phase 1 Refactor)
 *
 * Liest den HTU21D-Combo-Sensor (I²C 0x40) per Adafruit_HTU21DF-Lib, schreibt
 * die Werte in das übergebene JSON-String-Reservoir und aktualisiert die
 * globalen last_value_HTU21D_T/H.
 *
 * Kontrakt: emittiert JSON-Schlüssel "HTU21D_temperature" und "HTU21D_humidity".
 *
 * Globals werden weiterhin in airrohr-firmware.ino definiert; hier nur
 * `extern`-Verweise.
 */

#include "htu21d.h"
#include <Adafruit_HTU21DF.h>
#include "../utils.h"

// Globals definiert in airrohr-firmware.ino
extern Adafruit_HTU21DF htu21d;
extern float last_value_HTU21D_T;
extern float last_value_HTU21D_H;

void fetchSensorHTU21D(String &s)
{
	debug_outln_verbose(F("R/ "), F("HTU21D"));

	const auto t = htu21d.readTemperature();
	const auto h = htu21d.readHumidity();
	if (isnan(t) || isnan(h))
	{
		last_value_HTU21D_T = -128.0;
		last_value_HTU21D_H = -1.0;
		debug_outln_error(F("HTU21D read failed"));
	}
	else
	{
		last_value_HTU21D_T = t;
		last_value_HTU21D_H = h;
		add_Value2Json(s, F("HTU21D_temperature"), F("Temperature (°C): "), last_value_HTU21D_T);
		add_Value2Json(s, F("HTU21D_humidity"), F("Humidity (%): "), last_value_HTU21D_H);
	}
	debug_outln_info(F("----"));

	debug_outln_verbose(F("/R "), F("HTU21D"));
}

#include "../web/page_helpers.h"
#include "../html-content.h"
#include <cmath>

void render_htu21d_values(String &page_content)
{
	add_table_t_value(page_content, FPSTR(SENSORS_HTU21D), FPSTR(INTL_TEMPERATURE), last_value_HTU21D_T);
	add_table_h_value(page_content, FPSTR(SENSORS_HTU21D), FPSTR(INTL_HUMIDITY), last_value_HTU21D_H);
	float dew = dew_point(last_value_HTU21D_T, last_value_HTU21D_H);
	add_table_row_from_value(page_content, FPSTR(SENSORS_HTU21D), FPSTR(INTL_DEW_POINT),
							 isnan(dew) ? "-" : String(dew, 1), "°C");
	page_content += FPSTR(EMPTY_ROW);
}
