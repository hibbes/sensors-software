/*
 * sensors/npm.h — hibbes-Patch (Issue #7 Refactor Phase 1)
 *
 * Extrahiert die fetchSensorNPM-Funktion (Tera Sensor Next PM,
 * UART-basiert, eigener serialNPM-Port) aus airrohr-firmware.ino in
 * eigenes Übersetzungs-Modul. Globals (last_value_NPM_*-State,
 * is_NPM_running, NPM_waiting_for_16, current_state_npm/current_th_npm)
 * bleiben FÜR JETZT in airrohr-firmware.ino definiert; wir greifen von
 * hier per `extern` zu.
 *
 * JSON-Kontrakt: emittiert "NPM_P0" (PM1), "NPM_P1" (PM10), "NPM_P2"
 * (PM2.5) und Anzahl-Konzentrationen "NPM_N1", "NPM_N10", "NPM_N25".
 */

#ifndef SENSORS_NPM_H
#define SENSORS_NPM_H

#include <WString.h>

void fetchSensorNPM(String &s);

#endif
