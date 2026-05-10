/*
 * sensors/bmx280.h — hibbes-Patch (Issue #7 Phase 1 Refactor)
 *
 * Liefert die fetchSensorBMX280-Funktion für den Bosch BMP280/BME280
 * (I²C 0x76/0x77). Hardware-Detection zur Laufzeit via bmx280.sensorID().
 * Globals (last_value_BMX280_T/P, last_value_BME280_H, bmx280-Instance)
 * bleiben in airrohr-firmware.ino definiert.
 */

#ifndef SENSORS_BMX280_H
#define SENSORS_BMX280_H

#include <WString.h>

void fetchSensorBMX280(String &s);

#endif
