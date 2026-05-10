/*
 * sensors/bmp180.h — hibbes-Patch (Issue #7 Phase 1 Refactor)
 *
 * Liest den BMP180-Sensor (I²C 0x77, T+P, OLD-chip) und emittiert die
 * JSON-Schlüssel BMP_temperature und BMP_pressure (BMP_-Prefix, nicht BMP180_).
 */

#ifndef SENSORS_BMP180_H
#define SENSORS_BMP180_H

#include <WString.h>

void fetchSensorBMP(String &s);

#endif
