/*
 * sensors/dht22.cpp — hibbes-Patch (Issue #7 Phase 1 Refactor)
 *
 * Liest den DHT11/DHT22-Combo-Sensor (1-Wire/GPIO digital) per lokaler
 * DHT-Lib mit bis zu 5 Retry-Versuchen, schreibt die Werte in das
 * übergebene JSON-String-Reservoir und aktualisiert die globalen
 * last_value_DHT_T/H.
 *
 * Kontrakt: emittiert JSON-Schlüssel "temperature" und "humidity"
 * (kanonisches Default-Format, weil DHT22 historisch der "Standard"-
 * Sensor des airrohr-Stacks ist).
 *
 * Globals werden weiterhin in airrohr-firmware.ino definiert; hier nur
 * `extern`-Verweise.
 */

#include "dht22.h"
#include "../DHT.h"
#include "../utils.h"

// Globals definiert in airrohr-firmware.ino
extern DHT dht;
extern float last_value_DHT_T;
extern float last_value_DHT_H;

namespace cfg {
	extern char temp_correction[];
}

void fetchSensorDHT(String &s)
{
	debug_outln_verbose(F("Sensor reading "), F("DHT22"));

	// Check if valid number if non NaN (not a number) will be send.
	last_value_DHT_T = -128;
	last_value_DHT_H = -1;

	int count = 0;
	const int MAX_ATTEMPTS = 5;
	while ((count++ < MAX_ATTEMPTS))
	{
		auto t = dht.readTemperature();
		auto h = dht.readHumidity();
		if (isnan(t) || isnan(h))
		{
			delay(100);
			t = dht.readTemperature(false);
			h = dht.readHumidity();
		}
		if (isnan(t) || isnan(h))
		{
			debug_outln_error(F("DHT11/DHT22 read failed"));
		}
		else
		{
			last_value_DHT_T = t + readCorrectionOffset(cfg::temp_correction);
			last_value_DHT_H = h;
			add_Value2Json(s, F("temperature"), F("Temperature (°C): "), last_value_DHT_T);
			add_Value2Json(s, F("humidity"), F("Humidity (%): "), last_value_DHT_H);
			break;
		}
	}
	debug_outln_info(F("----"));

	debug_outln_verbose(F("Sensor end "), F("DHT22"));
}

#include "../web/page_helpers.h"
#include "../html-content.h"

void render_dht_values(String &page_content)
{
	add_table_t_value(page_content, FPSTR(SENSORS_DHT22), FPSTR(INTL_TEMPERATURE), last_value_DHT_T);
	add_table_h_value(page_content, FPSTR(SENSORS_DHT22), FPSTR(INTL_HUMIDITY), last_value_DHT_H);
	page_content += FPSTR(EMPTY_ROW);
}
