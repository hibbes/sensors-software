/*
 * sensors/sds011.cpp — hibbes-Patch (Issue #7 Refactor Phase 1)
 *
 * Liest den Nova/Sensirion SDS011 PM-Sensor per UART (serialSDS),
 * verarbeitet das State-Machine-Frame-Format (HDR=0xAA 0xC0,
 * 8-Byte-Body mit Checksum), und schreibt PM10/PM2.5 in das
 * übergebene JSON-String-Reservoir.
 *
 * Kontrakt: emittiert JSON-Schlüssel "SDS_P1" (PM10) und "SDS_P2"
 * (PM2.5) — siehe Sensor.Community-luftdaten-Spec.
 *
 * Globals werden weiterhin in airrohr-firmware.ino definiert; hier nur
 * `extern`-Verweise.
 */

#include "sds011.h"
#include "../utils.h"
#include "../defines.h"

// Globals definiert in airrohr-firmware.ino
namespace cfg {
	extern unsigned sending_intervall_ms;
}

extern unsigned long act_milli;
extern unsigned long starttime;
extern bool send_now;

extern bool is_SDS_running;
// SDS_waiting_for ist im .ino als anonyme-enum-Variable deklariert; wir
// greifen über int-kompatible extern-Decl darauf zu.
extern int SDS_waiting_for;

extern uint32_t sds_pm10_sum;
extern uint32_t sds_pm25_sum;
extern uint32_t sds_val_count;
extern uint32_t sds_pm10_max;
extern uint32_t sds_pm10_min;
extern uint32_t sds_pm25_max;
extern uint32_t sds_pm25_min;

extern float last_value_SDS_P1;
extern float last_value_SDS_P2;
extern unsigned long SDS_error_count;

// SDS_REPLY_HDR/BODY: Werte aus dem .ino-anonymen-Enum repliziert (Wire-Format).
static constexpr int SDS_REPLY_HDR_LOCAL = 10;
static constexpr int SDS_REPLY_BODY_LOCAL = 8;

// msSince-Macro lokal repliziert (im .ino als #define mit act_milli definiert)
#ifndef msSince
#define msSince(timestamp_before) (act_milli - (timestamp_before))
#endif

void fetchSensorSDS(String &s)
{
	if (cfg::sending_intervall_ms > (WARMUPTIME_SDS_MS + READINGTIME_SDS_MS) &&
		msSince(starttime) < (cfg::sending_intervall_ms - (WARMUPTIME_SDS_MS + READINGTIME_SDS_MS)))
	{
		if (is_SDS_running)
		{
			is_SDS_running = SDS_cmd(PmSensorCmd::Stop);
		}
	}
	else
	{
		if (!is_SDS_running)
		{
			is_SDS_running = SDS_cmd(PmSensorCmd::Start);
			SDS_waiting_for = SDS_REPLY_HDR_LOCAL;
		}

		while (serialSDS.available() >= SDS_waiting_for)
		{
			const uint8_t constexpr hdr_measurement[2] = {0xAA, 0xC0};
			uint8_t data[8];

			switch (SDS_waiting_for)
			{
			case SDS_REPLY_HDR_LOCAL:
				if (serialSDS.find(hdr_measurement, sizeof(hdr_measurement)))
					SDS_waiting_for = SDS_REPLY_BODY_LOCAL;
				break;
			case SDS_REPLY_BODY_LOCAL:
				debug_outln_verbose(F("R/ "), F("SDS011"));
				if (serialSDS.readBytes(data, sizeof(data)) == sizeof(data) && SDS_checksum_valid(data))
				{
					uint32_t pm25_serial = data[0] | (data[1] << 8);
					uint32_t pm10_serial = data[2] | (data[3] << 8);

					if (msSince(starttime) > (cfg::sending_intervall_ms - READINGTIME_SDS_MS))
					{
						sds_pm10_sum += pm10_serial;
						sds_pm25_sum += pm25_serial;
						UPDATE_MIN_MAX(sds_pm10_min, sds_pm10_max, pm10_serial);
						UPDATE_MIN_MAX(sds_pm25_min, sds_pm25_max, pm25_serial);
						debug_outln_verbose(F("PM10 (sec.) : "), String(pm10_serial / 10.0f));
						debug_outln_verbose(F("PM2.5 (sec.): "), String(pm25_serial / 10.0f));
						sds_val_count++;
					}
				}
				debug_outln_verbose(F("/R "), F("SDS011"));
				SDS_waiting_for = SDS_REPLY_HDR_LOCAL;
				break;
			}
		}
	}
	if (send_now)
	{
		last_value_SDS_P1 = -1;
		last_value_SDS_P2 = -1;
		if (sds_val_count > 2)
		{
			sds_pm10_sum = sds_pm10_sum - sds_pm10_min - sds_pm10_max;
			sds_pm25_sum = sds_pm25_sum - sds_pm25_min - sds_pm25_max;
			sds_val_count = sds_val_count - 2;
		}
		if (sds_val_count > 0)
		{
			last_value_SDS_P1 = float(sds_pm10_sum) / (sds_val_count * 10.0f);
			last_value_SDS_P2 = float(sds_pm25_sum) / (sds_val_count * 10.0f);
			add_Value2Json(s, F("SDS_P1"), F("PM10:  "), last_value_SDS_P1);
			add_Value2Json(s, F("SDS_P2"), F("PM2.5: "), last_value_SDS_P2);
			debug_outln_info(F("----"));
			if (sds_val_count < 3)
			{
				SDS_error_count++;
			}
		}
		else
		{
			SDS_error_count++;
		}
		sds_pm10_sum = 0;
		sds_pm25_sum = 0;
		sds_val_count = 0;
		sds_pm10_max = 0;
		sds_pm10_min = 20000;
		sds_pm25_max = 0;
		sds_pm25_min = 20000;
		if ((cfg::sending_intervall_ms > (WARMUPTIME_SDS_MS + READINGTIME_SDS_MS)))
		{

			if (is_SDS_running)
			{
				is_SDS_running = SDS_cmd(PmSensorCmd::Stop);
			}
		}
	}
}

#include "../web/page_helpers.h"
#include "../html-content.h"

void render_sds011_values(String &page_content)
{
	add_table_pm_value(page_content, FPSTR(SENSORS_SDS011), FPSTR(WEB_PM25), last_value_SDS_P2);
	add_table_pm_value(page_content, FPSTR(SENSORS_SDS011), FPSTR(WEB_PM10), last_value_SDS_P1);
	page_content += FPSTR(EMPTY_ROW);
}
