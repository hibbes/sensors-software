/*
 * sensors/ips.cpp — hibbes-Patch (Issue #7 Refactor Phase 1)
 *
 * Liest den Piera Systems IPS-7100-Sensor per UART (eigener serialIPS),
 * ASCII-CSV-Format mit "PC0.1," "PC0.3," ... "PM10," Schlüsseln. Parsed
 * per substring/indexOf-Logik wie im Upstream.
 *
 * Kontrakt: emittiert JSON-Schlüssel "IPS_P0/P1/P2/P01/P03/P05/P5" für
 * Massenkonzentrationen und "IPS_N1/N10/N25/N01/N03/N05/N5" für
 * Anzahlkonzentrationen.
 *
 * Globals werden weiterhin in airrohr-firmware.ino definiert; hier nur
 * `extern`-Verweise.
 */

#include "ips.h"
#include "../utils.h"
#include "../defines.h"

// Globals definiert in airrohr-firmware.ino
namespace cfg {
	extern unsigned sending_intervall_ms;
}

extern unsigned long act_milli;
extern unsigned long starttime;
extern bool send_now;

extern bool is_IPS_running;

extern float ips_pm01_sum;
extern float ips_pm03_sum;
extern float ips_pm05_sum;
extern float ips_pm1_sum;
extern float ips_pm25_sum;
extern float ips_pm5_sum;
extern float ips_pm10_sum;

extern unsigned long ips_pm01_sum_pcs;
extern unsigned long ips_pm03_sum_pcs;
extern unsigned long ips_pm05_sum_pcs;
extern unsigned long ips_pm1_sum_pcs;
extern unsigned long ips_pm25_sum_pcs;
extern unsigned long ips_pm5_sum_pcs;
extern unsigned long ips_pm10_sum_pcs;

extern uint16_t ips_val_count;

extern float ips_pm01_max;
extern float ips_pm01_min;
extern float ips_pm03_max;
extern float ips_pm03_min;
extern float ips_pm05_max;
extern float ips_pm05_min;
extern float ips_pm1_max;
extern float ips_pm1_min;
extern float ips_pm25_max;
extern float ips_pm25_min;
extern float ips_pm5_max;
extern float ips_pm5_min;
extern float ips_pm10_max;
extern float ips_pm10_min;

extern unsigned long ips_pm01_max_pcs;
extern unsigned long ips_pm01_min_pcs;
extern unsigned long ips_pm03_max_pcs;
extern unsigned long ips_pm03_min_pcs;
extern unsigned long ips_pm05_max_pcs;
extern unsigned long ips_pm05_min_pcs;
extern unsigned long ips_pm1_max_pcs;
extern unsigned long ips_pm1_min_pcs;
extern unsigned long ips_pm25_max_pcs;
extern unsigned long ips_pm25_min_pcs;
extern unsigned long ips_pm5_max_pcs;
extern unsigned long ips_pm5_min_pcs;
extern unsigned long ips_pm10_max_pcs;
extern unsigned long ips_pm10_min_pcs;

extern float last_value_IPS_P0;
extern float last_value_IPS_P1;
extern float last_value_IPS_P2;
extern float last_value_IPS_P01;
extern float last_value_IPS_P03;
extern float last_value_IPS_P05;
extern float last_value_IPS_P5;
extern float last_value_IPS_N1;
extern float last_value_IPS_N10;
extern float last_value_IPS_N25;
extern float last_value_IPS_N01;
extern float last_value_IPS_N03;
extern float last_value_IPS_N05;
extern float last_value_IPS_N5;
extern unsigned long IPS_error_count;

// msSince-Macro lokal repliziert
#ifndef msSince
#define msSince(timestamp_before) (act_milli - (timestamp_before))
#endif

