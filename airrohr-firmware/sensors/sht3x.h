/*
 * sensors/sht3x.h — hibbes-Patch (Issue #7 Phase 1 Refactor)
 *
 * Liest den SHT3x-Combo-Sensor (I²C 0x44, T+H) und emittiert die
 * JSON-Schlüssel SHT3X_temperature und SHT3X_humidity.
 */

#ifndef SENSORS_SHT3X_H
#define SENSORS_SHT3X_H

#include <WString.h>

void fetchSensorSHT3x(String &s);


/* Render /values-Tabelle. Issue #18 Phase E. */
void render_sht3x_values(String &page_content);

#endif
