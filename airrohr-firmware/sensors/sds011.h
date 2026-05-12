/*
 * sensors/sds011.h — hibbes-Patch (Issue #7 Refactor Phase 1)
 *
 * Extrahiert die fetchSensorSDS-Funktion (Sensirion/Nova SDS011 PM,
 * UART-basiert) aus airrohr-firmware.ino in eigenes Übersetzungs-
 * Modul. Globals (last_value_SDS_P1/P2, sds_*-State, is_SDS_running,
 * SDS_waiting_for, SDS_error_count) bleiben FÜR JETZT in
 * airrohr-firmware.ino definiert; wir greifen von hier per `extern` zu.
 *
 * JSON-Kontrakt: emittiert Schlüssel "SDS_P1" (PM10) und "SDS_P2" (PM2.5).
 */

#ifndef SENSORS_SDS011_H
#define SENSORS_SDS011_H

#include <WString.h>

void fetchSensorSDS(String &s);


/* Render /values-Tabelle. Issue #18 Phase E. */
void render_sds011_values(String &page_content);

#endif
