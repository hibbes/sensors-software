/*
 * sensors/htu21d.h — hibbes-Patch (Issue #7 Phase 1 Refactor)
 *
 * Liest den HTU21D-Combo-Sensor (I²C 0x40, T+H) und emittiert die
 * JSON-Schlüssel HTU21D_temperature und HTU21D_humidity.
 */

#ifndef SENSORS_HTU21D_H
#define SENSORS_HTU21D_H

#include <WString.h>

void fetchSensorHTU21D(String &s);


/* Render /values-Tabelle. Issue #18 Phase E. */
void render_htu21d_values(String &page_content);

#endif
