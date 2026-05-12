/*
 * sensors/gps.cpp — hibbes-Patch (Issue #7 Phase 1 Refactor)
 *
 * Liest den aktuellen TinyGPSPlus-State (Position/Höhe/Zeit) aus,
 * aktualisiert die last_value_GPS_*-Globals (mit Sentinel-Werten
 * -200/-1000 bei invaliden Fixes), und schreibt im send_now-Tick
 * die JSON-Schlüssel ins übergebene String-Reservoir.
 *
 * Kontrakt: emittiert JSON-Schlüssel "GPS_lat", "GPS_lon",
 * "GPS_height", "GPS_timestamp".
 *
 * ACHTUNG: Das kontinuierliche UART-Byte-Lesen läuft im Haupt-loop()
 * in airrohr-firmware.ino — fetchSensorGPS() macht KEINE serielle
 * Kommunikation. Globals werden weiterhin in airrohr-firmware.ino
 * definiert; hier nur `extern`-Verweise.
 */

#include "gps.h"
#include <Arduino.h>
#include <TinyGPS++.h>
#include "../utils.h"

// Globals definiert in airrohr-firmware.ino
extern TinyGPSPlus gps;
extern float last_value_GPS_alt;
extern double last_value_GPS_lat;
extern double last_value_GPS_lon;
extern String last_value_GPS_timestamp;
extern bool send_now;
extern unsigned long count_sends;
extern bool gps_init_failed;

void fetchSensorGPS(String &s)
{
	debug_outln_verbose(F("R/ "), "GPS");

	if (gps.location.isUpdated())
	{
		if (gps.location.isValid())
		{
			last_value_GPS_lat = gps.location.lat();
			last_value_GPS_lon = gps.location.lng();
		}
		else
		{
			last_value_GPS_lat = -200;
			last_value_GPS_lon = -200;
			debug_outln_verbose(F("Lat/Lng INVALID"));
		}
		if (gps.altitude.isValid())
		{
			last_value_GPS_alt = gps.altitude.meters();
		}
		else
		{
			last_value_GPS_alt = -1000;
			debug_outln_verbose(F("Altitude INVALID"));
		}
		if (!gps.date.isValid())
		{
			debug_outln_verbose(F("Date INVALID"));
		}
		if (!gps.time.isValid())
		{
			debug_outln_verbose(F("Time: INVALID"));
		}
		if (gps.date.isValid() && gps.time.isValid())
		{
			char gps_datetime[37];
			snprintf_P(gps_datetime, sizeof(gps_datetime), PSTR("%04d-%02d-%02dT%02d:%02d:%02d.%03d"),
					   gps.date.year(), gps.date.month(), gps.date.day(), gps.time.hour(), gps.time.minute(), gps.time.second(), gps.time.centisecond());
			last_value_GPS_timestamp = gps_datetime;
		}
		else
		{
			//define a default value
			last_value_GPS_timestamp = F("1970-01-01T00:00:00.000");
		}
	}

	if (send_now)
	{
		debug_outln_info(F("Lat: "), String(last_value_GPS_lat, 6));
		debug_outln_info(F("Lng: "), String(last_value_GPS_lon, 6));
		debug_outln_info(F("DateTime: "), last_value_GPS_timestamp);

		add_Value2Json(s, F("GPS_lat"), String(last_value_GPS_lat, 6));
		add_Value2Json(s, F("GPS_lon"), String(last_value_GPS_lon, 6));
		add_Value2Json(s, F("GPS_height"), F("Altitude: "), last_value_GPS_alt);
		add_Value2Json(s, F("GPS_timestamp"), last_value_GPS_timestamp);
		debug_outln_info(F("----"));
	}

	if (count_sends > 0 && gps.charsProcessed() < 10)
	{
		debug_outln_error(F("No GPS data received: check wiring"));
		gps_init_failed = true;
	}

	debug_outln_verbose(F("/R "), "GPS");
}

#include "../web/page_helpers.h"
#include "../html-content.h"

void render_gps_values(String &page_content)
{
	const String unit_Deg("°");
	add_table_row_from_value(page_content, FPSTR(WEB_GPS), FPSTR(INTL_LATITUDE),
							 check_display_value(last_value_GPS_lat, -200.0, 6, 0), unit_Deg);
	add_table_row_from_value(page_content, FPSTR(WEB_GPS), FPSTR(INTL_LONGITUDE),
							 check_display_value(last_value_GPS_lon, -200.0, 6, 0), unit_Deg);
	add_table_row_from_value(page_content, FPSTR(WEB_GPS), FPSTR(INTL_ALTITUDE),
							 check_display_value(last_value_GPS_alt, -1000.0, 2, 0), "m");
	add_table_row_from_value(page_content, FPSTR(WEB_GPS), FPSTR(INTL_TIME_UTC),
							 last_value_GPS_timestamp, emptyString);
	page_content += FPSTR(EMPTY_ROW);
}
