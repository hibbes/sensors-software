/*
 * sensors/hpm.h — hibbes-Patch (Issue #7 Refactor Phase 1)
 *
 * Extrahiert die fetchSensorHPM-Funktion (Honeywell HPM PM,
 * UART-basiert, teilt sich serialSDS mit SDS/PMS) aus
 * airrohr-firmware.ino in eigenes Übersetzungs-Modul. Globals
 * (last_value_HPM_P1/P2, hpm_*-State, is_HPM_running) bleiben FÜR JETZT
 * in airrohr-firmware.ino definiert; wir greifen von hier per `extern`
 * zu.
 *
 * JSON-Kontrakt: emittiert "HPM_P1" (PM2.5-Mapping) und "HPM_P2" (PM10-Mapping).
 */

#ifndef SENSORS_HPM_H
#define SENSORS_HPM_H

#include <WString.h>

void fetchSensorHPM(String &s);


/* Render /values-Tabelle. Issue #18 Phase E. */
void render_hpm_values(String &page_content);

#endif
