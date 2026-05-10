/*
 * sensors/pms.h — hibbes-Patch (Issue #7 Refactor Phase 1)
 *
 * Extrahiert die fetchSensorPMS-Funktion (Plantower PMSx003 PM,
 * UART-basiert, teilt sich serialSDS mit SDS/HPM) aus
 * airrohr-firmware.ino in eigenes Übersetzungs-Modul. Globals
 * (last_value_PMS_P0/P1/P2, pms_*-State, is_PMS_running) bleiben FÜR
 * JETZT in airrohr-firmware.ino definiert; wir greifen von hier per
 * `extern` zu.
 *
 * JSON-Kontrakt: emittiert "PMS_P0" (PM1), "PMS_P1" (PM10), "PMS_P2" (PM2.5).
 */

#ifndef SENSORS_PMS_H
#define SENSORS_PMS_H

#include <WString.h>

void fetchSensorPMS(String &s);

#endif