void fetchSensorIPS(String &s)
{
	debug_outln_verbose(F("R/ "), F("Piera Systems IPS-7100"));

			while (serialIPS.available() > 0)
			{
				serialIPS.read();
			}


	if (cfg::sending_intervall_ms > (WARMUPTIME_IPS_MS + READINGTIME_IPS_MS) && msSince(starttime) < (cfg::sending_intervall_ms - (WARMUPTIME_IPS_MS + READINGTIME_IPS_MS)))
	{
		if (is_IPS_running)
		{
			debug_outln_info(F("Change IPS to stop..."));
			IPS_cmd(PmSensorCmd3::Stop);
			is_IPS_running = false;
		}
	}
	else
	{
		if (!is_IPS_running)
		{
			debug_outln_info(F("Change IPS to start..."));
			IPS_cmd(PmSensorCmd3::Start);
			is_IPS_running = true;
		}

		//VIDER LE BUFFER DU START?

		if (msSince(starttime) > (cfg::sending_intervall_ms - READINGTIME_IPS_MS))
		{ //DIMINUER LE READING TIME

			debug_outln_info(F("Concentration IPS..."));
			String serial_data;
			//String serial_data2;

			// while (Serial.available() > 0) {
			// 	Serial.read();
			// }

			IPS_cmd(PmSensorCmd3::Get);

			if (serialIPS.available() > 0)
			{
				serial_data = serialIPS.readString();
			}


			// while (serialIPS.available() > 0)
			// {
			// 	serialIPS.read();
			// }

		 Debug.println(serial_data);

// 		if(serial_data.length()>240){


	int index1 = serial_data.indexOf("PC0.1,");
	int index2 = serial_data.indexOf(",PC0.3,");
	int index3 = serial_data.indexOf(",PC0.5,");
	int index4 = serial_data.indexOf(",PC1.0,");
	int index5 = serial_data.indexOf(",PC2.5,");
	int index6 = serial_data.indexOf(",PC5.0,");
	int index7 = serial_data.indexOf(",PC10,");
	int index8 = serial_data.indexOf(",PM0.1,");
	int index9 = serial_data.indexOf(",PM0.3,");
	int index10 = serial_data.indexOf(",PM0.5,");
	int index11 = serial_data.indexOf(",PM1.0,");
	int index12 = serial_data.indexOf(",PM2.5,");
	int index13 = serial_data.indexOf(",PM5.0,");
	int index14 = serial_data.indexOf(",PM10,");
	int index15 = serial_data.indexOf(",IPS");

	String N01_serial = serial_data.substring(index1+6,index2);
	String N03_serial = serial_data.substring(index2+7,index3);
	String N05_serial = serial_data.substring(index3+7,index4);
	String N1_serial = serial_data.substring(index4+7,index5);
	String N25_serial = serial_data.substring(index5+7,index6);
	String N5_serial = serial_data.substring(index6+7,index7);
	String N10_serial = serial_data.substring(index7+6,index8);

	String pm01_serial = serial_data.substring(index8+7,index9-6);
	String pm03_serial = serial_data.substring(index9+7,index10-6);
	String pm05_serial = serial_data.substring(index10+7,index11-6);
	String pm1_serial = serial_data.substring(index11+7,index12-6);
	String pm25_serial = serial_data.substring(index12+7,index13-6);
	String pm5_serial = serial_data.substring(index13+7,index14-6);
	String pm10_serial = serial_data.substring(index14+6,index15-6);

	debug_outln_verbose(F("PM0.1 (μg/m3) : "), pm01_serial);
	debug_outln_verbose(F("PM0.3 (μg/m3): "), pm03_serial);
	debug_outln_verbose(F("PM0.5 (μg/m3) : "), pm05_serial);
	debug_outln_verbose(F("PM1 (μg/m3) : "), pm1_serial);
	debug_outln_verbose(F("PM2.5 (μg/m3): "), pm25_serial);
	debug_outln_verbose(F("PM5 (μg/m3) : "), pm5_serial);
	debug_outln_verbose(F("PM10 (μg/m3) : "), pm10_serial);

	debug_outln_verbose(F("PM0.1 (pcs/L) : "), N01_serial);
	debug_outln_verbose(F("PM0.3 (pcs/L): "), N03_serial);
	debug_outln_verbose(F("PM0.5(pcs/L) : "), N05_serial);
	debug_outln_verbose(F("PM1 (pcs/L) : "), N1_serial);
	debug_outln_verbose(F("PM2.5 (pcs/L): "), N25_serial);
	debug_outln_verbose(F("PM5 (pcs/L) : "), N5_serial);
	debug_outln_verbose(F("PM10 (pcs/L) : "), N10_serial);

	ips_pm01_sum += pm01_serial.toFloat();
	ips_pm03_sum += pm03_serial.toFloat();
	ips_pm05_sum += pm05_serial.toFloat();
	ips_pm1_sum += pm1_serial.toFloat();
	ips_pm25_sum += pm25_serial.toFloat();
	ips_pm5_sum += pm5_serial.toFloat();
	ips_pm10_sum += pm10_serial.toFloat();

	//char *ptr;

    // SI EXCEPTION 28 DECLENCHÈE FLASHER SUR AUTRE PRISE USB.

	ips_pm01_sum_pcs += strtoul(N01_serial.c_str(),NULL,10);
	ips_pm03_sum_pcs += strtoul(N03_serial.c_str(),NULL,10);
	ips_pm05_sum_pcs += strtoul(N05_serial.c_str(),NULL,10);
	ips_pm1_sum_pcs += strtoul(N1_serial.c_str(),NULL,10);
	ips_pm25_sum_pcs += strtoul(N25_serial.c_str(),NULL,10);
	ips_pm5_sum_pcs += strtoul(N5_serial.c_str(),NULL,10);
	ips_pm10_sum_pcs += strtoul(N10_serial.c_str(),NULL,10);

	UPDATE_MIN_MAX(ips_pm01_min, ips_pm01_max, pm01_serial.toFloat());
	UPDATE_MIN_MAX(ips_pm03_min, ips_pm03_max, pm03_serial.toFloat());
	UPDATE_MIN_MAX(ips_pm05_min, ips_pm05_max, pm05_serial.toFloat());
	UPDATE_MIN_MAX(ips_pm1_min, ips_pm1_max, pm1_serial.toFloat());
	UPDATE_MIN_MAX(ips_pm25_min, ips_pm25_max, pm25_serial.toFloat());
	UPDATE_MIN_MAX(ips_pm5_min, ips_pm5_max, pm5_serial.toFloat());
	UPDATE_MIN_MAX(ips_pm10_min, ips_pm10_max, pm10_serial.toFloat());

	UPDATE_MIN_MAX(ips_pm01_min_pcs, ips_pm01_max_pcs, strtoul(N01_serial.c_str(),NULL,10));
	UPDATE_MIN_MAX(ips_pm03_min_pcs, ips_pm03_max_pcs, strtoul(N03_serial.c_str(),NULL,10));
	UPDATE_MIN_MAX(ips_pm05_min_pcs, ips_pm05_max_pcs, strtoul(N05_serial.c_str(),NULL,10));
	UPDATE_MIN_MAX(ips_pm1_min_pcs, ips_pm1_max_pcs, strtoul(N1_serial.c_str(),NULL,10));
	UPDATE_MIN_MAX(ips_pm25_min_pcs, ips_pm25_max_pcs, strtoul(N25_serial.c_str(),NULL,10));
	UPDATE_MIN_MAX(ips_pm5_min_pcs, ips_pm5_max_pcs, strtoul(N5_serial.c_str(),NULL,10));
	UPDATE_MIN_MAX(ips_pm10_min_pcs, ips_pm10_max_pcs, strtoul(N10_serial.c_str(),NULL,10));

	debug_outln_info(F("IPS Measure..."));
	ips_val_count++;
	debug_outln(String(ips_val_count), DEBUG_MAX_INFO);
	}
}

	if (send_now)
	{

		last_value_IPS_P0 = -1.0; //PM1
		last_value_IPS_P1 = -1.0; //PM10
		last_value_IPS_P2 = -1.0; //PM2.5
		last_value_IPS_P01 = -1.0; //PM0.1
		last_value_IPS_P03 = -1.0; //PM0.3
		last_value_IPS_P05 = -1.0; //PM0.5
		last_value_IPS_P5 = -1.0; //PM5
		last_value_IPS_N1 = -1.0;
		last_value_IPS_N10 = -1.0;
		last_value_IPS_N25 = -1.0;
		last_value_IPS_N01 = -1.0;
		last_value_IPS_N03 = -1.0;
		last_value_IPS_N05 = -1.0;
		last_value_IPS_N5 = -1.0;

		if (ips_val_count > 2)
		{
			ips_pm01_sum = ips_pm01_sum - ips_pm01_min - ips_pm01_max;
			ips_pm03_sum = ips_pm03_sum - ips_pm03_min - ips_pm03_max;
			ips_pm05_sum = ips_pm05_sum - ips_pm05_min - ips_pm05_max;
			ips_pm1_sum = ips_pm1_sum - ips_pm1_min - ips_pm1_max;
			ips_pm25_sum = ips_pm25_sum - ips_pm25_min - ips_pm25_max;
			ips_pm5_sum = ips_pm5_sum - ips_pm5_min - ips_pm5_max;
			ips_pm10_sum = ips_pm10_sum - ips_pm10_min - ips_pm10_max;

			ips_pm01_sum_pcs = ips_pm01_sum_pcs - ips_pm01_min_pcs - ips_pm01_max_pcs;
			ips_pm03_sum_pcs = ips_pm03_sum_pcs - ips_pm03_min_pcs - ips_pm03_max_pcs;
			ips_pm05_sum_pcs = ips_pm05_sum_pcs - ips_pm05_min_pcs - ips_pm05_max_pcs;
			ips_pm1_sum_pcs = ips_pm1_sum_pcs - ips_pm1_min_pcs - ips_pm1_max_pcs;
			ips_pm25_sum_pcs = ips_pm25_sum_pcs - ips_pm25_min_pcs - ips_pm25_max_pcs;
			ips_pm5_sum_pcs = ips_pm5_sum_pcs - ips_pm5_min_pcs - ips_pm5_max_pcs;
			ips_pm10_sum_pcs = ips_pm10_sum_pcs - ips_pm10_min_pcs - ips_pm10_max_pcs;

			ips_val_count = ips_val_count - 2;
		}
		if (ips_val_count > 0)
		{
			last_value_IPS_P0 = float(ips_pm1_sum) / (ips_val_count);
			last_value_IPS_P1 = float(ips_pm10_sum) / (ips_val_count);
			last_value_IPS_P2 = float(ips_pm25_sum) / (ips_val_count);
			last_value_IPS_P01 = float(ips_pm01_sum) / (ips_val_count);
			last_value_IPS_P03 = float(ips_pm03_sum) / (ips_val_count);
			last_value_IPS_P05 = float(ips_pm05_sum) / (ips_val_count);
			last_value_IPS_P5 = float(ips_pm5_sum) / (ips_val_count);

			last_value_IPS_N1 = float(ips_pm1_sum_pcs) / (ips_val_count * 1000.0f);
			last_value_IPS_N10 = float(ips_pm10_sum_pcs) / (ips_val_count * 1000.0f);
			last_value_IPS_N25 = float(ips_pm25_sum_pcs) / (ips_val_count * 1000.0f);
			last_value_IPS_N01 = float(ips_pm01_sum_pcs) / (ips_val_count * 1000.0f);
			last_value_IPS_N03 = float(ips_pm03_sum_pcs) / (ips_val_count * 1000.0f);
			last_value_IPS_N05 = float(ips_pm05_sum_pcs) / (ips_val_count * 1000.0f);
			last_value_IPS_N5 = float(ips_pm5_sum_pcs) / (ips_val_count * 1000.0f);

			add_Value2Json(s, F("IPS_P0"), F("PM1: "), last_value_IPS_P0);
			add_Value2Json(s, F("IPS_P1"), F("PM10:  "), last_value_IPS_P1);
			add_Value2Json(s, F("IPS_P2"), F("PM2.5: "), last_value_IPS_P2);
			add_Value2Json(s, F("IPS_P01"), F("PM0.1: "), last_value_IPS_P01);
			add_Value2Json(s, F("IPS_P03"), F("PM0.3:  "), last_value_IPS_P03);
			add_Value2Json(s, F("IPS_P05"), F("PM0.5: "), last_value_IPS_P05);
			add_Value2Json(s, F("IPS_P5"), F("PM5: "), last_value_IPS_P5);

			add_Value2Json(s, F("IPS_N1"), F("NC1.0: "), last_value_IPS_N1);
			add_Value2Json(s, F("IPS_N10"), F("NC10:  "), last_value_IPS_N10);
			add_Value2Json(s, F("IPS_N25"), F("NC2.5: "), last_value_IPS_N25);
			add_Value2Json(s, F("IPS_N01"), F("NC0.1: "), last_value_IPS_N01);
			add_Value2Json(s, F("IPS_N03"), F("NC0.3:  "), last_value_IPS_N03);
			add_Value2Json(s, F("IPS_N05"), F("NC0.5: "), last_value_IPS_N05);
			add_Value2Json(s, F("IPS_N5"), F("NC5: "), last_value_IPS_N5);


			debug_outln_info(F("----"));
			if (ips_val_count < 3)
			{
				IPS_error_count++;
			}
		}
		else
		{
			IPS_error_count++;
		}

		ips_pm01_sum = 0;
		ips_pm03_sum = 0;
		ips_pm05_sum = 0;
		ips_pm1_sum = 0;
		ips_pm25_sum = 0;
		ips_pm5_sum = 0;
		ips_pm10_sum = 0;

		ips_val_count = 0;

		ips_pm01_max = 0;
		ips_pm01_min = 200;
		ips_pm03_max = 0;
		ips_pm03_min = 200;
		ips_pm05_max = 0;
		ips_pm05_min = 200;
		ips_pm1_max = 0;
		ips_pm1_min = 200;
		ips_pm25_max = 0;
		ips_pm25_min = 200;
		ips_pm5_max = 0;
		ips_pm5_min = 200;
		ips_pm10_max = 0;
		ips_pm10_min = 200;

		ips_pm01_sum_pcs = 0;
		ips_pm03_sum_pcs = 0;
		ips_pm05_sum_pcs = 0;
		ips_pm1_sum_pcs = 0;
		ips_pm25_sum_pcs = 0;
		ips_pm5_sum_pcs = 0;
		ips_pm10_sum_pcs = 0;

		ips_pm01_max_pcs = 0;
		ips_pm01_min_pcs = 4000000000;
		ips_pm03_max_pcs = 0;
		ips_pm03_min_pcs = 4000000000;
		ips_pm05_max_pcs = 0;
		ips_pm05_min_pcs = 4000000000;
		ips_pm1_max_pcs = 0;
		ips_pm1_min_pcs = 4000000000;
		ips_pm25_max_pcs = 0;
		ips_pm25_min_pcs = 4000000000;
		ips_pm5_max_pcs = 0;
		ips_pm5_min_pcs = 4000000000;
		ips_pm10_max_pcs = 0;
		ips_pm10_min_pcs = 4000000000;

		if (cfg::sending_intervall_ms > (WARMUPTIME_IPS_MS + READINGTIME_IPS_MS))
		{
			if (is_IPS_running)
			{
				debug_outln_info(F("Change IPS to stop after measure..."));
				IPS_cmd(PmSensorCmd3::Stop);
				is_IPS_running = false;
			}
		}
	}

	debug_outln_verbose(F("/R "), F("Piera Systems IPS-7100"));

}
