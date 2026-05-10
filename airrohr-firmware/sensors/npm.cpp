/*
 * sensors/npm.cpp — hibbes-Patch (Issue #7 Refactor Phase 1)
 *
 * Liest den Tera Sensor Next PM-Sensor per UART (eigener serialNPM),
 * 16-Byte-Frame mit 0x81 0x11 Header, State+Body+Checksum-Blocks.
 *
 * Kontrakt: emittiert JSON-Schlüssel "NPM_P0/P1/P2" für Massenkonzent-
 * rationen und "NPM_N1/N10/N25" für Anzahlkonzentrationen.
 *
 * Globals werden weiterhin in airrohr-firmware.ino definiert; hier nur
 * `extern`-Verweise. Hilfsfunktionen `NPM_start_stop()` und
 * `NPM_temp_humi()` müssen vom Konsolidierungs-Schritt im .ino vom
 * `static`-Qualifier befreit werden, damit der Linker sie hier findet
 * (siehe DONE_WITH_CONCERNS-Bericht).
 */

#include "npm.h"
#include "../utils.h"
#include "../defines.h"

// Globals definiert in airrohr-firmware.ino
namespace cfg {
	extern unsigned sending_intervall_ms;
	extern bool npm_fulltime;
}

extern unsigned long act_milli;
extern unsigned long starttime;
extern bool send_now;

extern bool is_NPM_running;
// NPM_waiting_for_16 ist im .ino als anonyme-enum-Variable deklariert; wir
// greifen über int-kompatible extern-Decl darauf zu.
extern int NPM_waiting_for_16;

extern String current_state_npm;
extern String current_th_npm;

extern uint32_t npm_pm1_sum;
extern uint32_t npm_pm10_sum;
extern uint32_t npm_pm25_sum;
extern uint32_t npm_pm1_sum_pcs;
extern uint32_t npm_pm10_sum_pcs;
extern uint32_t npm_pm25_sum_pcs;
extern uint16_t npm_val_count;
extern uint16_t npm_pm1_max;
extern uint16_t npm_pm1_min;
extern uint16_t npm_pm10_max;
extern uint16_t npm_pm10_min;
extern uint16_t npm_pm25_max;
extern uint16_t npm_pm25_min;
extern uint16_t npm_pm1_max_pcs;
extern uint16_t npm_pm1_min_pcs;
extern uint16_t npm_pm10_max_pcs;
extern uint16_t npm_pm10_min_pcs;
extern uint16_t npm_pm25_max_pcs;
extern uint16_t npm_pm25_min_pcs;

extern float last_value_NPM_P0;
extern float last_value_NPM_P1;
extern float last_value_NPM_P2;
extern float last_value_NPM_N1;
extern float last_value_NPM_N10;
extern float last_value_NPM_N25;
extern unsigned long NPM_error_count;

// NPM-State-Machine-Konstanten lokal repliziert (Wire-Format)
static constexpr int NPM_REPLY_HEADER_16_LOCAL = 16;
static constexpr int NPM_REPLY_STATE_16_LOCAL = 14;
static constexpr int NPM_REPLY_BODY_16_LOCAL = 13;
static constexpr int NPM_REPLY_CHECKSUM_16_LOCAL = 1;

// Helfer im .ino definiert — müssen vom Konsolidierungs-Schritt
// vom `static`-Qualifier befreit werden.
extern bool NPM_start_stop();
extern String NPM_temp_humi();

// msSince-Macro lokal repliziert
#ifndef msSince
#define msSince(timestamp_before) (act_milli - (timestamp_before))
#endif

