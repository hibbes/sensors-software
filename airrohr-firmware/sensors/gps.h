/*
 * sensors/gps.h — hibbes-Patch (Issue #7 Phase 1 Refactor)
 *
 * Formatiert die zuletzt gelesene Position aus einem NEO-6M-GPS via
 * TinyGPSPlus-Lib zum JSON-Push und emittiert die Schlüssel
 * GPS_lat, GPS_lon, GPS_height und GPS_timestamp.
 *
 * ACHTUNG: Das kontinuierliche UART-Byte-Lesen (TinyGPSPlus.encode())
 * findet im Haupt-loop() in airrohr-firmware.ino statt — diese
 * Funktion liest nur den State der gps-Instanz und schreibt ins
 * JSON-Reservoir. Das `gps`-Objekt sowie last_value_GPS_*,
 * count_sends, gps_init_failed sind Globals in airrohr-firmware.ino.
 */

#ifndef SENSORS_GPS_H
#define SENSORS_GPS_H

#include <WString.h>

void fetchSensorGPS(String &s);


/* Render /values-Tabelle. Issue #18 Phase E. */
void render_gps_values(String &page_content);

#endif
