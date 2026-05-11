/**
 * Shared declarations for web page modules (Issue #18 Phase B).
 *
 * Page-Module (web/pages/*.cpp) ziehen aus diesem Header die Forward-Decls für
 * die Helper-Funktionen + extern-Decls für Globals, die alle Pages brauchen.
 * Die Definitionen leben weiterhin in airrohr-firmware.ino — der Header
 * deklariert nur, was über Translation-Unit-Grenzen sichtbar sein muss.
 */
#pragma once

#include <Arduino.h>
#if defined(ESP8266)
#include <ESP8266WebServer.h>
#elif defined(ESP32)
#include <WebServer.h>
#endif

#if defined(ESP8266)
extern ESP8266WebServer server;
#elif defined(ESP32)
extern WebServer server;
#endif

extern bool wificonfig_loop;
extern String esp_chipid;
extern String esp_mac_id;
extern unsigned long last_page_load;

extern bool webserver_request_auth();
extern void sendHttpRedirect();
extern void start_html_page(String &page_content, const String &title);
extern void end_html_page(String &page_content);
extern void sensor_restart();

extern const char WEB_PAGE_HEADER[] PROGMEM;
extern const char WEB_PAGE_HEADER_HEAD[] PROGMEM;
extern const char WEB_PAGE_HEADER_BODY[] PROGMEM;
extern const char WEB_PAGE_FOOTER[] PROGMEM;
extern const char WEB_RESET_CONTENT[] PROGMEM;
extern const char WEB_REMOVE_CONFIG_CONTENT[] PROGMEM;
extern const char WEB_ROOT_PAGE_CONTENT[] PROGMEM;
extern const char TXT_CONTENT_TYPE_TEXT_HTML[] PROGMEM;
