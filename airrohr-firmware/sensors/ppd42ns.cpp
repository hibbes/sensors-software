/*
 * sensors/ppd42ns.cpp — hibbes-Patch (Issue #7 Phase 1 Refactor)
 *
 * Sammelt fortlaufend die LOW-Pulse-Occupancy-Werte zweier GPIO-Pins
 * des Shinyei-PPD42NS und rechnet beim Sende-Tick (`send_now`) per
 * Spec-Sheet-Kurve in Konzentrationen um. Schreibt die Werte in das
 * übergebene JSON-String-Reservoir und aktualisiert die globalen
 * last_value_PPD_P1/P2.
 *
 * Kontrakt: emittiert JSON-Schlüssel "durP1", "ratioP1", "P1",
 * "durP2", "ratioP2", "P2".
 *
 * ACHTUNG (Consolidation-Pass-TODO): Die Pin-Konstanten PPD_PIN_PM1
 * und PPD_PIN_PM2 stammen aus ext_def.h, das nicht-static-PROGMEM-
 * Definitionen enthält und daher nur in EINEM TU includebar ist.
 * Dieses Modul greift deshalb auf zwei extern-int-Konstanten
 * (ppd_pin_pm1/ppd_pin_pm2) zu, die der Controller im Consolidation-
 * Pass in airrohr-firmware.ino aus den Macros gespiegelt anlegt:
 *   const int ppd_pin_pm1 = PPD_PIN_PM1;
 *   const int ppd_pin_pm2 = PPD_PIN_PM2;
 * Globals werden weiterhin in airrohr-firmware.ino definiert.
 */

#include "ppd42ns.h"
#include <Arduino.h>
#include "../defines.h"
#include "../utils.h"

// Globals definiert in airrohr-firmware.ino
extern float last_value_PPD_P1;
extern float last_value_PPD_P2;
extern unsigned long lowpulseoccupancyP1;
extern unsigned long lowpulseoccupancyP2;
extern boolean trigP1;
extern boolean trigP2;
extern unsigned long trigOnP1;
extern unsigned long trigOnP2;
extern bool send_now;
extern unsigned long starttime;
extern unsigned long act_micro;
extern unsigned long act_milli;

// Pin-Konstanten — vom Controller in airrohr-firmware.ino aus
// PPD_PIN_PM1/PM2-Macros gespiegelt (siehe Kopf-Kommentar).
extern const int ppd_pin_pm1;
extern const int ppd_pin_pm2;

// msSince ist in airrohr-firmware.ino als Macro definiert; hier lokal
// nachbilden, damit das Modul eigenständig übersetzt.
#ifndef msSince
#define msSince(timestamp_before) (act_milli - (timestamp_before))
#endif

void fetchSensorPPD(String &s)
{
	debug_outln_verbose(F("R/ "), F("PPD42NS"));

	if (msSince(starttime) <= SAMPLETIME_MS)
	{

		// Read pins connected to ppd42ns
		boolean valP1 = digitalRead(ppd_pin_pm1);
		boolean valP2 = digitalRead(ppd_pin_pm2);

		if (valP1 == LOW && trigP1 == false)
		{
			trigP1 = true;
			trigOnP1 = act_micro;
		}

		if (valP1 == HIGH && trigP1 == true)
		{
			lowpulseoccupancyP1 += act_micro - trigOnP1;
			trigP1 = false;
		}

		if (valP2 == LOW && trigP2 == false)
		{
			trigP2 = true;
			trigOnP2 = act_micro;
		}

		if (valP2 == HIGH && trigP2 == true)
		{
			unsigned long durationP2 = act_micro - trigOnP2;
			lowpulseoccupancyP2 += durationP2;
			trigP2 = false;
		}
	}
	// Checking if it is time to sample
	if (send_now)
	{
		auto calcConcentration = [](const float ratio)
		{
			/* spec sheet curve*/
			return (1.1f * ratio * ratio * ratio - 3.8f * ratio * ratio + 520.0f * ratio + 0.62f);
		};

		last_value_PPD_P1 = -1;
		last_value_PPD_P2 = -1;
		float ratio = lowpulseoccupancyP1 / (SAMPLETIME_MS * 10.0f);
		float concentration = calcConcentration(ratio);

		// json for push to api / P1
		last_value_PPD_P1 = concentration;
		add_Value2Json(s, F("durP1"), F("LPO P10    : "), lowpulseoccupancyP1);
		add_Value2Json(s, F("ratioP1"), F("Ratio PM10%: "), ratio);
		add_Value2Json(s, F("P1"), F("PM10 Count : "), last_value_PPD_P1);

		ratio = lowpulseoccupancyP2 / (SAMPLETIME_MS * 10.0f);
		concentration = calcConcentration(ratio);

		// json for push to api / P2
		last_value_PPD_P2 = concentration;
		add_Value2Json(s, F("durP2"), F("LPO PM25   : "), lowpulseoccupancyP2);
		add_Value2Json(s, F("ratioP2"), F("Ratio PM25%: "), ratio);
		add_Value2Json(s, F("P2"), F("PM25 Count : "), last_value_PPD_P2);

		debug_outln_info(F("----"));
	}

	debug_outln_verbose(F("/R "), F("PPD42NS"));
}
