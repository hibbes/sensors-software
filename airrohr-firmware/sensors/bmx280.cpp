/*
 * sensors/bmx280.cpp — hibbes-Patch (Issue #7 Phase 1 Refactor)
 *
 * Liest den Bosch BMP280/BME280 (I²C 0x76/0x77) per lokaler bmx280_i2c-
 * Lib, schreibt die Werte in das übergebene JSON-String-Reservoir und
 * aktualisiert die globalen last_value_BMX280_T/P sowie (nur BME280)
 * last_value_BME280_H. Hardware-Variante wird zur Laufzeit über
 * bmx280.sensorID() == BME280_SENSOR_ID erkannt.
 *
 * Kontrakt:
 *   - BME280-Branch: emittiert "BME280_temperature", "BME280_pressure",
 *     "BME280_humidity".
 *   - BMP280-Branch (Maskerade): emittiert "BMP_pressure" und
 *     "BMP_temperature" (BMP180-Format!), weil Sensor.Community +
 *     OpenSenseMap-luftdaten-Parser keine BMP280-Keys kennen, BMP_-
 *     Präfix aber akzeptieren. fein2wunder routet via p=3-Fallback
 *     korrekt auf BMP_pressure.
 *
 * Globals werden weiterhin in airrohr-firmware.ino definiert; hier nur
 * `extern`-Verweise.
 */

#include "bmx280.h"
#include "../bmx280_i2c.h"
#include "../utils.h"

// Globals definiert in airrohr-firmware.ino
extern BMX280 bmx280;
extern float last_value_BMX280_T;
extern float last_value_BMX280_P;
extern float last_value_BME280_H;

namespace cfg {
	extern char temp_correction[];
}

void fetchSensorBMX280(String &s)
{
	const __FlashStringHelper *const sensor_label =
		(bmx280.sensorID() == BME280_SENSOR_ID) ? F("BME280") : F("BMP280");
	debug_outln_verbose(F("Sensor reading "), sensor_label);

	bmx280.takeForcedMeasurement();
	const auto t = bmx280.readTemperature();
	const auto p = bmx280.readPressure();
	const auto h = bmx280.readHumidity();
	if (isnan(t) || isnan(p))
	{
		last_value_BMX280_T = -128.0;
		last_value_BMX280_P = -1.0;
		last_value_BME280_H = -1.0;
		debug_outln_error(F("BMP/BME280 read failed"));
	}
	else
	{
		last_value_BMX280_T = t + readCorrectionOffset(cfg::temp_correction);
		last_value_BMX280_P = p;
		if (bmx280.sensorID() == BME280_SENSOR_ID)
		{
			add_Value2Json(s, F("BME280_temperature"), F("Temperature (°C): "), last_value_BMX280_T);
			add_Value2Json(s, F("BME280_pressure"), F("Pressure (Pa): "), last_value_BMX280_P);
			last_value_BME280_H = h;
			add_Value2Json(s, F("BME280_humidity"), F("Humidity (%): "), last_value_BME280_H);
		}
		else
		{
			// Maskerade: BMP_-Prefix (BMP180-Format) statt BMP280_* — Sensor.Community + OpenSenseMap
			// luftdaten-Parser kennen keine BMP280-Keys, akzeptieren aber BMP_pressure/BMP_temperature.
			// fein2wunder routet weiter via p=3-Fallback auf BMP_pressure korrekt.
			add_Value2Json(s, F("BMP_pressure"), F("Pressure (Pa): "), last_value_BMX280_P);
			add_Value2Json(s, F("BMP_temperature"), F("Temperature (°C): "), last_value_BMX280_T);
		}
	}
	debug_outln_info(F("----"));
	debug_outln_verbose(F("Sensor end "), sensor_label);
}
