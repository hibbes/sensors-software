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

#include "../web/page_helpers.h"
#include "../html-content.h"
#include <cmath>

void render_scd30_values(String &page_content)
{
	add_table_t_value(page_content, FPSTR(SENSORS_SCD30), FPSTR(INTL_TEMPERATURE), last_value_SCD30_T);
	add_table_h_value(page_content, FPSTR(SENSORS_SCD30), FPSTR(INTL_HUMIDITY), last_value_SCD30_H);
	add_table_row_from_value(page_content, FPSTR(SENSORS_SCD30), FPSTR(INTL_CO2_PPM),
							 check_display_value(last_value_SCD30_CO2, 0, 0, 0), "ppm");
	float dew = dew_point(last_value_SCD30_T, last_value_SCD30_H);
	add_table_row_from_value(page_content, FPSTR(SENSORS_SCD30), FPSTR(INTL_DEW_POINT),
							 isnan(dew) ? "-" : String(dew, 1), "°C");
	page_content += FPSTR(EMPTY_ROW);
}

void render_scd30_status_info(String &page_content)
{
	if (scd30.getAutoSelfCalibration() == true)
		add_table_row_from_value(page_content, F("SCD30 Auto Calibration"), "enabled");
	else
		add_table_row_from_value(page_content, F("SCD30 Auto Calibration"), "disabled");
	uint16_t settingVal;
	scd30.getMeasurementInterval(&settingVal);
	add_table_row_from_value(page_content, F("SCD30 measurement interval"), String(settingVal));
	scd30.getTemperatureOffset(&settingVal);
	add_table_row_from_value(page_content, F("SCD30 temperature offset"), String(settingVal));
}
