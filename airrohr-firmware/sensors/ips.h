/*
 * sensors/ips.h — hibbes-Patch (Issue #7 Refactor Phase 1)
 *
 * Extrahiert die fetchSensorIPS-Funktion (Piera Systems IPS-7100,
 * UART-basiert, eigener serialIPS-Port; ASCII-CSV-Antwortformat) aus
 * airrohr-firmware.ino in eigenes Übersetzungs-Modul. Globals
 * (last_value_IPS_*-State, ips_*-Akkumulatoren, is_IPS_running,
 * IPS_error_count) bleiben FÜR JETZT in airrohr-firmware.ino definiert;
 * wir greifen von hier per `extern` zu.
 *
 * JSON-Kontrakt: emittiert "IPS_P0/P1/P2/P01/P03/P05/P5" für
 * Massenkonzentrationen und "IPS_N1/N10/N25/N01/N03/N05/N5" für
 * Anzahlkonzentrationen.
 */

#ifndef SENSORS_IPS_H
#define SENSORS_IPS_H

#include <WString.h>

void fetchSensorIPS(String &s);

#endif
