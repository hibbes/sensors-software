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
#include "../ext_def.h"
#include "../utils.h"
#include "../html-content.h"
#include "../intl.h"
#include "../bmx280_i2c.h"
#include <SparkFun_SCD30_Arduino_Library.h>

#if defined(ESP8266)
extern ESP8266WebServer server;
#elif defined(ESP32)
extern WebServer server;
#endif

extern bool wificonfig_loop;
extern bool airrohr_selftest_failed;
extern String esp_chipid;
extern String esp_mac_id;
extern String SOFTWARE_VERSION;
extern unsigned long last_page_load;
extern unsigned long count_sends;
extern unsigned long last_update_attempt;
extern unsigned long time_point_device_start_ms;
extern unsigned long sending_time;
extern unsigned long starttime;
extern unsigned long act_milli;
extern const uint8_t default_ip_first_octet;
extern const uint8_t default_ip_second_octet;
extern const uint8_t default_ip_third_octet;
extern const uint8_t default_ip_fourth_octet;
#ifndef msSince
#define msSince(timestamp_before) (act_milli - (timestamp_before))
#endif
extern int last_update_returncode;
extern int last_sendData_returncode;
extern int last_signal_strength;
extern int last_disconnect_reason;

extern unsigned long SDS_error_count;
extern unsigned long NPM_error_count;
extern unsigned long IPS_error_count;
extern unsigned long SPS30_read_error_counter;
extern unsigned long WiFi_error_count;
extern uint8_t sntp_time_set;

// Sensor-Lesewerte (vom Main-Loop befüllt, von /values + /status gelesen).
extern float last_value_PPD_P1, last_value_PPD_P2;
extern float last_value_SDS_P1, last_value_SDS_P2;
extern float last_value_PMS_P0, last_value_PMS_P1, last_value_PMS_P2;
extern float last_value_HPM_P1, last_value_HPM_P2;
extern float last_value_NPM_P0, last_value_NPM_P1, last_value_NPM_P2;
extern float last_value_NPM_N1, last_value_NPM_N10, last_value_NPM_N25;
extern float last_value_SPS30_P0, last_value_SPS30_P1, last_value_SPS30_P2, last_value_SPS30_P4;
extern float last_value_SPS30_N05, last_value_SPS30_N1, last_value_SPS30_N25, last_value_SPS30_N4, last_value_SPS30_N10;
extern float last_value_SPS30_TS;
extern float last_value_IPS_P0, last_value_IPS_P1, last_value_IPS_P2;
extern float last_value_IPS_P01, last_value_IPS_P03, last_value_IPS_P05, last_value_IPS_P5;
extern float last_value_IPS_N1, last_value_IPS_N10, last_value_IPS_N25;
extern float last_value_IPS_N01, last_value_IPS_N03, last_value_IPS_N05, last_value_IPS_N5;
extern float last_value_DHT_T, last_value_DHT_H;
extern float last_value_HTU21D_T, last_value_HTU21D_H;
extern float last_value_AHT20_T, last_value_AHT20_H;
extern float last_value_SHT3X_T, last_value_SHT3X_H;
extern float last_value_BMP_T, last_value_BMP_P;
extern float last_value_BMX280_T, last_value_BMX280_P;
extern float last_value_BME280_H;
extern float last_value_SCD30_T, last_value_SCD30_H;
extern uint16_t last_value_SCD30_CO2;
extern float last_value_DS18B20_T;
extern float last_value_dnms_laeq, last_value_dnms_la_min, last_value_dnms_la_max;
extern double last_value_GPS_lat, last_value_GPS_lon;
extern float last_value_GPS_alt;
extern String last_value_GPS_timestamp;
extern String last_value_SDS_version;
extern String last_value_NPM_version;

// Sensor-Klassen-Instanzen (in airrohr-firmware.ino angelegt).
extern BMX280 bmx280;
extern SCD30 scd30;

// cfg-Namespace — alle Felder, die Web-Pages lesen oder schreiben.
namespace cfg
{
	extern unsigned debug;
	extern unsigned time_for_wifi_config;
	extern unsigned sending_intervall_ms;
	extern bool powersave;

