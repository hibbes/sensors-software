/*
 * sensors/scd30.h — hibbes-Patch (Issue #7 Phase 1 Refactor)
 *
 * Liefert die fetchSensorSCD30-Funktion für den Sensirion SCD30
 * (CO2 + T + H, I²C 0x61). Globals (last_value_SCD30_T/H/CO2,
 * scd30-Instance) bleiben in airrohr-firmware.ino definiert.
 */

#ifndef SENSORS_SCD30_H
#define SENSORS_SCD30_H

#include <WString.h>

void fetchSensorSCD30(String &s);

#endif
