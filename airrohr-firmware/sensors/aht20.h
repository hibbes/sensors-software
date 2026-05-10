/*
 * sensors/aht20.h — hibbes-Patch (Issue #7 Refactor Phase 1)
 *
 * Pilot der modular-Sensor-Refactor-Initiative: extrahiert die
 * fetchSensorAHT20-Funktion aus airrohr-firmware.ino in eigenes
 * Übersetzungs-Modul. Nach erfolgreichem Pilot folgen die anderen
 * 15 Sensoren.
 *
 * Globals (last_value_AHT20_T/H, aht20-Instance, aht20_init_failed)
 * bleiben FÜR JETZT in airrohr-firmware.ino definiert; wir greifen
 * von hier per `extern` zu.
 */

#ifndef SENSORS_AHT20_H
#define SENSORS_AHT20_H

#include <WString.h>

void fetchSensorAHT20(String &s);

#endif
