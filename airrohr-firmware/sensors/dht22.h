/*
 * sensors/dht22.h — hibbes-Patch (Issue #7 Phase 1 Refactor)
 *
 * Liefert die fetchSensorDHT-Funktion für den DHT11/DHT22-Combo-Sensor
 * (1-Wire / GPIO digital). Globals (last_value_DHT_T/H, dht-Instance)
 * bleiben in airrohr-firmware.ino definiert; hier nur Funktions-Decl.
 */

#ifndef SENSORS_DHT22_H
#define SENSORS_DHT22_H

#include <WString.h>

void fetchSensorDHT(String &s);


/* Render /values-Tabelle. Issue #18 Phase E. */
void render_dht_values(String &page_content);

#endif
