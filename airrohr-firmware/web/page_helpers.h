/**
 * Shared declarations for web page modules (Issue #18 Phase B).
 *
 * Page-Module (web/pages/*.cpp) ziehen aus diesem Header die Forward-Decls
 * für Helpers + extern-Decls für Globals, die alle Pages brauchen.
 * Definitionen leben weiterhin in airrohr-firmware.ino. Auch von der .ino
 * inkludiert, damit struct_wifiInfo nur an einer Stelle definiert ist.
 */
#pragma once

#include <Arduino.h>
#if defined(ESP8266)
#include <ESP8266WebServer.h>
#elif defined(ESP32)
#include <WebServer.h>
#endif

#include "../defines.h"
#include "../utils.h"
#include "../html-content.h"
#include "../intl.h"

#if defined(ESP8266)
extern ESP8266WebServer server;
#elif defined(ESP32)
extern WebServer server;
#endif

extern bool wificonfig_loop;
extern String esp_chipid;
extern String esp_mac_id;
extern unsigned long last_page_load;

namespace cfg
{
	extern unsigned debug;
}

struct struct_wifiInfo
{
	char ssid[LEN_WLANSSID];
	uint8_t encryptionType;
	int32_t RSSI;
	int32_t channel;
#if defined(ESP8266)
	bool isHidden;
	uint8_t unused[3];
#endif
};

extern struct struct_wifiInfo *wifiInfo;
extern uint8_t count_wifiInfo;

bool webserver_request_auth();
void sendHttpRedirect();
void start_html_page(String &page_content, const String &title);
void end_html_page(String &page_content);
void sensor_restart();
