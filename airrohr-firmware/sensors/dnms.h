/*
 * sensors/dnms.h — hibbes-Patch (Issue #7 Phase 1 Refactor)
 *
 * Liest den DNMS-Lärmmessknoten (digital noise measurement sensor,
 * I²C, eigene Firmware) ab und emittiert die JSON-Schlüssel
 * DNMS_noise_LAeq, DNMS_noise_LA_min, DNMS_noise_LA_max.
 *
 * Der DNMS-Treiber (dnms_calculate_leq/dnms_read_data_ready/
 * dnms_read_leq/dnms_reset und struct dnms_measurements) lebt in
 * dnms_i2c.{h,cpp}. Globals (last_value_dnms_*, cfg::dnms_correction)
 * bleiben in airrohr-firmware.ino definiert; Zugriff hier per `extern`.
 */

#ifndef SENSORS_DNMS_H
#define SENSORS_DNMS_H

#include <WString.h>

void fetchSensorDNMS(String &s);


/* Render /values-Tabelle. Issue #18 Phase E. */
void render_dnms_values(String &page_content);

#endif
