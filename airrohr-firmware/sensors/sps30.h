/*
 * sensors/sps30.h — hibbes-Patch (Issue #7 Phase 1 Refactor)
 *
 * Liest den Sensirion SPS30-PM-Sensor (I²C) ab. Die kontinuierliche
 * Sample-Akkumulation läuft im Haupt-loop() in airrohr-firmware.ino;
 * fetchSensorSPS30() bildet nur den arithmetischen Mittelwert über das
 * Sende-Intervall und emittiert die JSON-Schlüssel SPS30_P0..P4 plus
 * die Number-Concentrations N05..N10 und Typical Particle Size TS.
 *
 * Globals (sps30-Instance, value_*, last_value_*, *_count) bleiben in
 * airrohr-firmware.ino definiert; Zugriff hier per `extern`.
 */

#ifndef SENSORS_SPS30_H
#define SENSORS_SPS30_H

#include <WString.h>

void fetchSensorSPS30(String &s);

#endif
