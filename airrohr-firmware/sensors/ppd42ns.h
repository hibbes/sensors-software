/*
 * sensors/ppd42ns.h — hibbes-Patch (Issue #7 Phase 1 Refactor)
 *
 * Liest den PPD42NS Shinyei-PM-Sensor (digitales Pulse-Counting via
 * GPIO-LOW-Pulse-Occupancy-Verfahren) und emittiert die JSON-Schlüssel
 * P1/P2 plus Diagnose-Werte durP1/durP2/ratioP1/ratioP2.
 *
 * Globals (last_value_PPD_P1/P2, lowpulseoccupancyP1/P2, trigP1/P2,
 * trigOnP1/P2, send_now, starttime, act_micro) bleiben in
 * airrohr-firmware.ino definiert; Zugriff hier per `extern`.
 */

#ifndef SENSORS_PPD42NS_H
#define SENSORS_PPD42NS_H

#include <WString.h>

void fetchSensorPPD(String &s);


/* Render /values-Tabelle. Issue #18 Phase E. */
void render_ppd_values(String &page_content);

#endif
