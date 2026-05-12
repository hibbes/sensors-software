/*
 * sensors/bmp180.cpp — hibbes-Patch (Issue #7 Phase 1 Refactor)
 *
 * Liest den BMP180-Sensor (I²C 0x77, OLD-chip) per Adafruit_BMP085-Lib,
 * schreibt die Werte in das übergebene JSON-String-Reservoir und aktualisiert
 * die globalen last_value_BMP_T/P.
 *
 * Kontrakt: emittiert JSON-Schlüssel "BMP_temperature" und "BMP_pressure"
 * (BMP_-Prefix, nicht BMP180_, weil downstream-Parser das kürzere Schema kennen).
 *
 * Globals werden weiterhin in airrohr-firmware.ino definiert; hier nur
 * `extern`-Verweise.
 */

#include "bmp180.h"
#include <Adafruit_BMP085.h>
#include "../utils.h"

// Globals definiert in airrohr-firmware.ino
extern Adafruit_BMP085 bmp;
extern float last_value_BMP_T;
extern float last_value_BMP_P;

void fetchSensorBMP(String &s)
{
	debug_outln_verbose(F("R/ "), F("BMP180"));

	const auto p = bmp.readPressure();
	const auto t = bmp.readTemperature();
	if (isnan(p) || isnan(t))
	{
		last_value_BMP_T = -128.0;
		last_value_BMP_P = -1.0;
		debug_outln_error(F("BMP180 read failed"));
	}
	else
	{
		last_value_BMP_T = t;
		last_value_BMP_P = p;
		add_Value2Json(s, F("BMP_pressure"), F("Pressure (hPa): "), last_value_BMP_P);
		add_Value2Json(s, F("BMP_temperature"), F("Temperature (°C): "), last_value_BMP_T);
	}
	debug_outln_info(F("----"));
	debug_outln_verbose(F("/R "), F("BMP180"));
}

#include "../web/page_helpers.h"
#include "../html-content.h"

void render_bmp180_values(String &page_content)
{
	const String unit_P("hPa");
	add_table_t_value(page_content, FPSTR(SENSORS_BMP180), FPSTR(INTL_TEMPERATURE), last_value_BMP_T);
	add_table_row_from_value(page_content, FPSTR(SENSORS_BMP180), FPSTR(INTL_PRESSURE),
							 check_display_value(last_value_BMP_P / 100.0f, (-1 / 100.0f), 2, 0), unit_P);
	add_table_row_from_value(page_content, FPSTR(SENSORS_BMP180), FPSTR(INTL_PRESSURE_AT_SEALEVEL),
							 last_value_BMP_P != -1.0f
								 ? String(pressure_at_sealevel(last_value_BMP_T, last_value_BMP_P / 100.0f), 2)
								 : "-",
							 unit_P);
	page_content += FPSTR(EMPTY_ROW);
}
