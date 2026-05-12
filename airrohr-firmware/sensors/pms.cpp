/*
 * sensors/pms.cpp — hibbes-Patch (Issue #7 Refactor Phase 1)
 *
 * Liest den Plantower PMSx003 PM-Sensor per UART (serialSDS — geteilte
 * UART mit SDS/HPM, nur einer kann gleichzeitig aktiv sein), Frame-
 * Format mit 0x42 0x4D Header und 24-/32-Byte-Body + Checksum.
 *
 * Kontrakt: emittiert JSON-Schlüssel "PMS_P0" (PM1), "PMS_P1" (PM10),
 * "PMS_P2" (PM2.5).
 *
 * Globals werden weiterhin in airrohr-firmware.ino definiert; hier nur
 * `extern`-Verweise.
 */

#include "pms.h"
#include "../utils.h"
#include "../defines.h"

// Globals definiert in airrohr-firmware.ino
namespace cfg {
	extern unsigned sending_intervall_ms;
}

extern unsigned long act_milli;
extern unsigned long starttime;
extern bool send_now;

extern bool is_PMS_running;

extern int pms_pm1_sum;
extern int pms_pm10_sum;
extern int pms_pm25_sum;
extern int pms_val_count;
extern int pms_pm1_max;
extern int pms_pm1_min;
extern int pms_pm10_max;
extern int pms_pm10_min;
extern int pms_pm25_max;
extern int pms_pm25_min;

extern float last_value_PMS_P0;
extern float last_value_PMS_P1;
extern float last_value_PMS_P2;

// msSince-Macro lokal repliziert
#ifndef msSince
#define msSince(timestamp_before) (act_milli - (timestamp_before))
#endif

