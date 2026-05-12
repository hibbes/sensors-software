/*
 * sensors/sps30.cpp — hibbes-Patch (Issue #7 Phase 1 Refactor)
 *
 * Mittelt die im Haupt-loop() aufgesammelten SPS30-Rohwerte und
 * schreibt die JSON-Schlüssel ins übergebene String-Reservoir.
 * Resettet anschließend die Akkumulator-Globals für das nächste
 * Sende-Intervall.
 *
 * Kontrakt: emittiert JSON-Schlüssel "SPS30_P0", "SPS30_P2",
 * "SPS30_P4", "SPS30_P1", "SPS30_N05", "SPS30_N1", "SPS30_N25",
 * "SPS30_N4", "SPS30_N10", "SPS30_TS".
 *
 * Globals werden weiterhin in airrohr-firmware.ino definiert; hier
 * nur `extern`-Verweise.
 */

#include "sps30.h"
#include "../utils.h"

// Globals definiert in airrohr-firmware.ino
extern float last_value_SPS30_P0;
extern float last_value_SPS30_P1;
extern float last_value_SPS30_P2;
extern float last_value_SPS30_P4;
extern float last_value_SPS30_N05;
extern float last_value_SPS30_N1;
extern float last_value_SPS30_N25;
extern float last_value_SPS30_N4;
extern float last_value_SPS30_N10;
extern float last_value_SPS30_TS;

extern float value_SPS30_P0;
extern float value_SPS30_P1;
extern float value_SPS30_P2;
extern float value_SPS30_P4;
extern float value_SPS30_N05;
extern float value_SPS30_N1;
extern float value_SPS30_N25;
extern float value_SPS30_N4;
extern float value_SPS30_N10;
extern float value_SPS30_TS;

extern uint16_t SPS30_measurement_count;
extern unsigned long SPS30_read_counter;
extern unsigned long SPS30_read_error_counter;

void fetchSensorSPS30(String &s)
{
	debug_outln_verbose(F("R/ "), F("Sensirion SPS30"));

	last_value_SPS30_P0 = value_SPS30_P0 / SPS30_measurement_count;
	last_value_SPS30_P2 = value_SPS30_P2 / SPS30_measurement_count;
	last_value_SPS30_P4 = value_SPS30_P4 / SPS30_measurement_count;
	last_value_SPS30_P1 = value_SPS30_P1 / SPS30_measurement_count;
	last_value_SPS30_N05 = value_SPS30_N05 / SPS30_measurement_count;
	last_value_SPS30_N1 = value_SPS30_N1 / SPS30_measurement_count;
	last_value_SPS30_N25 = value_SPS30_N25 / SPS30_measurement_count;
	last_value_SPS30_N4 = value_SPS30_N4 / SPS30_measurement_count;
	last_value_SPS30_N10 = value_SPS30_N10 / SPS30_measurement_count;
	last_value_SPS30_TS = value_SPS30_TS / SPS30_measurement_count;

	add_Value2Json(s, F("SPS30_P0"), F("PM1.0: "), last_value_SPS30_P0);
	add_Value2Json(s, F("SPS30_P2"), F("PM2.5: "), last_value_SPS30_P2);
	add_Value2Json(s, F("SPS30_P4"), F("PM4.0: "), last_value_SPS30_P4);
	add_Value2Json(s, F("SPS30_P1"), F("PM 10: "), last_value_SPS30_P1);
	add_Value2Json(s, F("SPS30_N05"), F("NC0.5: "), last_value_SPS30_N05);
	add_Value2Json(s, F("SPS30_N1"), F("NC1.0: "), last_value_SPS30_N1);
	add_Value2Json(s, F("SPS30_N25"), F("NC2.5: "), last_value_SPS30_N25);
	add_Value2Json(s, F("SPS30_N4"), F("NC4.0: "), last_value_SPS30_N4);
	add_Value2Json(s, F("SPS30_N10"), F("NC10:  "), last_value_SPS30_N10);
	add_Value2Json(s, F("SPS30_TS"), F("TPS:   "), last_value_SPS30_TS);

	debug_outln_info(F("SPS30 read counter: "), String(SPS30_read_counter));
	debug_outln_info(F("SPS30 read error counter: "), String(SPS30_read_error_counter));

	SPS30_measurement_count = 0;
	SPS30_read_counter = 0;
	SPS30_read_error_counter = 0;
	value_SPS30_P0 = value_SPS30_P1 = value_SPS30_P2 = value_SPS30_P4 = 0.0;
	value_SPS30_N05 = value_SPS30_N1 = value_SPS30_N25 = value_SPS30_N10 = value_SPS30_N4 = 0.0;
	value_SPS30_TS = 0.0;

	debug_outln_info(F("----"));
	debug_outln_verbose(F("/R "), F("Sensirion SPS30"));
}

/*****************************************************************
 * /values render: Sensirion SPS30 — 10 Werte. Issue #18 Phase E.  *
 *****************************************************************/
#include "../web/page_helpers.h"
#include "../html-content.h"

void render_sps30_values(String &page_content)
{
	add_table_pm_value(page_content, FPSTR(SENSORS_SPS30), FPSTR(WEB_PM1), last_value_SPS30_P0);
	add_table_pm_value(page_content, FPSTR(SENSORS_SPS30), FPSTR(WEB_PM25), last_value_SPS30_P2);
	add_table_pm_value(page_content, FPSTR(SENSORS_SPS30), FPSTR(WEB_PM4), last_value_SPS30_P4);
	add_table_pm_value(page_content, FPSTR(SENSORS_SPS30), FPSTR(WEB_PM10), last_value_SPS30_P1);
	add_table_nc_value(page_content, FPSTR(SENSORS_SPS30), FPSTR(WEB_NC0k5), last_value_SPS30_N05);
	add_table_nc_value(page_content, FPSTR(SENSORS_SPS30), FPSTR(WEB_NC1k0), last_value_SPS30_N1);
	add_table_nc_value(page_content, FPSTR(SENSORS_SPS30), FPSTR(WEB_NC2k5), last_value_SPS30_N25);
	add_table_nc_value(page_content, FPSTR(SENSORS_SPS30), FPSTR(WEB_NC4k0), last_value_SPS30_N4);
	add_table_nc_value(page_content, FPSTR(SENSORS_SPS30), FPSTR(WEB_NC10), last_value_SPS30_N10);
	add_table_row_from_value(page_content, FPSTR(SENSORS_SPS30), FPSTR(WEB_TPS),
							 check_display_value(last_value_SPS30_TS, -1, 1, 0), "µm");
	page_content += FPSTR(EMPTY_ROW);
}

void render_sps30_status_error(String &page_content)
{
	add_table_row_from_value(page_content, FPSTR(SENSORS_SPS30), String(SPS30_read_error_counter));
}
