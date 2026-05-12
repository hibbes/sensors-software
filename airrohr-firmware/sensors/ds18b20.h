/*
 * sensors/ds18b20.h — hibbes-Patch (Issue #7 Phase 1 Refactor)
 *
 * Liest den DS18B20-Sensor (OneWire, T-only) und emittiert den JSON-Schlüssel
 * DS18B20_temperature.
 */

#ifndef SENSORS_DS18B20_H
#define SENSORS_DS18B20_H

#include <WString.h>

void fetchSensorDS18B20(String &s);


/* Render /values-Tabelle. Issue #18 Phase E. */
void render_ds18b20_values(String &page_content);

#endif