void fetchSensorPMS(String &s)
{
	char buffer;
	int value;
	int len = 0;
	int pm1_serial = 0;
	int pm10_serial = 0;
	int pm25_serial = 0;
	int checksum_is = 0;
	int checksum_should = 0;
	bool checksum_ok = false;
	int frame_len = 24; // min. frame length

	debug_outln_verbose(F("R/ "), F("PMSx003"));
	if (msSince(starttime) < (cfg::sending_intervall_ms - (WARMUPTIME_SDS_MS + READINGTIME_SDS_MS)))
	{
		if (is_PMS_running)
		{
			is_PMS_running = PMS_cmd(PmSensorCmd::Stop);
		}
	}
	else
	{
		if (!is_PMS_running)
		{
			is_PMS_running = PMS_cmd(PmSensorCmd::Start);
		}

		while (serialSDS.available() > 0)
		{
			buffer = serialSDS.read();
			debug_outln(String(len) + " - " + String(buffer, DEC) + " - " + String(buffer, HEX) + " - " + int(buffer) + " .", DEBUG_MAX_INFO);
			//			"aa" = 170, "ab" = 171, "c0" = 192
			value = int(buffer);
			switch (len)
			{
			case 0:
				if (value != 66)
				{
					len = -1;
				};
				break;
			case 1:
				if (value != 77)
				{
					len = -1;
				};
				break;
			case 2:
				checksum_is = value;
				break;
			case 3:
				frame_len = value + 4;
				break;
			case 10:
				pm1_serial += (value << 8);
				break;
			case 11:
				pm1_serial += value;
				break;
			case 12:
				pm25_serial = (value << 8);
				break;
			case 13:
				pm25_serial += value;
				break;
			case 14:
				pm10_serial = (value << 8);
				break;
			case 15:
				pm10_serial += value;
				break;
			case 22:
				if (frame_len == 24)
				{
					checksum_should = (value << 8);
				};
				break;
			case 23:
				if (frame_len == 24)
				{
					checksum_should += value;
				};
				break;
			case 30:
				checksum_should = (value << 8);
				break;
			case 31:
				checksum_should += value;
				break;
			}
			if ((len > 2) && (len < (frame_len - 2)))
			{
				checksum_is += value;
			}
			len++;
			if (len == frame_len)
			{
				debug_outln_verbose(F("Checksum is: "), String(checksum_is + 143));
				debug_outln_verbose(F("Checksum should: "), String(checksum_should));
				if (checksum_should == (checksum_is + 143))
				{
					checksum_ok = true;
				}
				else
				{
					len = 0;
				};
				if (checksum_ok && (msSince(starttime) > (cfg::sending_intervall_ms - READINGTIME_SDS_MS)))
				{
					if ((!isnan(pm1_serial)) && (!isnan(pm10_serial)) && (!isnan(pm25_serial)))
					{
						pms_pm1_sum += pm1_serial;
						pms_pm10_sum += pm10_serial;
						pms_pm25_sum += pm25_serial;
						UPDATE_MIN_MAX(pms_pm1_min, pms_pm1_max, pm1_serial);
						UPDATE_MIN_MAX(pms_pm25_min, pms_pm25_max, pm25_serial);
						UPDATE_MIN_MAX(pms_pm10_min, pms_pm10_max, pm10_serial);
						debug_outln_verbose(F("PM1 (sec.): "), String(pm1_serial));
						debug_outln_verbose(F("PM2.5 (sec.): "), String(pm25_serial));
						debug_outln_verbose(F("PM10 (sec.) : "), String(pm10_serial));
						pms_val_count++;
					}
					len = 0;
					checksum_ok = false;
					pm1_serial = 0;
					pm10_serial = 0;
					pm25_serial = 0;
					checksum_is = 0;
				}
			}
			yield();
		}
	}

	if (send_now)
	{
		last_value_PMS_P0 = -1;
		last_value_PMS_P1 = -1;
		last_value_PMS_P2 = -1;
		if (pms_val_count > 2)
		{
			pms_pm1_sum = pms_pm1_sum - pms_pm1_min - pms_pm1_max;
			pms_pm10_sum = pms_pm10_sum - pms_pm10_min - pms_pm10_max;
			pms_pm25_sum = pms_pm25_sum - pms_pm25_min - pms_pm25_max;
			pms_val_count = pms_val_count - 2;
		}
		if (pms_val_count > 0)
		{
			last_value_PMS_P0 = float(pms_pm1_sum) / float(pms_val_count);
			last_value_PMS_P1 = float(pms_pm10_sum) / float(pms_val_count);
			last_value_PMS_P2 = float(pms_pm25_sum) / float(pms_val_count);
			add_Value2Json(s, F("PMS_P0"), F("PM1:   "), last_value_PMS_P0);
			add_Value2Json(s, F("PMS_P1"), F("PM10:  "), last_value_PMS_P1);
			add_Value2Json(s, F("PMS_P2"), F("PM2.5: "), last_value_PMS_P2);
			debug_outln_info(F("----"));
		}
		pms_pm1_sum = 0;
		pms_pm10_sum = 0;
		pms_pm25_sum = 0;
		pms_val_count = 0;
		pms_pm1_max = 0;
		pms_pm1_min = 20000;
		pms_pm10_max = 0;
		pms_pm10_min = 20000;
		pms_pm25_max = 0;
		pms_pm25_min = 20000;
		if (cfg::sending_intervall_ms > (WARMUPTIME_SDS_MS + READINGTIME_SDS_MS))
		{
			is_PMS_running = PMS_cmd(PmSensorCmd::Stop);
		}
	}

	debug_outln_verbose(F("/R "), F("PMSx003"));
}

#include "../web/page_helpers.h"
#include "../html-content.h"

void render_pms_values(String &page_content)
{
	add_table_pm_value(page_content, FPSTR(SENSORS_PMSx003), FPSTR(WEB_PM1), last_value_PMS_P0);
	add_table_pm_value(page_content, FPSTR(SENSORS_PMSx003), FPSTR(WEB_PM25), last_value_PMS_P2);
	add_table_pm_value(page_content, FPSTR(SENSORS_PMSx003), FPSTR(WEB_PM10), last_value_PMS_P1);
	page_content += FPSTR(EMPTY_ROW);
}