	extern char current_lang[3];

	extern bool www_basicauth_enabled;
	extern char www_username[LEN_WWW_USERNAME];
	extern char www_password[LEN_CFG_PASSWORD];

	extern char wlanssid[LEN_WLANSSID];
	extern char wlanpwd[LEN_CFG_PASSWORD];

	extern char static_ip[16];
	extern char static_subnet[16];
	extern char static_gateway[16];
	extern char static_dns[16];

	extern char fs_ssid[LEN_FS_SSID];
	extern char fs_pwd[LEN_CFG_PASSWORD];

	extern bool dht_read, htu21d_read, aht20_read;
	extern bool ppd_read, sds_read, pms_read, hpm_read, npm_read, npm_fulltime;
	extern bool ips_read, sps30_read;
	extern bool bmp_read, bmx280_read;
	extern char height_above_sealevel[8];
	extern bool sht3x_read, scd30_read, ds18b20_read;
	extern bool dnms_read;
	extern char dnms_correction[LEN_DNMS_CORRECTION];
	extern bool gps_read;
	extern char temp_correction[LEN_TEMP_CORRECTION];

	extern bool send2dusti, send2madavi, send2sensemap, send2fsapp;
	extern bool send2aircms, send2custom, send2influx, send2csv;
	extern bool send2wunderground;

	extern bool auto_update, use_beta;
	extern char ota_host[LEN_OTA_HOST];

	extern bool has_display, has_sh1106, has_flipped_display;
	extern bool has_lcd1602, has_lcd1602_27, has_lcd2004, has_lcd2004_27;
	extern bool display_wifi_info, display_device_info;

	extern bool ssl_madavi, ssl_dusti;
	extern char senseboxid[LEN_SENSEBOXID];
	extern bool ssl_custom;
	extern char host_custom[LEN_HOST_CUSTOM];
	extern char url_custom[LEN_URL_CUSTOM];
	extern unsigned port_custom;
	extern char user_custom[LEN_USER_CUSTOM];
	extern char pwd_custom[LEN_CFG_PASSWORD];
	extern bool ssl_influx;
	extern char host_influx[LEN_HOST_INFLUX];
	extern char url_influx[LEN_URL_INFLUX];
	extern unsigned port_influx;
	extern char user_influx[LEN_USER_INFLUX];
	extern char pwd_influx[LEN_PASS_INFLUX];
	extern char measurement_name_influx[LEN_MEASUREMENT_NAME_INFLUX];
	extern char wu_station_id[LEN_WU_STATION_ID];
	extern char wu_password[LEN_CFG_PASSWORD];
}

// configShape[]-Tabelle aus airrohr-cfg.h nimmt Adressen von cfg::*-Feldern,
// muss daher NACH den extern-Decls oben inkludiert werden.
#include "../airrohr-cfg.h"

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

// Logger-Konfiguration (Push-Targets). Definition in airrohr-firmware.ino.
extern LoggerConfig loggerConfigs[LoggerCount];

// Page-spezifische Helper, definiert in airrohr-firmware.ino.
bool webserver_request_auth();
void sendHttpRedirect();
void start_html_page(String &page_content, const String &title);
void end_html_page(String &page_content);
void sensor_restart();
bool writeConfig();

// HTML-/Tabellen-Helper, die mehrere Pages teilen.
// add_table_row_from_value, calcWiFiSignalQuality, check_display_value,
// delayToString, tmpl, loggerDescription, readCorrectionOffset werden bereits
// in utils.h deklariert (utils.cpp-Definitionen).
void add_warning_first_cycle(String &page_content);
void add_age_last_values(String &s);
void display_debug(const String &text1, const String &text2);
float dew_point(const float temperature, const float humidity);
float pressure_at_sealevel(const float temperature, const float pressure);

// Form-Builder (für /config), definiert in airrohr-firmware.ino.
String form_checkbox(const ConfigShapeId cfgid, const String &info, const bool linebreak);
String form_submit(const String &value);
String form_select_lang();
void add_form_input(String &page_content, const ConfigShapeId cfgid, const __FlashStringHelper *info, const int length);
