/*
 * sensors/dnms.cpp — hibbes-Patch (Issue #7 Phase 1 Refactor)
 *
 * Triggert eine DNMS-Leq-Berechnung, pollt mit kleiner Latenz auf
 * data-ready und liest die A-bewerteten Schalldruckpegel
 * (LAeq/LA_min/LA_max) aus. Mit dem in cfg::dnms_correction
 * konfigurierten Offset korrigiert; Schreibt die Werte in das
 * übergebene JSON-String-Reservoir.
 *
 * Kontrakt: emittiert JSON-Schlüssel "DNMS_noise_LAeq",
 * "DNMS_noise_LA_min", "DNMS_noise_LA_max".
 *
 * Globals werden weiterhin in airrohr-firmware.ino definiert; hier
 * nur `extern`-Verweise.
 */

#include "dnms.h"
#include <Arduino.h>
#include "../dnms_i2c.h"
#include "../utils.h"

// Globals definiert in airrohr-firmware.ino
extern float last_value_dnms_laeq;
extern float last_value_dnms_la_min;
extern float last_value_dnms_la_max;

// cfg-Felder definiert im namespace cfg in airrohr-firmware.ino
namespace cfg {
	extern char dnms_correction[];
}

void fetchSensorDNMS(String &s)
{
	static bool dnms_error = false;
	debug_outln_verbose(F("R/ "), F("DNMS"));
	last_value_dnms_laeq = -1.0;
	last_value_dnms_la_min = -1.0;
	last_value_dnms_la_max = -1.0;

	if (dnms_calculate_leq() != 0)
	{
		dnms_error = true;
	}

	uint16_t data_ready = 0;
	dnms_error = true;

	for (unsigned i = 0; i < 20; i++)
	{
		delay(2);
		int16_t ret_dnms = dnms_read_data_ready(&data_ready);
		if ((ret_dnms == 0) && (data_ready != 0))
		{
			dnms_error = false;
			break;
		}
	}
	if (!dnms_error)
	{
		struct dnms_measurements dnms_values;
		if (dnms_read_leq(&dnms_values) == 0)
		{
			float dnms_corr_value = readCorrectionOffset(cfg::dnms_correction);
			last_value_dnms_laeq = dnms_values.leq_a + dnms_corr_value;
			last_value_dnms_la_min = dnms_values.leq_a_min + dnms_corr_value;
			last_value_dnms_la_max = dnms_values.leq_a_max + dnms_corr_value;
		}
		else
		{
			dnms_error = true;
		}
	}
	if (dnms_error)
	{
		dnms_reset(); // try to reset dnms
		debug_outln_error(F("DNMS read failed"));
	}
	else
	{
		add_Value2Json(s, F("DNMS_noise_LAeq"), F("LAeq: "), last_value_dnms_laeq);
		add_Value2Json(s, F("DNMS_noise_LA_min"), F("LA_MIN: "), last_value_dnms_la_min);
		add_Value2Json(s, F("DNMS_noise_LA_max"), F("LA_MAX: "), last_value_dnms_la_max);
	}
	debug_outln_info(F("----"));
	debug_outln_verbose(F("/R "), F("DNMS"));
}
