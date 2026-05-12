/*
 * sensors/hpm.cpp — hibbes-Patch (Issue #7 Refactor Phase 1)
 *
 * Liest den Honeywell HPM PM-Sensor per UART (serialSDS — geteilte UART
 * mit SDS/PMS), 32-Byte-Frame mit 0x42 0x4D Header.
 *
 * Kontrakt: emittiert JSON-Schlüssel "HPM_P1" (entspricht der Variable
 * pm10_sum/PM10-Wertegruppe — Zuordnung nach Originalcode-Verhalten,
 * Naming-Quirk vom Upstream übernommen) und "HPM_P2".
 *
 * Globals werden weiterhin in airrohr-firmware.ino definiert; hier nur
 * `extern`-Verweise.
 */

#include "hpm.h"
#include "../utils.h"
#include "../defines.h"

// Globals definiert in airrohr-firmware.ino
namespace cfg {
	extern unsigned sending_intervall_ms;
}

extern unsigned long act_milli;
extern unsigned long starttime;
extern bool send_now;

extern bool is_HPM_running;

extern int hpm_pm10_sum;
extern int hpm_pm25_sum;
extern int hpm_val_count;
extern int hpm_pm10_max;
extern int hpm_pm10_min;
extern int hpm_pm25_max;
extern int hpm_pm25_min;

extern float last_value_HPM_P1;
extern float last_value_HPM_P2;

// msSince-Macro lokal repliziert
#ifndef msSince
#define msSince(timestamp_before) (act_milli - (timestamp_before))
#endif

void fetchSensorHPM(String &s)
{
	char buffer;
	int value;
	int len = 0;
	int pm10_serial = 0;
	int pm25_serial = 0;
	int checksum_is = 0;
	int checksum_should = 0;
	bool checksum_ok = false;

	debug_outln_verbose(F("R/ "), F("Honeywell PM"));
	if (msSince(starttime) < (cfg::sending_intervall_ms - (WARMUPTIME_SDS_MS + READINGTIME_SDS_MS)))
	{
		if (is_HPM_running)
		{
			is_HPM_running = HPM_cmd(PmSensorCmd::Stop);
		}
	}
	else
	{
		if (!is_HPM_running)
		{
			is_HPM_running = HPM_cmd(PmSensorCmd::Start);
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
			case 6:
				pm25_serial += (value << 8);
				break;
			case 7:
				pm25_serial += value;
				break;
			case 8:
				pm10_serial = (value << 8);
				break;
			case 9:
				pm10_serial += value;
				break;
			case 30:
				checksum_should = (value << 8);
				break;
			case 31:
				checksum_should += value;
				break;
			}
			if (len > 2 && len < 30)
			{
				checksum_is += value;
			}
			len++;
			if (len == 32)
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
				if (checksum_ok && (long(msSince(starttime)) > (long(cfg::sending_intervall_ms) - long(READINGTIME_SDS_MS))))
				{
					if ((!isnan(pm10_serial)) && (!isnan(pm25_serial)))
					{
						hpm_pm10_sum += pm10_serial;
						hpm_pm25_sum += pm25_serial;
						UPDATE_MIN_MAX(hpm_pm10_min, hpm_pm10_max, pm10_serial);
						UPDATE_MIN_MAX(hpm_pm25_min, hpm_pm25_max, pm25_serial);
						debug_outln_verbose(F("PM2.5 (sec.): "), String(pm25_serial));
						debug_outln_verbose(F("PM10 (sec.) : "), String(pm10_serial));
						hpm_val_count++;
					}
					len = 0;
					checksum_ok = false;
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
		last_value_HPM_P1 = -1.0f;
		last_value_HPM_P2 = -1.0f;
		if (hpm_val_count > 2)
		{
			hpm_pm10_sum = hpm_pm10_sum - hpm_pm10_min - hpm_pm10_max;
			hpm_pm25_sum = hpm_pm25_sum - hpm_pm25_min - hpm_pm25_max;
			hpm_val_count = hpm_val_count - 2;
		}
		if (hpm_val_count > 0)
		{
			last_value_HPM_P1 = float(hpm_pm10_sum) / float(hpm_val_count);
			last_value_HPM_P2 = float(hpm_pm25_sum) / float(hpm_val_count);
			add_Value2Json(s, F("HPM_P1"), F("PM2.5: "), last_value_HPM_P1);
			add_Value2Json(s, F("HPM_P2"), F("PM10:  "), last_value_HPM_P2);
			debug_outln_info(F("----"));
		}
		hpm_pm10_sum = 0;
		hpm_pm25_sum = 0;
		hpm_val_count = 0;
		hpm_pm10_max = 0;
		hpm_pm10_min = 20000;
		hpm_pm25_max = 0;
		hpm_pm25_min = 20000;
		if (cfg::sending_intervall_ms > (WARMUPTIME_SDS_MS + READINGTIME_SDS_MS))
		{
			is_HPM_running = HPM_cmd(PmSensorCmd::Stop);
		}
	}

	debug_outln_verbose(F("/R "), F("Honeywell PM"));
}

#include "../web/page_helpers.h"
#include "../html-content.h"

void render_hpm_values(String &page_content)
{
	add_table_pm_value(page_content, FPSTR(SENSORS_HPM), FPSTR(WEB_PM25), last_value_HPM_P2);
	add_table_pm_value(page_content, FPSTR(SENSORS_HPM), FPSTR(WEB_PM10), last_value_HPM_P1);
	page_content += FPSTR(EMPTY_ROW);
}