void fetchSensorNPM(String &s)
{

	debug_outln_verbose(F("R/ "), F("Tera Sensor Next PM"));
	if (cfg::sending_intervall_ms > (WARMUPTIME_NPM_MS + READINGTIME_NPM_MS) && msSince(starttime) < (cfg::sending_intervall_ms - (WARMUPTIME_NPM_MS + READINGTIME_NPM_MS)))
	{
		if (is_NPM_running && !cfg::npm_fulltime)
		{
			debug_outln_info(F("Change NPM to stop..."));
			is_NPM_running = NPM_start_stop();
		}
	}
	else
	{
		if (!is_NPM_running && !cfg::npm_fulltime)
		{
			debug_outln_info(F("Change NPM to start..."));
			is_NPM_running = NPM_start_stop();
			NPM_waiting_for_16 = NPM_REPLY_HEADER_16_LOCAL;
		}

		if (is_NPM_running && cfg::npm_fulltime)
		{
				NPM_waiting_for_16 = NPM_REPLY_HEADER_16_LOCAL;
		}


		if (msSince(starttime) > (cfg::sending_intervall_ms - READINGTIME_NPM_MS))
		{ //DIMINUER LE READING TIME

			debug_outln_info(F("Concentration NPM..."));
			NPM_cmd(PmSensorCmd2::Concentration);
			while (!serialNPM.available())
			{
				debug_outln("Wait for Serial...", DEBUG_MAX_INFO);
			}

			while (serialNPM.available() >= NPM_waiting_for_16)
			{
				const uint8_t constexpr header[2] = {0x81, 0x11};
				uint8_t state[1];
				uint8_t data[12];
				uint8_t checksum[1];
				uint8_t test[16];
				uint16_t N1_serial;
				uint16_t N25_serial;
				uint16_t N10_serial;
				uint16_t pm1_serial;
				uint16_t pm25_serial;
				uint16_t pm10_serial;

				switch (NPM_waiting_for_16)
				{
				case NPM_REPLY_HEADER_16_LOCAL:
					if (serialNPM.find(header, sizeof(header)))
						NPM_waiting_for_16 = NPM_REPLY_STATE_16_LOCAL;
					break;
				case NPM_REPLY_STATE_16_LOCAL:
					serialNPM.readBytes(state, sizeof(state));
					current_state_npm = NPM_state(state[0]);
					NPM_waiting_for_16 = NPM_REPLY_BODY_16_LOCAL;
					break;
				case NPM_REPLY_BODY_16_LOCAL:
					if (serialNPM.readBytes(data, sizeof(data)) == sizeof(data))
					{
						NPM_data_reader(data, 12);
						N1_serial = word(data[0], data[1]);
						N25_serial = word(data[2], data[3]);
						N10_serial = word(data[4], data[5]);

						pm1_serial = word(data[6], data[7]);
						pm25_serial = word(data[8], data[9]);
						pm10_serial = word(data[10], data[11]);

						debug_outln_info(F("Next PM Measure..."));

						debug_outln_verbose(F("PM1 (μg/m3) : "), String(pm1_serial / 10.0f));
						debug_outln_verbose(F("PM2.5 (μg/m3): "), String(pm25_serial / 10.0f));
						debug_outln_verbose(F("PM10 (μg/m3) : "), String(pm10_serial / 10.0f));

						debug_outln_verbose(F("PM1 (pcs/L) : "), String(N1_serial));
						debug_outln_verbose(F("PM2.5 (pcs/L): "), String(N25_serial));
						debug_outln_verbose(F("PM10 (pcs/L) : "), String(N10_serial));
					}
					NPM_waiting_for_16 = NPM_REPLY_CHECKSUM_16_LOCAL;
					break;
				case NPM_REPLY_CHECKSUM_16_LOCAL:
					serialNPM.readBytes(checksum, sizeof(checksum));
					memcpy(test, header, sizeof(header));
					memcpy(&test[sizeof(header)], state, sizeof(state));
					memcpy(&test[sizeof(header) + sizeof(state)], data, sizeof(data));
					memcpy(&test[sizeof(header) + sizeof(state) + sizeof(data)], checksum, sizeof(checksum));
					NPM_data_reader(test, 16);
					if (NPM_checksum_valid_16(test))
					{
							debug_outln_info(F("Checksum OK..."));
													npm_pm1_sum += pm1_serial;
						npm_pm25_sum += pm25_serial;
						npm_pm10_sum += pm10_serial;

						npm_pm1_sum_pcs += N1_serial;
						npm_pm25_sum_pcs += N25_serial;
						npm_pm10_sum_pcs += N10_serial;

						UPDATE_MIN_MAX(npm_pm1_min, npm_pm1_max, pm1_serial);
						UPDATE_MIN_MAX(npm_pm25_min, npm_pm25_max, pm25_serial);
						UPDATE_MIN_MAX(npm_pm10_min, npm_pm10_max, pm10_serial);

						UPDATE_MIN_MAX(npm_pm1_min_pcs, npm_pm1_max_pcs, N1_serial);
						UPDATE_MIN_MAX(npm_pm25_min_pcs, npm_pm25_max_pcs, N25_serial);
						UPDATE_MIN_MAX(npm_pm10_min_pcs, npm_pm10_max_pcs, N10_serial);

						npm_val_count++;
						debug_outln(String(npm_val_count), DEBUG_MAX_INFO);
					}
					NPM_waiting_for_16 = NPM_REPLY_HEADER_16_LOCAL;
					break;
				}
			}
		}
	}

	if (send_now)
	{
		last_value_NPM_P0 = -1.0f;
		last_value_NPM_P1 = -1.0f;
		last_value_NPM_P2 = -1.0f;
		last_value_NPM_N1 = -1.0f;
		last_value_NPM_N10 = -1.0f;
		last_value_NPM_N25 = -1.0f;

		if (npm_val_count > 2)
		{
			npm_pm1_sum = npm_pm1_sum - npm_pm1_min - npm_pm1_max;
			npm_pm10_sum = npm_pm10_sum - npm_pm10_min - npm_pm10_max;
			npm_pm25_sum = npm_pm25_sum - npm_pm25_min - npm_pm25_max;
			npm_pm1_sum_pcs = npm_pm1_sum_pcs - npm_pm1_min_pcs - npm_pm1_max_pcs;
			npm_pm10_sum_pcs = npm_pm10_sum_pcs - npm_pm10_min_pcs - npm_pm10_max_pcs;
			npm_pm25_sum_pcs = npm_pm25_sum_pcs - npm_pm25_min_pcs - npm_pm25_max_pcs;
			npm_val_count = npm_val_count - 2;
		}
		if (npm_val_count > 0)
		{
			last_value_NPM_P0 = float(npm_pm1_sum) / (npm_val_count * 10.0f);
			last_value_NPM_P1 = float(npm_pm10_sum) / (npm_val_count * 10.0f);
			last_value_NPM_P2 = float(npm_pm25_sum) / (npm_val_count * 10.0f);

			last_value_NPM_N1 = float(npm_pm1_sum_pcs) / (npm_val_count * 1000.0f);
			last_value_NPM_N10 = float(npm_pm10_sum_pcs) / (npm_val_count * 1000.0f);
			last_value_NPM_N25 = float(npm_pm25_sum_pcs) / (npm_val_count * 1000.0f);

			add_Value2Json(s, F("NPM_P0"), F("PM1: "), last_value_NPM_P0);
			add_Value2Json(s, F("NPM_P1"), F("PM10:  "), last_value_NPM_P1);
			add_Value2Json(s, F("NPM_P2"), F("PM2.5: "), last_value_NPM_P2);

			add_Value2Json(s, F("NPM_N1"), F("NC1.0: "), last_value_NPM_N1);
			add_Value2Json(s, F("NPM_N10"), F("NC10:  "), last_value_NPM_N10);
			add_Value2Json(s, F("NPM_N25"), F("NC2.5: "), last_value_NPM_N25);

			debug_outln_info(F("----"));
			if (npm_val_count < 3)
			{
				NPM_error_count++;
			}
		}
		else
		{
			NPM_error_count++;
		}

		npm_pm1_sum = 0;
		npm_pm10_sum = 0;
		npm_pm25_sum = 0;

		npm_val_count = 0;

		npm_pm1_max = 0;
		npm_pm1_min = 20000;
		npm_pm10_max = 0;
		npm_pm10_min = 20000;
		npm_pm25_max = 0;
		npm_pm25_min = 20000;

		npm_pm1_sum_pcs = 0;
		npm_pm10_sum_pcs = 0;
		npm_pm25_sum_pcs = 0;

		npm_pm1_max_pcs = 0;
		npm_pm1_min_pcs = 60000;
		npm_pm10_max_pcs = 0;
		npm_pm10_min_pcs = 60000;
		npm_pm25_max_pcs = 0;
		npm_pm25_min_pcs = 60000;

		if (cfg::sending_intervall_ms > (WARMUPTIME_NPM_MS + READINGTIME_NPM_MS))
		{
			debug_outln_info(F("Temperature and humidity in NPM after measure..."));
			current_th_npm = NPM_temp_humi();
			if (is_NPM_running && !cfg::npm_fulltime)
			{
				debug_outln_info(F("Change NPM to stop after measure..."));
				is_NPM_running = NPM_start_stop();
			}
		}
	}

	debug_outln_verbose(F("/R "), F("Tera Sensor Next PM"));
}
