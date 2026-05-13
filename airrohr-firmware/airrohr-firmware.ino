/************************************************************************
 *                                                                      *
 *  This source code needs to be compiled for the board                 *
 *  NodeMCU 1.0 (ESP-12E Module)                                        *
 *                                                                      *
 ************************************************************************
 *                                                                      *
 *    airRohr firmware                                                  *
 *    Copyright (C) 2016-2021  Code for Stuttgart a.o.                  *
 *    Copyright (C) 2021-2024  Sensor.Community a.o.                    *
 *    Copyright (C) 2019-2020  Dirk Mueller                             *
 *                                                                      *
 * This program is free software: you can redistribute it and/or modify *
 * it under the terms of the GNU General Public License as published by *
 * the Free Software Foundation, either version 3 of the License, or    *
 * (at your option) any later version.                                  *
 *                                                                      *
 * This program is distributed in the hope that it will be useful,      *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of       *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        *
 * GNU General Public License for more details.                         *
 *                                                                      *
 * You should have received a copy of the GNU General Public License    *
 * along with this program. If not, see <http://www.gnu.org/licenses/>. *
 *                                                                      *
 ************************************************************************
 * OK LAB Particulate Matter Sensor                                     *
 *      - nodemcu-LoLin board                                           *
 *      - Nova SDS0111                                                  *
 *  http://inovafitness.com/en/Laser-PM2-5-Sensor-SDS011-35.html        *
 *                                                                      *
 * Wiring Instruction see included Readme.md                            *
 *                                                                      *
 ************************************************************************
 *                                                                      *
 * Alternative                                                          *
 *      - nodemcu-LoLin board                                           *
 *                                                                      *
 * Wiring Instruction:                                                  *
 *      Pin 2 of dust sensor PM2.5 -> Digital 6 (PWM)                   *
 *      Pin 3 of dust sensor       -> +5V                               *
 *      Pin 4 of dust sensor PM1   -> Digital 3 (PMW)                   *
 *                                                                      *
 *                                                                      *
 ************************************************************************
 *                                                                      *
 * Please check Readme.md for other sensors and hardware                *
 *                                                                      *
 ************************************************************************
 *
 * latest build
 * RAM:   [====      ]  41.8% (used 34220 bytes from 81920 bytes)
 * Flash: [=======   ]  67.1% (used 701191 bytes from 1044464 bytes)
 *
 ************************************************************************/
 
#include <WString.h>
#include <pgmspace.h>

// SOFTWARE_VERSION_STR moved to defines.h so html-content.cpp can see it (Issue #18)
// String SOFTWARE_VERSION moved below includes

/*****************************************************************
 * Includes                                                      *
 *****************************************************************/

#if defined(ESP8266)
#include <FS.h> // must be first
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <SoftwareSerial.h>
#include <Hash.h>
#include <ctime>
#include <coredecls.h>
#include <sntp.h>
#endif

#if defined(ESP32)
#define FORMAT_SPIFFS_IF_FAILED true
#include <FS.h>
#include <HTTPClient.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <HardwareSerial.h>
#include <hwcrypto/sha.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#endif

// includes common to ESP8266 and ESP32 (especially external libraries)
#include "./oledfont.h" // avoids including the default Arial font, needs to be included before SSD1306.h
#include <SSD1306.h>
#include <SH1106.h>
#include <LiquidCrystal_I2C.h>
#define ARDUINOJSON_ENABLE_ARDUINO_STREAM 0
#define ARDUINOJSON_ENABLE_ARDUINO_PRINT 0
#define ARDUINOJSON_DECODE_UNICODE 0
#include <ArduinoJson.h>
#include <DNSServer.h>
#include "./DHT.h"
#include <Adafruit_HTU21DF.h>
#include <Adafruit_AHTX0.h>
#if defined(ESP8266)
#include <ESP8266HTTPUpdateServer.h>
#endif
// Sensor-Treiber-Module (Issue #7 Phase 1 Refactor, Makerspace Schiller-Gymnasium Offenburg)
#include "sensors/aht20.h"
#include "sensors/dht22.h"
#include "sensors/htu21d.h"
#include "sensors/bmp180.h"
#include "sensors/sht3x.h"
#include "sensors/scd30.h"
#include "sensors/bmx280.h"
#include "sensors/ds18b20.h"
#include "sensors/sds011.h"
#include "sensors/pms.h"
#include "sensors/hpm.h"
#include "sensors/npm.h"
#include "sensors/ips.h"
#include "sensors/ppd42ns.h"
#include "sensors/sps30.h"
#include "sensors/dnms.h"
#include "sensors/gps.h"
#include <Adafruit_BMP085.h>
#include <Adafruit_SHT31.h>
#include <StreamString.h>
#include <DallasTemperature.h>
#include <SparkFun_SCD30_Arduino_Library.h>
#include <TinyGPS++.h>
#include "./bmx280_i2c.h"
#include "./sps30_i2c.h"
#include "./dnms_i2c.h"

#define INTL_DEFINE_VARIABLES
#include "./intl.h"

#include "./utils.h"
#include "defines.h"
#include "ext_def.h"
#include "html-content.h"
#include "./web/page_helpers.h"
#include "./web/pages.h"
#include "./web/assets_generated.h"

String SOFTWARE_VERSION(SOFTWARE_VERSION_STR);

/******************************************************************
 * The variables inside the cfg namespace are persistent          *
 * configuration values. They have defaults which can be          *
 * configured at compile-time via the ext_def.h file              *
 * They can be changed by the user via the web interface, the     *
 * changes are persisted to the flash and read back after reboot. *
 * Note that the names of these variables can't be easily changed *
 * as they are part of the json format used to persist the data.  *
 ******************************************************************/
namespace cfg
{
	unsigned debug = DEBUG;
	
	unsigned time_for_wifi_config = 600000;
	unsigned sending_intervall_ms = 145000;
	bool powersave;

	char current_lang[3];

	// credentials for basic auth of internal web server
	bool www_basicauth_enabled = WWW_BASICAUTH_ENABLED;
	char www_username[LEN_WWW_USERNAME];
	char www_password[LEN_CFG_PASSWORD];

	// wifi credentials
	char wlanssid[LEN_WLANSSID];
	char wlanpwd[LEN_CFG_PASSWORD];

	char static_ip[16];
	char static_subnet[16];
	char static_gateway[16];
	char static_dns[16];

	// credentials of the sensor in access point mode
	char fs_ssid[LEN_FS_SSID] = FS_SSID;
	char fs_pwd[LEN_CFG_PASSWORD] = FS_PWD;

	// (in)active sensors
	bool dht_read = DHT_READ;
	bool htu21d_read = HTU21D_READ;
	bool aht20_read = AHT20_READ;
	bool ppd_read = PPD_READ;
	bool sds_read = SDS_READ;
	bool pms_read = PMS_READ;
	bool hpm_read = HPM_READ;
	bool npm_read = NPM_READ;
	bool npm_fulltime = NPM_FULLTIME;
	bool ips_read = IPS_READ;
	bool sps30_read = SPS30_READ;
	bool bmp_read = BMP_READ;
	bool bmx280_read = BMX280_READ;
	char height_above_sealevel[8] = "0";
	bool sht3x_read = SHT3X_READ;
	bool scd30_read = SCD30_READ;
	bool ds18b20_read = DS18B20_READ;
	bool dnms_read = DNMS_READ;
	char dnms_correction[LEN_DNMS_CORRECTION] = DNMS_CORRECTION;
	bool gps_read = GPS_READ;
	char temp_correction[LEN_TEMP_CORRECTION] = TEMP_CORRECTION;

	// send to "APIs"
	bool send2dusti = SEND2SENSORCOMMUNITY;
	bool send2madavi = SEND2MADAVI;
	bool send2sensemap = SEND2SENSEMAP;
	bool send2fsapp = SEND2FSAPP;
	bool send2aircms = SEND2AIRCMS;
	bool send2custom = SEND2CUSTOM;
	bool send2influx = SEND2INFLUX;
	bool send2csv = SEND2CSV;

	bool auto_update = AUTO_UPDATE;
	bool use_beta = USE_BETA;
	char ota_host[LEN_OTA_HOST] = "";

	// (in)active displays
	bool has_display = HAS_DISPLAY; // OLED with SSD1306 and I2C
	bool has_sh1106 = HAS_SH1106;
	bool has_flipped_display = HAS_FLIPPED_DISPLAY;
	bool has_lcd1602 = HAS_LCD1602;
	bool has_lcd1602_27 = HAS_LCD1602_27;
	bool has_lcd2004 = HAS_LCD2004;
	bool has_lcd2004_27 = HAS_LCD2004_27;

	bool display_wifi_info = DISPLAY_WIFI_INFO;
	bool display_device_info = DISPLAY_DEVICE_INFO;

	// API settings
	bool ssl_madavi = SSL_MADAVI;
	bool ssl_dusti = SSL_SENSORCOMMUNITY;
	char senseboxid[LEN_SENSEBOXID] = SENSEBOXID;

	char host_influx[LEN_HOST_INFLUX];
	char url_influx[LEN_URL_INFLUX];
	unsigned port_influx = PORT_INFLUX;
	char user_influx[LEN_USER_INFLUX] = USER_INFLUX;
	char pwd_influx[LEN_PASS_INFLUX] = PWD_INFLUX;
	char measurement_name_influx[LEN_MEASUREMENT_NAME_INFLUX];
	bool ssl_influx = SSL_INFLUX;

	char host_custom[LEN_HOST_CUSTOM];
	char url_custom[LEN_URL_CUSTOM];
	bool ssl_custom = SSL_CUSTOM;
	unsigned port_custom = PORT_CUSTOM;
	char user_custom[LEN_USER_CUSTOM] = USER_CUSTOM;
	char pwd_custom[LEN_CFG_PASSWORD] = PWD_CUSTOM;

	// hibbes-Patch (Issue #16): direkter Push an Wunderground PWS-API
	bool send2wunderground = false;
	char wu_station_id[LEN_WU_STATION_ID] = "";
	char wu_password[LEN_CFG_PASSWORD] = "";

	void initNonTrivials(const char *id)
	{
		strcpy(cfg::current_lang, CURRENT_LANG);
		strcpy_P(www_username, WWW_USERNAME);
		strcpy_P(www_password, WWW_PASSWORD);
		strcpy_P(wlanssid, WLANSSID);
		strcpy_P(wlanpwd, WLANPWD);
		strcpy_P(host_custom, HOST_CUSTOM);
		strcpy_P(url_custom, URL_CUSTOM);
		strcpy_P(ota_host, OTA_HOST);
		strcpy_P(host_influx, HOST_INFLUX);
		strcpy_P(url_influx, URL_INFLUX);
		strcpy_P(measurement_name_influx, MEASUREMENT_NAME_INFLUX);

		if (!*fs_ssid)
		{
			strcpy(fs_ssid, SSID_BASENAME);
			strcat(fs_ssid, id);
		}
	}
}

// JSON_BUFFER_SIZE moved to defines.h so web/pages/config.cpp can see it (Issue #18)

LoggerConfig loggerConfigs[LoggerCount];

long int sample_count = 0;
bool htu21d_init_failed = false;
bool aht20_init_failed = false;
bool bmp_init_failed = false;
bool bmx280_init_failed = false;
bool sht3x_init_failed = false;
bool scd30_init_failed = false;
bool dnms_init_failed = false;
bool gps_init_failed = false;
bool airrohr_selftest_failed = false;

#if defined(ESP8266)
ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;
#endif
#if defined(ESP32)
WebServer server(80);
#endif

// default IPv4 address (ex. 192.168.4.1)
const uint8_t default_ip_first_octet = 192;
const uint8_t default_ip_second_octet = 168;
const uint8_t default_ip_third_octet = 4;
const uint8_t default_ip_fourth_octet = 1;

#include "./airrohr-cfg.h"

/*****************************************************************
 * Variables for Noise Measurement DNMS                          *
 *****************************************************************/
float last_value_dnms_laeq = -1.0;
float last_value_dnms_la_min = -1.0;
float last_value_dnms_la_max = -1.0;

/*****************************************************************
 * Display definitions                                           *
 *****************************************************************/
SSD1306 *oled_ssd1306 = nullptr;
SH1106 *oled_sh1106 = nullptr;
LiquidCrystal_I2C *lcd_1602 = nullptr;
LiquidCrystal_I2C *lcd_2004 = nullptr;
const uint8_t lcd_1602_default_i2c_address = 0x3f;
const uint8_t lcd_1602_alternate_i2c_address = 0x27;
const uint8_t lcd_1602_columns = 16;
const uint8_t lcd_1602_rows = 2;
const uint8_t lcd_2004_default_i2c_address = 0x3f;
const uint8_t lcd_2004_alternate_i2c_address =  0x27;
const uint8_t lcd_2004_columns = 20;
const uint8_t lcd_2004_rows = 4;

/*****************************************************************
 * Serial declarations                                           *
 *****************************************************************/
#if defined(ESP8266)
SoftwareSerial serialSDS;
SoftwareSerial *serialGPS;
SoftwareSerial serialNPM;
SoftwareSerial serialIPS;
#endif
#if defined(ESP32)
#define serialSDS (Serial1)
#define serialGPS (&(Serial2))
#define serialNPM (Serial1)
#define serialIPS (Serial1)
#endif

/*****************************************************************
 * DHT declaration                                               *
 *****************************************************************/
DHT dht(ONEWIRE_PIN, DHT_TYPE);

/*****************************************************************
 * HTU21D declaration                                            *
 *****************************************************************/
Adafruit_HTU21DF htu21d;

/*****************************************************************
 * AHT20 declaration                                             *
 *****************************************************************/
Adafruit_AHTX0 aht20;

/*****************************************************************
 * BMP declaration                                               *
 *****************************************************************/
Adafruit_BMP085 bmp;

/*****************************************************************
 * BMP/BME280 declaration                                        *
 *****************************************************************/
BMX280 bmx280;
const uint8_t bmx280_default_i2c_address = 0x77;
const uint8_t bmx280_alternate_i2c_address = 0x76;

/*****************************************************************
 * SHT3x declaration                                             *
 *****************************************************************/
Adafruit_SHT31 sht3x;

/*****************************************************************
 * DS18B20 declaration                                            *
 *****************************************************************/
OneWire oneWire;
DallasTemperature ds18b20(&oneWire);

/*****************************************************************
 * SCD30 declaration                                             *
 *****************************************************************/
SCD30 scd30;

/*****************************************************************
 * GPS declaration                                               *
 *****************************************************************/
TinyGPSPlus gps;

/*****************************************************************
 * Variable Definitions for PPD24NS                              *
 * P1 for PM10 & P2 for PM25                                     *
 *****************************************************************/

boolean trigP1 = false;
boolean trigP2 = false;
unsigned long trigOnP1;
unsigned long trigOnP2;

unsigned long lowpulseoccupancyP1 = 0;
unsigned long lowpulseoccupancyP2 = 0;

bool send_now = false;
unsigned long starttime;
unsigned long time_point_device_start_ms;
unsigned long starttime_SDS;
unsigned long starttime_GPS;
unsigned long starttime_NPM;
unsigned long starttime_IPS;
unsigned long act_micro;
unsigned long act_milli;
unsigned long last_micro = 0;
unsigned long min_micro = 1000000000;
unsigned long max_micro = 0;

bool is_SDS_running = true;
// hibbes-Patch (Issue #7): de-anonymisiert für cross-TU-extern-Zugriff aus sensors/sds011.cpp + sensors/npm.cpp
enum SDS_State
{
	SDS_REPLY_HDR = 10,
	SDS_REPLY_BODY = 8
};
int SDS_waiting_for;

// To read NPM responses
enum NPM_State_16
{
	NPM_REPLY_HEADER_16 = 16,
	NPM_REPLY_STATE_16 = 14,
	NPM_REPLY_BODY_16 = 13,
	NPM_REPLY_CHECKSUM_16 = 1
};
int NPM_waiting_for_16; //for concentration

enum
{
	NPM_REPLY_HEADER_4 = 4,
	NPM_REPLY_STATE_4 = 2,
	NPM_REPLY_CHECKSUM_4 = 1
} NPM_waiting_for_4; //for change

enum
{
	NPM_REPLY_HEADER_5 = 5,
	NPM_REPLY_STATE_5 = 3,
	NPM_REPLY_DATA_5 = 2,
	NPM_REPLY_CHECKSUM_5 = 1
} NPM_waiting_for_5; //for fan speed

enum
{
	NPM_REPLY_HEADER_6 = 6,
	NPM_REPLY_STATE_6 = 4,
	NPM_REPLY_DATA_6 = 3,
	NPM_REPLY_CHECKSUM_6 = 1
} NPM_waiting_for_6; // for version

enum
{
	NPM_REPLY_HEADER_8 = 8,
	NPM_REPLY_STATE_8 = 6,
	NPM_REPLY_BODY_8 = 5,
	NPM_REPLY_CHECKSUM_8 = 1
} NPM_waiting_for_8; // for temperature/humidity


//ENUM POUR IPS??

String current_state_npm;
String current_th_npm;

bool is_PMS_running = true;
bool is_HPM_running = true;
bool is_NPM_running = false;
bool is_IPS_running;

unsigned long sending_time = 0;
unsigned long last_update_attempt;
int last_update_returncode;
int last_sendData_returncode;

float last_value_BMP_T = -128.0;
float last_value_BMP_P = -1.0;
float last_value_BMX280_T = -128.0;
float last_value_BMX280_P = -1.0;
float last_value_BME280_H = -1.0;
float last_value_DHT_T = -128.0;
float last_value_DHT_H = -1.0;
float last_value_DS18B20_T = -1.0;
float last_value_HTU21D_T = -128.0;
float last_value_HTU21D_H = -1.0;
float last_value_AHT20_T = -128.0;
float last_value_AHT20_H = -1.0;
float last_value_SHT3X_T = -128.0;
float last_value_SHT3X_H = -1.0;
float last_value_SCD30_T = -128.0;
float last_value_SCD30_H = -1.0;
uint16_t last_value_SCD30_CO2 = 0;

uint32_t sds_pm10_sum = 0;
uint32_t sds_pm25_sum = 0;
uint32_t sds_val_count = 0;
uint32_t sds_pm10_max = 0;
uint32_t sds_pm10_min = 20000;
uint32_t sds_pm25_max = 0;
uint32_t sds_pm25_min = 20000;

int pms_pm1_sum = 0;
int pms_pm10_sum = 0;
int pms_pm25_sum = 0;
int pms_val_count = 0;
int pms_pm1_max = 0;
int pms_pm1_min = 20000;
int pms_pm10_max = 0;
int pms_pm10_min = 20000;
int pms_pm25_max = 0;
int pms_pm25_min = 20000;

int hpm_pm10_sum = 0;
int hpm_pm25_sum = 0;
int hpm_val_count = 0;
int hpm_pm10_max = 0;
int hpm_pm10_min = 20000;
int hpm_pm25_max = 0;
int hpm_pm25_min = 20000;

uint32_t npm_pm1_sum = 0;
uint32_t npm_pm10_sum = 0;
uint32_t npm_pm25_sum = 0;
uint32_t npm_pm1_sum_pcs = 0;
uint32_t npm_pm10_sum_pcs = 0;
uint32_t npm_pm25_sum_pcs = 0;
uint16_t npm_val_count = 0;
uint16_t npm_pm1_max = 0;
uint16_t npm_pm1_min = 20000;
uint16_t npm_pm10_max = 0;
uint16_t npm_pm10_min = 20000;
uint16_t npm_pm25_max = 0;
uint16_t npm_pm25_min = 20000;
uint16_t npm_pm1_max_pcs = 0;
uint16_t npm_pm1_min_pcs = 60000;
uint16_t npm_pm10_max_pcs = 0;
uint16_t npm_pm10_min_pcs = 60000;
uint16_t npm_pm25_max_pcs = 0;
uint16_t npm_pm25_min_pcs = 60000;

float ips_pm01_sum = 0;
float ips_pm03_sum = 0;
float ips_pm05_sum = 0;
float ips_pm1_sum = 0;
float ips_pm25_sum = 0;
float ips_pm5_sum = 0;
float ips_pm10_sum = 0;
unsigned long ips_pm01_sum_pcs = 0;
unsigned long ips_pm03_sum_pcs = 0;
unsigned long ips_pm05_sum_pcs = 0;
unsigned long ips_pm1_sum_pcs = 0;
unsigned long ips_pm25_sum_pcs = 0;
unsigned long ips_pm5_sum_pcs = 0;
unsigned long ips_pm10_sum_pcs = 0;
uint16_t ips_val_count = 0;
float ips_pm01_max = 0;
float ips_pm01_min = 200;
float ips_pm03_max = 0;
float ips_pm03_min = 200;
float ips_pm05_max = 0;
float ips_pm05_min = 200;
float ips_pm1_max = 0;
float ips_pm1_min = 200;
float ips_pm25_max = 0;
float ips_pm25_min = 200;
float ips_pm5_max = 0;
float ips_pm5_min = 200;
float ips_pm10_max = 0;
float ips_pm10_min = 200;
unsigned long ips_pm01_max_pcs = 0;
unsigned long  ips_pm01_min_pcs = 4000000000;
unsigned long  ips_pm03_max_pcs = 0;
unsigned long  ips_pm03_min_pcs = 4000000000;
unsigned long  ips_pm05_max_pcs = 0;
unsigned long  ips_pm05_min_pcs = 4000000000;
unsigned long  ips_pm1_max_pcs = 0;
unsigned long  ips_pm1_min_pcs = 4000000000;
unsigned long  ips_pm25_max_pcs = 0;
unsigned long  ips_pm25_min_pcs = 4000000000;
unsigned long  ips_pm5_max_pcs = 0;
unsigned long ips_pm5_min_pcs = 4000000000;
unsigned long  ips_pm10_max_pcs = 0;
unsigned long  ips_pm10_min_pcs = 4000000000;

float last_value_SPS30_P0 = -1.0;
float last_value_SPS30_P1 = -1.0;
float last_value_SPS30_P2 = -1.0;
float last_value_SPS30_P4 = -1.0;
float last_value_SPS30_N05 = -1.0;
float last_value_SPS30_N1 = -1.0;
float last_value_SPS30_N25 = -1.0;
float last_value_SPS30_N4 = -1.0;
float last_value_SPS30_N10 = -1.0;
float last_value_SPS30_TS = -1.0;
float value_SPS30_P0 = 0.0;
float value_SPS30_P1 = 0.0;
float value_SPS30_P2 = 0.0;
float value_SPS30_P4 = 0.0;
float value_SPS30_N05 = 0.0;
float value_SPS30_N1 = 0.0;
float value_SPS30_N25 = 0.0;
float value_SPS30_N4 = 0.0;
float value_SPS30_N10 = 0.0;
float value_SPS30_TS = 0.0;

uint16_t SPS30_measurement_count = 0;
unsigned long SPS30_read_counter = 0;
unsigned long SPS30_read_error_counter = 0;
unsigned long SPS30_read_timer = 0;
bool sps30_init_failed = false;

float last_value_PPD_P1 = -1.0;
float last_value_PPD_P2 = -1.0;
float last_value_SDS_P1 = -1.0;
float last_value_SDS_P2 = -1.0;
float last_value_PMS_P0 = -1.0;
float last_value_PMS_P1 = -1.0;
float last_value_PMS_P2 = -1.0;
float last_value_HPM_P1 = -1.0;
float last_value_HPM_P2 = -1.0;
float last_value_NPM_P0 = -1.0;
float last_value_NPM_P1 = -1.0;
float last_value_NPM_P2 = -1.0;
float last_value_NPM_N1 = -1.0;
float last_value_NPM_N10 = -1.0;
float last_value_NPM_N25 = -1.0;

float last_value_IPS_P0 = -1.0; //PM1
float last_value_IPS_P1 = -1.0;	//PM10
float last_value_IPS_P2 = -1.0;	//PM2.5
float last_value_IPS_P01 = -1.0; //PM0.1
float last_value_IPS_P03 = -1.0; //PM0.3 //ATTENTION P4 = PM4 POUR SPS30
float last_value_IPS_P05 = -1.0; //PM0.5
float last_value_IPS_P5 = -1.0; //PM5
float last_value_IPS_N1 = -1.0;
float last_value_IPS_N10 = -1.0;
float last_value_IPS_N25 = -1.0;
float last_value_IPS_N01 = -1.0;
float last_value_IPS_N03 = -1.0; //ATTENTION P4 = PM4 POUR SPS30
float last_value_IPS_N05 = -1.0;
float last_value_IPS_N5 = -1.0;

float last_value_GPS_alt = -1000.0;
double last_value_GPS_lat = -200.0;
double last_value_GPS_lon = -200.0;
String last_value_GPS_timestamp;
String last_data_string;
int last_signal_strength;
int last_disconnect_reason;

String esp_chipid;
String esp_mac_id;
String last_value_SDS_version;
String last_value_NPM_version;
String last_value_IPS_version;

unsigned long SDS_error_count;
unsigned long NPM_error_count;
unsigned long IPS_error_count;
unsigned long WiFi_error_count;

// hibbes-Patch: PPD-Pin-Macros von ext_def.h als file-scope int exposed
// damit sensors/ppd42ns.cpp sie via extern referenzieren kann (cross-TU)
// Note: top-level `const` hat internal linkage in C++ — wir brauchen external
// linkage, daher plain int (effektiv const, wir mutieren sie nirgendwo).
int ppd_pin_pm1 = PPD_PIN_PM1;
int ppd_pin_pm2 = PPD_PIN_PM2;

// hibbes-Patch (Issue #4): Silent-failure-detection für WiFi-State-Korruption.
// Pro Send-Cycle zählen wie viele Pushes versucht/erfolgreich waren. Wenn N
// Cycles in Folge alle Pushes scheitern trotz WL_CONNECTED, ist der WLAN-Stack
// kaputt (Router-NAT-Stale, ARP-Probleme, BearSSL-Session-Hang) und braucht
// einen harten Disconnect+Reconnect — das Standard-WiFi.status()-Check sieht
// das nicht.
uint8_t cycle_send_attempts = 0;

// Issue #11: pro Send-Cycle ein Master-Budget. Wenn ein Target hängt (TLS-Stall
// gegen kaputten Server, DNS-Timeout, etc.), sollen nachfolgende Pushes nicht
// blockieren bis hin zum Watchdog-Reset. Bei `millis() > cycle_send_deadline`
// werden weitere sendData()/sendWundergroundUpdate() früh-returned.
// 90 s ist großzügig genug für 10-15 Push-Targets bei je 5-8 s realem Push-Time
// + TLS-Handshake, deckelt aber den Worst Case unter sending_intervall_ms=145s.
static constexpr unsigned long CYCLE_SEND_BUDGET_MS = 90 * 1000UL;
unsigned long cycle_send_deadline = 0;
uint8_t cycle_send_successes = 0;
uint8_t consecutive_silent_failures = 0;
constexpr uint8_t SILENT_FAILURE_THRESHOLD = 3;

unsigned long last_page_load = millis();

bool wificonfig_loop = false;
uint8_t sntp_time_set;

unsigned long count_sends = 0;
unsigned long last_display_millis = 0;
uint8_t next_display_count = 0;

// struct struct_wifiInfo moved to web/page_helpers.h (Issue #18)
struct struct_wifiInfo *wifiInfo;
uint8_t count_wifiInfo;

IPAddress addr_static_ip;
IPAddress addr_static_subnet;
IPAddress addr_static_gateway;
IPAddress addr_static_dns;

#define msSince(timestamp_before) (act_milli - (timestamp_before))

const char data_first_part[] PROGMEM = "{\"software_version\": \"" SOFTWARE_VERSION_STR "\", \"sensordatavalues\":[";
const char JSON_SENSOR_DATA_VALUES[] PROGMEM = "sensordatavalues";

/*****************************************************************
 * display values                                                *
 *****************************************************************/
void display_debug(const String &text1, const String &text2)
{
	debug_outln_info(F("output debug text to displays..."));
	if (oled_ssd1306)
	{
		oled_ssd1306->clear();
		oled_ssd1306->displayOn();
		oled_ssd1306->setTextAlignment(TEXT_ALIGN_LEFT);
		oled_ssd1306->drawString(0, 12, text1);
		oled_ssd1306->drawString(0, 24, text2);
		oled_ssd1306->display();
	}
	if (oled_sh1106)
	{
		oled_sh1106->clear();
		oled_sh1106->displayOn();
		oled_sh1106->setTextAlignment(TEXT_ALIGN_LEFT);
		oled_sh1106->drawString(0, 12, text1);
		oled_sh1106->drawString(0, 24, text2);
		oled_sh1106->display();
	}
	if (lcd_1602)
	{
		lcd_1602->clear();
		lcd_1602->setCursor(0, 0);
		lcd_1602->print(text1);
		lcd_1602->setCursor(0, 1);
		lcd_1602->print(text2);
	}
	if (lcd_2004)
	{
		lcd_2004->clear();
		lcd_2004->setCursor(0, 0);
		lcd_2004->print(text1);
		lcd_2004->setCursor(0, 1);
		lcd_2004->print(text2);
	}
}

/*****************************************************************
 * read SDS011 sensor serial and firmware date                   *
 *****************************************************************/
static String SDS_version_date()
{

	if (cfg::sds_read && !last_value_SDS_version.length())
	{
		debug_outln_verbose(FPSTR(DBG_TXT_START_READING), FPSTR(DBG_TXT_SDS011_VERSION_DATE));
		is_SDS_running = SDS_cmd(PmSensorCmd::Start);
		delay(250);
#if defined(ESP8266)
		serialSDS.perform_work();
#endif
		serialSDS.flush();
		// Query Version/Date
		SDS_rawcmd(0x07, 0x00, 0x00);
		delay(400);
		const constexpr uint8_t header_cmd_response[2] = {0xAA, 0xC5};
		while (serialSDS.find(header_cmd_response, sizeof(header_cmd_response)))
		{
			uint8_t data[8];
			unsigned r = serialSDS.readBytes(data, sizeof(data));
			if (r == sizeof(data) && data[0] == 0x07 && SDS_checksum_valid(data))
			{
				char tmp[20];
				snprintf_P(tmp, sizeof(tmp), PSTR("%02d-%02d-%02d(%02x%02x)"),
						   data[1], data[2], data[3], data[4], data[5]);
				last_value_SDS_version = tmp;
				break;
			}
		}
		debug_outln_verbose(FPSTR(DBG_TXT_END_READING), FPSTR(DBG_TXT_SDS011_VERSION_DATE));
	}

	return last_value_SDS_version;
}

/*****************************************************************
 * read Next PM sensor serial and firmware date                   *
 *****************************************************************/

static uint8_t NPM_get_state()
{
	uint8_t result;
	NPM_waiting_for_4 = NPM_REPLY_HEADER_4;
	debug_outln_info(F("State NPM..."));
	NPM_cmd(PmSensorCmd2::State);

	while (!serialNPM.available())
	{
		debug_outln("Wait for Serial...", DEBUG_MAX_INFO);
	}

	while (serialNPM.available() >= NPM_waiting_for_4)
	{
		const uint8_t constexpr header[2] = {0x81, 0x16};
		uint8_t state[1];
		uint8_t checksum[1];
		uint8_t test[4];

		switch (NPM_waiting_for_4)
		{
		case NPM_REPLY_HEADER_4:
			if (serialNPM.find(header, sizeof(header)))
				NPM_waiting_for_4 = NPM_REPLY_STATE_4;
			break;
		case NPM_REPLY_STATE_4:
			serialNPM.readBytes(state, sizeof(state));
			NPM_state(state[0]);
			result = state[0];
			NPM_waiting_for_4 = NPM_REPLY_CHECKSUM_4;
			break;
		case NPM_REPLY_CHECKSUM_4:
			serialNPM.readBytes(checksum, sizeof(checksum));
			memcpy(test, header, sizeof(header));
			memcpy(&test[sizeof(header)], state, sizeof(state));
			memcpy(&test[sizeof(header) + sizeof(state)], checksum, sizeof(checksum));
			NPM_data_reader(test, 4);
			NPM_waiting_for_4 = NPM_REPLY_HEADER_4;
			if (NPM_checksum_valid_4(test))
			{
				debug_outln_info(F("Checksum OK..."));
			}
			break;
		}
	}
	return result;
}

// hibbes-Patch: static dropped — fetchSensorNPM is now in sensors/npm.cpp and needs cross-TU access
bool NPM_start_stop()
{
	bool result;
	NPM_waiting_for_4 = NPM_REPLY_HEADER_4;
	debug_outln_info(F("Switch start/stop NPM..."));
	NPM_cmd(PmSensorCmd2::Change);

	while (!serialNPM.available())
	{
		debug_outln("Wait for Serial...", DEBUG_MAX_INFO);
	}

	while (serialNPM.available() >= NPM_waiting_for_4)
	{
		const uint8_t constexpr header[2] = {0x81, 0x15};
		uint8_t state[1];
		uint8_t checksum[1];
		uint8_t test[4];

		switch (NPM_waiting_for_4)
		{
		case NPM_REPLY_HEADER_4:
			if (serialNPM.find(header, sizeof(header)))
				NPM_waiting_for_4 = NPM_REPLY_STATE_4;
			break;
		case NPM_REPLY_STATE_4:
			serialNPM.readBytes(state, sizeof(state));
			NPM_state(state[0]);

			if (bitRead(state[0], 0) == 0)
			{
				debug_outln_info(F("NPM start..."));
				result = true;
			}
			else if (bitRead(state[0], 0) == 1)
			{
				debug_outln_info(F("NPM stop..."));
				result = false;
			}
			else
			{
				result = !is_NPM_running; //DANGER BECAUSE NON INITIALISED
			}
			NPM_waiting_for_4 = NPM_REPLY_CHECKSUM_4;
			break;
		case NPM_REPLY_CHECKSUM_4:
			serialNPM.readBytes(checksum, sizeof(checksum));
			memcpy(test, header, sizeof(header));
			memcpy(&test[sizeof(header)], state, sizeof(state));
			memcpy(&test[sizeof(header) + sizeof(state)], checksum, sizeof(checksum));
			NPM_data_reader(test, 4);
			NPM_waiting_for_4 = NPM_REPLY_HEADER_4;
			if (NPM_checksum_valid_4(test))
			{
				debug_outln_info(F("Checksum OK..."));
			}
			break;
		}
	}
	return result; //ATTENTION
}

static String NPM_version_date()
{
	//debug_outln_verbose(FPSTR(DBG_TXT_START_READING), FPSTR(DBG_TXT_NPM_VERSION_DATE));
	delay(250);
	NPM_waiting_for_6 = NPM_REPLY_HEADER_6;
	debug_outln_info(F("Version NPM..."));
	NPM_cmd(PmSensorCmd2::Version);

	while (!serialNPM.available())
	{
		debug_outln("Wait for Serial...", DEBUG_MAX_INFO);
	}

	while (serialNPM.available() >= NPM_waiting_for_6)
	{
		const uint8_t constexpr header[2] = {0x81, 0x17};
		uint8_t state[1];
		uint8_t data[2];
		uint8_t checksum[1];
		uint8_t test[6];

		switch (NPM_waiting_for_6)
		{
		case NPM_REPLY_HEADER_6:
			if (serialNPM.find(header, sizeof(header)))
				NPM_waiting_for_6 = NPM_REPLY_STATE_6;
			break;
		case NPM_REPLY_STATE_6:
			serialNPM.readBytes(state, sizeof(state));
			NPM_state(state[0]);
			NPM_waiting_for_6 = NPM_REPLY_DATA_6;
			break;
		case NPM_REPLY_DATA_6:
			if (serialNPM.readBytes(data, sizeof(data)) == sizeof(data))
			{
				NPM_data_reader(data, 2);
				uint16_t NPMversion = word(data[0], data[1]);
				last_value_NPM_version = String(NPMversion);
				//debug_outln_verbose(FPSTR(DBG_TXT_END_READING), FPSTR(DBG_TXT_NPM_VERSION_DATE));
				debug_outln_info(F("Next PM Firmware: "), last_value_NPM_version);
			}
			NPM_waiting_for_6 = NPM_REPLY_CHECKSUM_6;
			break;
		case NPM_REPLY_CHECKSUM_6:
			serialNPM.readBytes(checksum, sizeof(checksum));
			memcpy(test, header, sizeof(header));
			memcpy(&test[sizeof(header)], state, sizeof(state));
			memcpy(&test[sizeof(header) + sizeof(state)], data, sizeof(data));
			memcpy(&test[sizeof(header) + sizeof(state) + sizeof(data)], checksum, sizeof(checksum));
			NPM_data_reader(test, 6);
			NPM_waiting_for_6 = NPM_REPLY_HEADER_6;
			if (NPM_checksum_valid_6(test))
			{
				debug_outln_info(F("Checksum OK..."));
			}
			break;
		}
	}
	return last_value_NPM_version;
}

static void NPM_fan_speed()
{

	NPM_waiting_for_5 = NPM_REPLY_HEADER_5;
	debug_outln_info(F("Set fan speed to 50 %..."));
	NPM_cmd(PmSensorCmd2::Speed);

	while (!serialNPM.available())
	{
		debug_outln("Wait for Serial...", DEBUG_MAX_INFO);
	}

	while (serialNPM.available() >= NPM_waiting_for_5)
	{
		const uint8_t constexpr header[2] = {0x81, 0x21};
		uint8_t state[1];
		uint8_t data[1];
		uint8_t checksum[1];
		uint8_t test[5];

		switch (NPM_waiting_for_5)
		{
		case NPM_REPLY_HEADER_5:
			if (serialNPM.find(header, sizeof(header)))
				NPM_waiting_for_5 = NPM_REPLY_STATE_5;
			break;
		case NPM_REPLY_STATE_5:
			serialNPM.readBytes(state, sizeof(state));
			NPM_state(state[0]);
			NPM_waiting_for_5 = NPM_REPLY_DATA_5;
			break;
		case NPM_REPLY_DATA_5:
			if (serialNPM.readBytes(data, sizeof(data)) == sizeof(data))
			{
				NPM_data_reader(data, 1);
			}
			NPM_waiting_for_5 = NPM_REPLY_CHECKSUM_5;
			break;
		case NPM_REPLY_CHECKSUM_5:
			serialNPM.readBytes(checksum, sizeof(checksum));
			memcpy(test, header, sizeof(header));
			memcpy(&test[sizeof(header)], state, sizeof(state));
			memcpy(&test[sizeof(header) + sizeof(state)], data, sizeof(data));
			memcpy(&test[sizeof(header) + sizeof(state) + sizeof(data)], checksum, sizeof(checksum));
			NPM_data_reader(test, 5);
			NPM_waiting_for_5 = NPM_REPLY_HEADER_5;
			if (NPM_checksum_valid_5(test))
			{
				debug_outln_info(F("Checksum OK..."));
			}
			break;
		}
	}
}



// hibbes-Patch: static dropped — fetchSensorNPM is now in sensors/npm.cpp and needs cross-TU access
String NPM_temp_humi() 
{
	uint16_t NPM_temp;
	uint16_t NPM_humi;
	NPM_waiting_for_8 = NPM_REPLY_HEADER_8;
	debug_outln_info(F("Temperature/Humidity in Next PM..."));
	NPM_cmd(PmSensorCmd2::Temphumi);
			while (!serialNPM.available())
			{
				debug_outln("Wait for Serial...", DEBUG_MAX_INFO);
			}

			while (serialNPM.available() >= NPM_waiting_for_8)
			{
				const uint8_t constexpr header[2] = {0x81, 0x14};
				uint8_t state[1];
				uint8_t data[4];
				uint8_t checksum[1];
				uint8_t test[8];

				switch (NPM_waiting_for_8)
				{
				case NPM_REPLY_HEADER_8:
					if (serialNPM.find(header, sizeof(header)))
						NPM_waiting_for_8 = NPM_REPLY_STATE_8;
					break;
				case NPM_REPLY_STATE_8:
					serialNPM.readBytes(state, sizeof(state));
					NPM_state(state[0]);
					NPM_waiting_for_8 = NPM_REPLY_BODY_8;
					break;
				case NPM_REPLY_BODY_8:
					if (serialNPM.readBytes(data, sizeof(data)) == sizeof(data))
					{
						NPM_data_reader(data, 4);
						 NPM_temp = word(data[0], data[1]);
						 NPM_humi = word(data[2], data[3]);
						debug_outln_verbose(F("Temperature (°C): "), String(NPM_temp / 100.0f));
						debug_outln_verbose(F("Relative humidity (%): "), String(NPM_humi / 100.0f));
					}
					NPM_waiting_for_8 = NPM_REPLY_CHECKSUM_8;
					break;
				case NPM_REPLY_CHECKSUM_16:
					serialNPM.readBytes(checksum, sizeof(checksum));
					memcpy(test, header, sizeof(header));
					memcpy(&test[sizeof(header)], state, sizeof(state));
					memcpy(&test[sizeof(header) + sizeof(state)], data, sizeof(data));
					memcpy(&test[sizeof(header) + sizeof(state) + sizeof(data)], checksum, sizeof(checksum));
					NPM_data_reader(test, 8);
					if (NPM_checksum_valid_8(test))
						debug_outln_info(F("Checksum OK..."));
					NPM_waiting_for_8 = NPM_REPLY_HEADER_8;
					break;
				}
			}
			return String(NPM_temp / 100.0f) + " / "+ String(NPM_humi / 100.0f);
}


/*****************************************************************
 * read IPS-7100 sensor serial and firmware date                   *
*****************************************************************/

static String IPS_version_date()
{
	debug_outln_info(F("Version IPS..."));
	String serial_data;

	IPS_cmd(PmSensorCmd3::Reset);

	if (serialIPS.available() > 0)
	{
	serial_data = serialIPS.readString();
	//Debug.println(serial_data);
	}

	int index1 = serial_data.indexOf("VERSION_NUMBER ");
	int index2 = serial_data.indexOf("\nSERIAL_NUMBER ");
	int index3 = serial_data.indexOf("\nD:");

	String version_ips = serial_data.substring(index1+15,index2-1);
	String serial_ips = serial_data.substring(index2+15,index3-1);

	last_value_IPS_version = version_ips + "/" + serial_ips;

	debug_outln_info(F("IPS-7100 Version/Serial Number: "), last_value_IPS_version);

	return last_value_IPS_version;
}

/*****************************************************************
 * disable unneeded NMEA sentences, TinyGPS++ needs GGA, RMC     *
 *****************************************************************/
static void disable_unneeded_nmea()
{
	serialGPS->println(F("$PUBX,40,GLL,0,0,0,0*5C")); // Geographic position, latitude / longitude
													  //	serialGPS->println(F("$PUBX,40,GGA,0,0,0,0*5A"));       // Global Positioning System Fix Data
	serialGPS->println(F("$PUBX,40,GSA,0,0,0,0*4E")); // GPS DOP and active satellites
													  //	serialGPS->println(F("$PUBX,40,RMC,0,0,0,0*47"));       // Recommended minimum specific GPS/Transit data
	serialGPS->println(F("$PUBX,40,GSV,0,0,0,0*59")); // GNSS satellites in view
	serialGPS->println(F("$PUBX,40,VTG,0,0,0,0*5E")); // Track made good and ground speed
}

/*****************************************************************
 * read config from spiffs                                       *
 *****************************************************************/

/* backward compatibility for the times when we stored booleans as strings */
static bool boolFromJSON(const DynamicJsonDocument &json, const __FlashStringHelper *key)
{
	if (json[key].is<const char *>())
	{
		return !strcmp_P(json[key].as<const char *>(), PSTR("true"));
	}
	return json[key].as<bool>();
}

static void readConfig(bool oldconfig = false)
{
	bool rewriteConfig = false;

	String cfgName(F("/config.json"));
	if (oldconfig)
	{
		cfgName += F(".old");
	}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
	File configFile = SPIFFS.open(cfgName, "r");
	if (!configFile)
	{
		if (!oldconfig)
		{
			return readConfig(true /* oldconfig */);
		}

		debug_outln_error(F("failed to open config file."));
		return;
	}

	debug_outln_info(F("opened config file..."));
	DynamicJsonDocument json(JSON_BUFFER_SIZE);
	DeserializationError err = deserializeJson(json, configFile.readString());
	debug_outln_info(F("parsing json: "), err.f_str());
	configFile.close();
#pragma GCC diagnostic pop

	if (!err)
	{
		for (unsigned e = 0; e < sizeof(configShape) / sizeof(configShape[0]); ++e)
		{
			ConfigShapeEntry c;
			memcpy_P(&c, &configShape[e], sizeof(ConfigShapeEntry));
			if (json[c.cfg_key()].isNull())
			{
				continue;
			}
			switch (c.cfg_type)
			{
			case Config_Type_Bool:
				*(c.cfg_val.as_bool) = boolFromJSON(json, c.cfg_key());
				break;
			case Config_Type_UInt:
			case Config_Type_Time:
				*(c.cfg_val.as_uint) = json[c.cfg_key()].as<unsigned int>();
				break;
			case Config_Type_String:
			case Config_Type_Password:
				strncpy(c.cfg_val.as_str, json[c.cfg_key()].as<const char *>(), c.cfg_len);
				c.cfg_val.as_str[c.cfg_len] = '\0';
				break;
			};
		}

		if (cfg::debug > DEBUG_MIN_INFO)
		{
			serializeJsonPretty(json, Debug); Debug.print('\n');
		}

		String writtenVersion(json["SOFTWARE_VERSION"].as<const char *>());
		if (writtenVersion.length() && writtenVersion[0] == 'N' && SOFTWARE_VERSION != writtenVersion)
		{
			debug_outln_info(F("Rewriting old config from: "), writtenVersion);
			// would like to do that, but this would wipe firmware.old which the two stage loader
			// might still need
			// SPIFFS.format();
			rewriteConfig = true;
		}
		if (cfg::sending_intervall_ms < READINGTIME_SDS_MS)
		{
			cfg::sending_intervall_ms = READINGTIME_SDS_MS;
		}
		if (strcmp_P(cfg::senseboxid, PSTR("00112233445566778899aabb")) == 0)
		{
			cfg::senseboxid[0] = '\0';
			cfg::send2sensemap = false;
			rewriteConfig = true;
		}
		if (strlen(cfg::measurement_name_influx) == 0)
		{
			strcpy_P(cfg::measurement_name_influx, MEASUREMENT_NAME_INFLUX);
			rewriteConfig = true;
		}
		if (strcmp_P(cfg::host_influx, PSTR("api.luftdaten.info")) == 0)
		{
			cfg::host_influx[0] = '\0';
			cfg::send2influx = false;
			rewriteConfig = true;
		}
		if (boolFromJSON(json, F("pm24_read")) || boolFromJSON(json, F("pms32_read")))
		{
			cfg::pms_read = true;
			rewriteConfig = true;
		}
		if (boolFromJSON(json, F("bmp280_read")) || boolFromJSON(json, F("bme280_read")))
		{
			cfg::bmx280_read = true;
			rewriteConfig = true;
		}

		if (strlen(cfg::fs_ssid) == 0)
		{
			snprintf_P(cfg::fs_ssid, LEN_FS_SSID, PSTR("%s%s"), SSID_BASENAME, esp_chipid.c_str());
			debug_outln_info(F("Setting default AP SSID to "), cfg::fs_ssid);
			rewriteConfig = true;
		}
	}
	else
	{
		debug_outln_error(F("failed to load json config"));

		if (!oldconfig)
		{
			return readConfig(true /* oldconfig */);
		}
	}

	if (rewriteConfig)
	{
		writeConfig();
	}
}

static void init_config()
{

	debug_outln_info(F("mounting FS..."));

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#if defined(ESP32)
	bool spiffs_begin_ok = SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED);
#else
	bool spiffs_begin_ok = SPIFFS.begin();
#endif

#pragma GCC diagnostic pop

	if (!spiffs_begin_ok)
	{
		debug_outln_error(F("failed to mount FS"));
		return;
	}
	readConfig();
}

/*****************************************************************
 * write config to spiffs                                        *
 *****************************************************************/
bool writeConfig()
{
	DynamicJsonDocument json(JSON_BUFFER_SIZE);
	debug_outln_info(F("Saving config..."));
	json["SOFTWARE_VERSION"] = SOFTWARE_VERSION;

	for (unsigned e = 0; e < sizeof(configShape) / sizeof(configShape[0]); ++e)
	{
		ConfigShapeEntry c;
		memcpy_P(&c, &configShape[e], sizeof(ConfigShapeEntry));
		switch (c.cfg_type)
		{
		case Config_Type_Bool:
			json[c.cfg_key()].set(*c.cfg_val.as_bool);
			break;
		case Config_Type_UInt:
		case Config_Type_Time:
			json[c.cfg_key()].set(*c.cfg_val.as_uint);
			break;
		case Config_Type_Password:
		case Config_Type_String:
			json[c.cfg_key()].set(c.cfg_val.as_str);
			break;
		};
	}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

	SPIFFS.remove(F("/config.json.old"));
	SPIFFS.rename(F("/config.json"), F("/config.json.old"));

	File configFile = SPIFFS.open(F("/config.json"), "w");
	if (configFile)
	{
		serializeJson(json, configFile);
		configFile.close();
		debug_outln_info(F("Config written successfully."));
	}
	else
	{
		debug_outln_error(F("failed to open config file for writing"));
		return false;
	}

#pragma GCC diagnostic pop

	return true;
}

/*****************************************************************
 * Prepare information for data Loggers                          *
 *****************************************************************/
static void createLoggerConfigs()
{
#if defined(ESP8266)
	auto new_session = []()
	{ return new BearSSL::Session; };
#else
	auto new_session = []()
	{ return nullptr; };
#endif
	if (cfg::send2dusti)
	{
		loggerConfigs[LoggerSensorCommunity].destport = 80;
		if (cfg::ssl_dusti)
		{
			loggerConfigs[LoggerSensorCommunity].destport = 443;
			loggerConfigs[LoggerSensorCommunity].session = new_session();
		}
	}
	loggerConfigs[LoggerMadavi].destport = PORT_MADAVI;
	if (cfg::send2madavi && cfg::ssl_madavi)
	{
		loggerConfigs[LoggerMadavi].destport = 443;
		loggerConfigs[LoggerMadavi].session = new_session();
	}
	loggerConfigs[LoggerSensemap].destport = PORT_SENSEMAP;
	loggerConfigs[LoggerSensemap].session = new_session();
	loggerConfigs[LoggerFSapp].destport = PORT_FSAPP;
	loggerConfigs[Loggeraircms].destport = PORT_AIRCMS;
	loggerConfigs[LoggerInflux].destport = cfg::port_influx;
	if (cfg::send2influx && cfg::ssl_influx)
	{
		loggerConfigs[LoggerInflux].session = new_session();
	}
	loggerConfigs[LoggerCustom].destport = cfg::port_custom;
	if (cfg::send2custom && (cfg::ssl_custom || (cfg::port_custom == 443)))
	{
		loggerConfigs[LoggerCustom].session = new_session();
	}
	// hibbes-Patch (Issue #16): Wunderground PWS-API ist immer HTTPS
	loggerConfigs[LoggerWunderground].destport = PORT_WUNDERGROUND;
	if (cfg::send2wunderground)
	{
		loggerConfigs[LoggerWunderground].session = new_session();
	}
}




void sensor_restart()
{
#if defined(ESP8266)
	WiFi.disconnect();
	WiFi.mode(WIFI_OFF);
	delay(100);
#endif

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

	SPIFFS.end();

#pragma GCC diagnostic pop

	if (cfg::npm_read)
	{
		serialNPM.end();
	}
	else
	{
		serialSDS.end();
	}
	debug_outln_info(F("Restart."));
	delay(500);
	ESP.restart();
	// should not be reached
	while (true)
	{
		yield();
	}
}



/*****************************************************************
 * Webserver read serial ring buffer                             *
 *****************************************************************/
static void webserver_serial()
{
	String s(Debug.popLines());

	server.send(s.length() ? 200 : 204, FPSTR(TXT_CONTENT_TYPE_TEXT_PLAIN), s);
}



/*****************************************************************
 * Webserver reset: webserver_reset() moved to web/pages/reset.cpp (Issue #18)
 *****************************************************************/

/*****************************************************************
 * Webserver data.json                                           *
 *****************************************************************/
static void webserver_data_json()
{
	String s1;
	unsigned long age = 0;

	debug_outln_info(F("ws: data json..."));
	if (!count_sends)
	{
		s1 = FPSTR(data_first_part);
		s1 += "]}";
		age = cfg::sending_intervall_ms - msSince(starttime);
		if (age > cfg::sending_intervall_ms)
		{
			age = 0;
		}
		age = 0 - age;
	}
	else
	{
		s1 = last_data_string;
		age = msSince(starttime);
		if (age > cfg::sending_intervall_ms)
		{
			age = 0;
		}
	}
	String s2 = F(", \"age\":\"");
	s2 += String((long)((age + 500) / 1000));
	s2 += F("\", \"sensordatavalues\"");
	s1.replace(F(", \"sensordatavalues\""), s2);
	server.send(200, FPSTR(TXT_CONTENT_TYPE_JSON), s1);
}

/*****************************************************************
 * Webserver metrics endpoint                                    *
 *****************************************************************/
static void webserver_metrics_endpoint()
{
	debug_outln_info(F("ws: /metrics"));
	RESERVE_STRING(page_content, XLARGE_STR);
	page_content = F("software_version{version=\"" SOFTWARE_VERSION_STR "\",$i} 1\nuptime_ms{$i} $u\nsending_intervall_ms{$i} $s\nnumber_of_measurements{$i} $c\n");
	String id(F("node=\"" SENSOR_BASENAME));
	id += esp_chipid;
	id += '\"';
	page_content.replace("$i", id);
	page_content.replace("$u", String(msSince(time_point_device_start_ms)));
	page_content.replace("$s", String(cfg::sending_intervall_ms));
	page_content.replace("$c", String(count_sends));
	DynamicJsonDocument json2data(JSON_BUFFER_SIZE);
	DeserializationError err = deserializeJson(json2data, last_data_string);
	if (!err)
	{
		for (JsonObject measurement : json2data[FPSTR(JSON_SENSOR_DATA_VALUES)].as<JsonArray>())
		{
			page_content += measurement["value_type"].as<const char *>();
			page_content += '{';
			page_content += id;
			page_content += "} ";
			page_content += measurement["value"].as<const char *>();
			page_content += '\n';
		}
		page_content += F("last_sample_age_ms{");
		page_content += id;
		page_content += "} ";
		page_content += String(msSince(starttime));
		page_content += '\n';
	}
	else
	{
		debug_outln_error(FPSTR(DBG_TXT_DATA_READ_FAILED));
	}
	page_content += F("# EOF\n");
	debug_outln(page_content, DEBUG_MED_INFO);
	server.send(200, FPSTR(TXT_CONTENT_TYPE_TEXT_PLAIN), page_content);
}

/*****************************************************************
 * Webserver Images                                              *
 *****************************************************************/

static void webserver_favicon()
{
	server.sendHeader(F("Cache-Control"), F("max-age=2592000, public"));

	server.send_P(200, TXT_CONTENT_TYPE_IMAGE_PNG,
				  LUFTDATEN_INFO_LOGO_PNG, LUFTDATEN_INFO_LOGO_PNG_SIZE);
}

static void webserver_static()
{
	server.sendHeader(F("Cache-Control"), F("max-age=2592000, public"));

	if (server.arg(String('r')) == F("logo"))
	{
		server.send_P(200, TXT_CONTENT_TYPE_IMAGE_PNG,
					  LUFTDATEN_INFO_LOGO_PNG, LUFTDATEN_INFO_LOGO_PNG_SIZE);
	}
	else if (server.arg(String('r')) == F("css"))
	{
		server.send_P(200, TXT_CONTENT_TYPE_TEXT_CSS,
					  ASSET_STYLE_CSS, ASSET_STYLE_CSS_LEN);
	}
	else
	{
		webserver_not_found();
	}
}

/*****************************************************************
 * Webserver page not found                                      *
 *****************************************************************/
static void webserver_not_found()
{
	last_page_load = millis();
	debug_outln_info(F("ws: not found ..."));
	if (WiFi.status() != WL_CONNECTED)
	{
		if ((server.uri().indexOf(F("success.html")) != -1) || (server.uri().indexOf(F("detect.html")) != -1))
		{
			server.send(200, FPSTR(TXT_CONTENT_TYPE_TEXT_HTML), FPSTR(WEB_IOS_REDIRECT));
		}
		else
		{
			sendHttpRedirect();
		}
	}
	else
	{
		server.send(404, FPSTR(TXT_CONTENT_TYPE_TEXT_PLAIN), F("Not found."));
	}
}

/*****************************************************************
 * Webserver setup                                               *
 *****************************************************************/
static void setup_webserver()
{
	server.on("/", webserver_root);
	server.on(F("/config"), webserver_config);
	server.on(F("/config.json"), webserver_config_json);
	server.on(F("/wifi"), webserver_wifi);
	server.on(F("/values"), webserver_values);
	server.on(F("/status"), webserver_status);
	server.on(F("/generate_204"), webserver_config);
	server.on(F("/fwlink"), webserver_config);
	server.on(F("/debug"), webserver_debug_level);
	server.on(F("/serial"), webserver_serial);
	server.on(F("/removeConfig"), webserver_removeConfig);
	server.on(F("/reset"), webserver_reset);
	server.on(F("/data.json"), webserver_data_json);
	server.on(F("/metrics"), webserver_metrics_endpoint);
	server.on(F("/favicon.ico"), webserver_favicon);
	server.on(F(STATIC_PREFIX), webserver_static);
	server.onNotFound(webserver_not_found);

#if defined(ESP8266)
	// /update endpoint für lokale OTA-Pushes via curl -F "image=@firmware.bin" http://<sensor>/update
	// Auth folgt cfg::www_basicauth_enabled — wenn an, dann mit configurierten Creds, sonst offen
	if (cfg::www_basicauth_enabled) {
		httpUpdater.setup(&server, "/update", cfg::www_username, cfg::www_password);
	} else {
		httpUpdater.setup(&server, "/update");
	}
#endif

	debug_outln_info(F("Starting Webserver... "), WiFi.localIP().toString());
	server.begin();
}

static int selectChannelForAp()
{
	std::array<int, 14> channels_rssi;
	std::fill(channels_rssi.begin(), channels_rssi.end(), -100);

	for (unsigned i = 0; i < std::min((uint8_t)14, count_wifiInfo); i++)
	{
		if (wifiInfo[i].RSSI > channels_rssi[wifiInfo[i].channel])
		{
			channels_rssi[wifiInfo[i].channel] = wifiInfo[i].RSSI;
		}
	}

	if ((channels_rssi[1] < channels_rssi[6]) && (channels_rssi[1] < channels_rssi[11]))
	{
		return 1;
	}
	else if ((channels_rssi[6] < channels_rssi[1]) && (channels_rssi[6] < channels_rssi[11]))
	{
		return 6;
	}
	else
	{
		return 11;
	}
}

/*****************************************************************
 * WifiConfig                                                    *
 *****************************************************************/
static void wifiConfig()
{
	debug_outln_info(F("Starting WiFiManager"));
	debug_outln_info(F("AP ID: "), String(cfg::fs_ssid));
	debug_outln_info(F("Password: "), String(cfg::fs_pwd));

	wificonfig_loop = true;

	WiFi.disconnect(true);
	debug_outln_info(F("scan for wifi networks..."));
	int8_t scanReturnCode = WiFi.scanNetworks(false /* scan async */, true /* show hidden networks */);
	if (scanReturnCode < 0)
	{
		debug_outln_error(F("WiFi scan failed. Treating as empty. "));
		count_wifiInfo = 0;
	}
	else
	{
		count_wifiInfo = (uint8_t)scanReturnCode;
	}

	delete[] wifiInfo;
	wifiInfo = new struct_wifiInfo[std::max(count_wifiInfo, (uint8_t)1)];

	for (unsigned i = 0; i < count_wifiInfo; i++)
	{
		String SSID;
		uint8_t *BSSID;

		memset(&wifiInfo[i], 0, sizeof(struct_wifiInfo));
#if defined(ESP8266)
		WiFi.getNetworkInfo(i, SSID, wifiInfo[i].encryptionType,
							wifiInfo[i].RSSI, BSSID, wifiInfo[i].channel,
							wifiInfo[i].isHidden);
#else
		WiFi.getNetworkInfo(i, SSID, wifiInfo[i].encryptionType,
							wifiInfo[i].RSSI, BSSID, wifiInfo[i].channel);
#endif
		SSID.toCharArray(wifiInfo[i].ssid, sizeof(wifiInfo[0].ssid));
	}

	// Use 13 channels if locale is not "EN"
	wifi_country_t wifi;
	wifi.policy = WIFI_COUNTRY_POLICY_MANUAL;
	strcpy(wifi.cc, INTL_LANG);
	wifi.nchan = (INTL_LANG[0] == 'E' && INTL_LANG[1] == 'N') ? 11 : 13;
	wifi.schan = 1;

#if defined(ESP8266)
	wifi_set_country(&wifi);
#endif

	WiFi.mode(WIFI_AP);
	const IPAddress apIP(
		default_ip_first_octet, 
		default_ip_second_octet, 
		default_ip_third_octet, 
		default_ip_fourth_octet);
		
	WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
	WiFi.softAP(cfg::fs_ssid, cfg::fs_pwd, selectChannelForAp());
	// In case we create a unique password at first start
	debug_outln_info(F("AP Password is: "), cfg::fs_pwd);

	DNSServer dnsServer;
	// Ensure we don't poison the client DNS cache
	dnsServer.setTTL(0);
	dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
	dnsServer.start(53, "*", apIP); // 53 is port for DNS server

	setup_webserver();

	// 10 minutes timeout for wifi config
	last_page_load = millis();
	while ((millis() - last_page_load) < cfg::time_for_wifi_config + 500)
	{
		dnsServer.processNextRequest();
		server.handleClient();
#if defined(ESP8266)
		wdt_reset(); // nodemcu is alive
		MDNS.update();
#endif
		yield();
	}

	WiFi.softAPdisconnect(true);

	wifi.policy = WIFI_COUNTRY_POLICY_MANUAL;
	strcpy(wifi.cc, INTL_LANG);
	wifi.nchan = 13;
	wifi.schan = 1;

#if defined(ESP8266)
	wifi_set_country(&wifi);
#endif

	WiFi.mode(WIFI_STA);

	dnsServer.stop();
	delay(100);

	debug_outln_info(FPSTR(DBG_TXT_CONNECTING_TO), cfg::wlanssid);

	if( *cfg::wlanpwd ) // non-empty password
	{
		WiFi.begin(cfg::wlanssid, cfg::wlanpwd);
	}
	else  // empty password: WiFi AP without a password, e.g. "freifunk" or the like
	{
		WiFi.begin(cfg::wlanssid); // since somewhen, the espressif API changed semantics: no password need the 1 args call since.
	}

	debug_outln_info(F("---- Result Webconfig ----"));
	debug_outln_info(F("WLANSSID: "), cfg::wlanssid);
	debug_outln_info(FPSTR(DBG_TXT_SEP));
	debug_outln_info_bool(F("PPD: "), cfg::ppd_read);
	debug_outln_info_bool(F("SDS: "), cfg::sds_read);
	debug_outln_info_bool(F("PMS: "), cfg::pms_read);
	debug_outln_info_bool(F("HPM: "), cfg::hpm_read);
	debug_outln_info_bool(F("SPS30: "), cfg::sps30_read);
	debug_outln_info_bool(F("NPM: "), cfg::npm_read);
	debug_outln_info_bool(F("DHT: "), cfg::dht_read);
	debug_outln_info_bool(F("DS18B20: "), cfg::ds18b20_read);
	debug_outln_info_bool(F("HTU21D: "), cfg::htu21d_read);
	debug_outln_info_bool(F("AHT20: "), cfg::aht20_read);
	debug_outln_info_bool(F("BMP: "), cfg::bmp_read);
	debug_outln_info_bool(F("DNMS: "), cfg::dnms_read);
	debug_outln_info(FPSTR(DBG_TXT_SEP));
	debug_outln_info_bool(F("SensorCommunity: "), cfg::send2dusti);
	debug_outln_info_bool(F("Madavi: "), cfg::send2madavi);
	debug_outln_info_bool(F("CSV: "), cfg::send2csv);
	debug_outln_info(FPSTR(DBG_TXT_SEP));
	debug_outln_info_bool(F("Autoupdate: "), cfg::auto_update);
	debug_outln_info_bool(F("Display: "), cfg::has_display);
	debug_outln_info_bool(F("LCD 1602: "), !!lcd_1602);
	debug_outln_info(F("Debug: "), String(cfg::debug));
	wificonfig_loop = false;
}

static void waitForWifiToConnect(int maxRetries)
{
	int retryCount = 0;
	while ((WiFi.status() != WL_CONNECTED) && (retryCount < maxRetries))
	{
		delay(500);
		debug_out(".", DEBUG_MIN_INFO);
		++retryCount;
	}
}

/*****************************************************************
 * WiFi auto connecting script                                   *
 *****************************************************************/

static WiFiEventHandler disconnectEventHandler;

static void connectWifi()
{
	display_debug(F("Connecting to"), String(cfg::wlanssid));
#if defined(ESP8266)
	// Enforce Rx/Tx calibration
	system_phy_set_powerup_option(1);
	// 20dBM == 100mW == max tx power allowed in europe
	WiFi.setOutputPower(20.0f);
	if (cfg::powersave) {
		WiFi.setSleepMode(WIFI_MODEM_SLEEP);
	} else {
		WiFi.setSleepMode(WIFI_NONE_SLEEP);
	}
	WiFi.setPhyMode(WIFI_PHY_MODE_11N);
	delay(100);

	disconnectEventHandler = WiFi.onStationModeDisconnected([](const WiFiEventStationModeDisconnected &evt)
															{ last_disconnect_reason = evt.reason; });
#endif
	if (WiFi.getAutoConnect())
	{
		WiFi.setAutoConnect(false);
	}
	if (!WiFi.getAutoReconnect())
	{
		WiFi.setAutoReconnect(true);
	}

	// Use 13 channels for connect to known AP
	wifi_country_t wifi;
	wifi.policy = WIFI_COUNTRY_POLICY_MANUAL;
	strcpy(wifi.cc, INTL_LANG);
	wifi.nchan = 13;
	wifi.schan = 1;

#if defined(ESP8266)
	wifi_set_country(&wifi);
#endif

	WiFi.mode(WIFI_STA);

#if defined(ESP8266)
	WiFi.hostname(cfg::fs_ssid);
	if (addr_static_ip.fromString(cfg::static_ip) && addr_static_subnet.fromString(cfg::static_subnet) && addr_static_gateway.fromString(cfg::static_gateway) && addr_static_dns.fromString(cfg::static_dns))
	{
		//ESP argument order is: ip, gateway, subnet, dns1
		//Arduino arg order is:  ip, dns, gateway, subnet.
		//To allow compatibility, check first octet of 3rd arg. If 255, interpret as ESP order, otherwise Arduino order.
		//Here ESP order is used
		WiFi.config(addr_static_ip, addr_static_gateway, addr_static_subnet, addr_static_dns, addr_static_dns);
	}
#endif

#if defined(ESP32)
	WiFi.setHostname(cfg::fs_ssid);
#endif

	WiFi.begin(cfg::wlanssid, cfg::wlanpwd); // Start WiFI

	debug_outln_info(FPSTR(DBG_TXT_CONNECTING_TO), cfg::wlanssid);

	waitForWifiToConnect(40);
	debug_outln_info(emptyString);
	if (WiFi.status() != WL_CONNECTED)
	{
		String fss(cfg::fs_ssid);
		display_debug(fss.substring(0, 16), fss.substring(16));

		wifi.policy = WIFI_COUNTRY_POLICY_AUTO;

#if defined(ESP8266)
		wifi_set_country(&wifi);
#endif

		wifiConfig();
		if (WiFi.status() != WL_CONNECTED)
		{
			waitForWifiToConnect(20);
			debug_outln_info(emptyString);
		}
	}
	debug_outln_info(F("WiFi connected, IP is: "), WiFi.localIP().toString());
	last_signal_strength = WiFi.RSSI();

	if (MDNS.begin(cfg::fs_ssid))
	{
		MDNS.addService("http", "tcp", 80);
		MDNS.addServiceTxt("http", "tcp", "PATH", "/config");
	}
}

static WiFiClient *getNewLoggerWiFiClient(const LoggerEntry logger)
{

	WiFiClient *_client;
	if (loggerConfigs[logger].session)
	{
		_client = new WiFiClientSecure;
#if defined(ESP8266)
		static_cast<WiFiClientSecure *>(_client)->setSession(loggerConfigs[logger].session);
		static_cast<WiFiClientSecure *>(_client)->setBufferSizes(1024, TCP_MSS > 1024 ? 2048 : 1024);
		//static_cast<WiFiClientSecure *>(_client)->setSSLVersion(BR_TLS12, BR_TLS12);
		switch (logger)
		{
		case Loggeraircms:
		case LoggerInflux:
		case LoggerCustom:
		case LoggerFSapp:
		case LoggerWunderground:
			// hibbes-Patch (Issue #16): WU's CDN nutzt nicht die airrohr-CA-Trust-Anchor.
			// setInsecure() trade-off: kein Cert-Verify, dafür funktioniert TLS-Handshake.
			static_cast<WiFiClientSecure *>(_client)->setInsecure();
			break;
		default:
			configureCACertTrustAnchor(static_cast<WiFiClientSecure *>(_client));
		}
#endif
	}
	else
	{
		_client = new WiFiClient;
	}
	return _client;
}

/*****************************************************************
 * send data to rest api                                         *
 *****************************************************************/
static unsigned long sendData(const LoggerEntry logger, const String &data, const int pin, const char *host, const char *url)
{

	unsigned long start_send = millis();
	const __FlashStringHelper *contentType;
	int result = 0;

	String s_Host(FPSTR(host));
	String s_url(FPSTR(url));

	switch (logger)
	{
	case Loggeraircms:
		contentType = FPSTR(TXT_CONTENT_TYPE_TEXT_PLAIN);
		break;
	case LoggerInflux:
		contentType = FPSTR(TXT_CONTENT_TYPE_INFLUXDB);
		break;
	default:
		contentType = FPSTR(TXT_CONTENT_TYPE_JSON);
		break;
	}

	// Issue #11: wenn das cycle-Budget überschritten ist, früh raus —
	// nachfolgende Pushes blockieren sonst bis zum Watchdog-Reset.
	if (cycle_send_deadline && millis() > cycle_send_deadline)
	{
		debug_outln_info(F("## Cycle deadline exceeded, skipping push"));
		return 0;
	}

	std::unique_ptr<WiFiClient> client(getNewLoggerWiFiClient(logger));

	HTTPClient http;
	// Issue #11: Per-Push-Timeout 20 s → 15 s. SC mit TLS-Handshake braucht
	// realistisch 3-8 s, 15 s lässt Slow-Network-Headroom ohne den Cycle zu killen.
	http.setTimeout(15 * 1000);
	http.setUserAgent(SOFTWARE_VERSION + '/' + esp_chipid + '/' + esp_mac_id);
	// Issue #10: setReuse(true) erlaubt der HTTPClient-Instanz, die TCP-Connection
	// für mehrere Requests innerhalb des Lifecycles offen zu halten. In der aktuellen
	// Architektur lebt die HTTPClient nur eine sendData()-Call lang, der echte Gewinn
	// kommt aus dem darunterliegenden BearSSL-Session-Cache (loggerConfigs[logger].
	// session), der Handshakes über Cycle-Grenzen hinweg resumed. Das Flag schadet
	// nicht und macht den Code zukunftssicher falls sendData() später mehrere
	// Sub-Requests bündelt (Issue #11 ESPAsyncTCP).
	http.setReuse(true);
	bool send_success = false;
	if (logger == LoggerCustom && (*cfg::user_custom || *cfg::pwd_custom))
	{
		http.setAuthorization(cfg::user_custom, cfg::pwd_custom);
	}
	if (logger == LoggerInflux && (*cfg::user_influx || *cfg::pwd_influx))
	{
		http.setAuthorization(cfg::user_influx, cfg::pwd_influx);
	}
	// hibbes-Patch (Issue #4): pro-Cycle-Tracking für silent-failure-Detection
	cycle_send_attempts++;

	if (http.begin(*client, s_Host, loggerConfigs[logger].destport, s_url, !!loggerConfigs[logger].session))
	{
		http.addHeader(F("Content-Type"), contentType);
		http.addHeader(F("X-Sensor"), String(F(SENSOR_BASENAME)) + esp_chipid);
		http.addHeader(F("X-MAC-ID"), String(F(SENSOR_BASENAME)) + esp_mac_id);
		if (pin)
		{
			http.addHeader(F("X-PIN"), String(pin));
		}

		result = http.POST(data);

		if (result >= HTTP_CODE_OK && result <= HTTP_CODE_ALREADY_REPORTED)
		{
			debug_outln_info(F("Succeeded - "), s_Host);
			send_success = true;
			cycle_send_successes++;  // hibbes-Patch (Issue #4)
		}
		else
		{
			debug_outln_info(F("Request failed with error: "), String(result));
			if (result >= HTTP_CODE_BAD_REQUEST)
			{
				debug_outln_info(F("Details:"), http.getString());
			}
		}
		http.end();
	}
	else
	{
		debug_outln_info(F("Failed connecting to "), s_Host);
	}

	// hibbes-Patch (Issue #4): error-counter auch bei http.begin()-Fehler hochzählen
	// (vorher: nur bei HTTP-Fehlercode > 0). Connect-Failures sind vermutlich Symptom
	// von WLAN-State-Issues und müssen sichtbar sein.
	if (!send_success)
	{
		loggerConfigs[logger].errors++;
		if (result != 0)
		{
			last_sendData_returncode = result;
		}
	}

	return millis() - start_send;
}

/*****************************************************************
 * send single sensor data to sensor.community api                *
 *****************************************************************/
static unsigned long sendSensorCommunity(const String &data, const int pin, const __FlashStringHelper *sensorname, const char *replace_str)
{
	unsigned long sum_send_time = 0;

	if (cfg::send2dusti && data.length())
	{
		debug_outln_info(F("## Sending to sensor.community - "), sensorname);

		// Issue #8: ArduinoJson-Roundtrip statt String-Replace-Hack.
		// Eingangs-`data` ist die rohe Sequenz aus add_Value2Json():
		//   {"value_type":"SDS_P1","value":"3.5"},{"value_type":"SDS_P2","value":"1.2"},
		// (mit trailing comma). Wir wrappen sie in ein gültiges Array, parsen,
		// strippen Sensor-Prefixes pro value_type-Key (statt String-weit), bauen
		// die finale "{software_version, sensordatavalues:[...]}"-Struktur und
		// serialisieren zurück. Damit ist ein zufälliger Substring im "value"-Feld
		// nicht mehr gefährdet.
		String arr_input;
		arr_input.reserve(data.length() + 2);
		arr_input = '[';
		arr_input += data;
		arr_input.remove(arr_input.length() - 1); // trailing comma
		arr_input += ']';

		DynamicJsonDocument doc(JSON_BUFFER_SIZE);
		DeserializationError err = deserializeJson(doc, arr_input);
		if (err)
		{
			debug_outln_error(F("SC: JSON parse failed"));
			debug_outln_verbose(F("err: "), String(err.c_str()));
			return 0;
		}

		DynamicJsonDocument out(JSON_BUFFER_SIZE);
		out[F("software_version")] = SOFTWARE_VERSION;
		JsonArray svv = out.createNestedArray(F("sensordatavalues"));
		const size_t prefix_len = replace_str ? strlen(replace_str) : 0;
		for (JsonObject m : doc.as<JsonArray>())
		{
			const char *vt = m["value_type"] | "";
			JsonObject n = svv.createNestedObject();
			if (prefix_len && strncmp(vt, replace_str, prefix_len) == 0)
			{
				// Copy stripped key as String — ArduinoJson v6 kopiert
				// String-rvalue in den Doc-Buffer, so dass die Lifetime
				// nach unserer Loop sicher ist.
				n[F("value_type")] = String(vt + prefix_len);
			}
			else
			{
				n[F("value_type")] = String(vt);
			}
			n[F("value")] = String(m["value"] | "");
		}

		String json_out;
		json_out.reserve(measureJson(out) + 1);
		serializeJson(out, json_out);

		sum_send_time = sendData(LoggerSensorCommunity, json_out, pin, HOST_SENSORCOMMUNITY, URL_SENSORCOMMUNITY);
	}

	return sum_send_time;
}

/*****************************************************************
 * send data to mqtt api                                         *
 *****************************************************************/
// rejected (see issue #33)

/*****************************************************************
 * send data to influxdb                                         *
 *****************************************************************/
static void create_influxdb_string_from_data(String &data_4_influxdb, const String &data)
{
	DynamicJsonDocument json2data(JSON_BUFFER_SIZE);
	DeserializationError err = deserializeJson(json2data, data);
	if (!err)
	{
		data_4_influxdb += cfg::measurement_name_influx;
		data_4_influxdb += F(",node=" SENSOR_BASENAME);
		data_4_influxdb += esp_chipid + " ";
		for (JsonObject measurement : json2data[FPSTR(JSON_SENSOR_DATA_VALUES)].as<JsonArray>())
		{
			data_4_influxdb += measurement["value_type"].as<const char *>();
			data_4_influxdb += "=";

			if (isNumeric(measurement["value"]))
			{
				//send numerics without quotes
				data_4_influxdb += measurement["value"].as<const char *>();
			}
			else
			{
				//quote string values
				data_4_influxdb += "\"";
				data_4_influxdb += measurement["value"].as<const char *>();
				data_4_influxdb += "\"";
			}
			data_4_influxdb += ",";
		}
		if ((unsigned)(data_4_influxdb.lastIndexOf(',') + 1) == data_4_influxdb.length())
		{
			data_4_influxdb.remove(data_4_influxdb.length() - 1);
		}

		data_4_influxdb += '\n';
	}
	else
	{
		debug_outln_error(FPSTR(DBG_TXT_DATA_READ_FAILED));
	}
}

/*****************************************************************
 * send data as csv to serial out                                *
 *****************************************************************/
static void send_csv(const String &data)
{
	DynamicJsonDocument json2data(JSON_BUFFER_SIZE);
	DeserializationError err = deserializeJson(json2data, data);
	debug_outln_info(F("CSV Output: "), data);
	if (!err)
	{
		String headline = F("Timestamp_ms;");
		String valueline(act_milli);
		valueline += ';';
		for (JsonObject measurement : json2data[FPSTR(JSON_SENSOR_DATA_VALUES)].as<JsonArray>())
		{
			headline += measurement["value_type"].as<const char *>();
			headline += ';';
			valueline += measurement["value"].as<const char *>();
			valueline += ';';
		}
		static bool first_csv_line = true;
		if (first_csv_line)
		{
			if (headline.length() > 0)
			{
				headline.remove(headline.length() - 1);
			}
			Debug.println(headline);
			first_csv_line = false;
		}
		if (valueline.length() > 0)
		{
			valueline.remove(valueline.length() - 1);
		}
		Debug.println(valueline);
	}
	else
	{
		debug_outln_error(FPSTR(DBG_TXT_DATA_READ_FAILED));
	}
}

/*****************************************************************
 * read DHT22 sensor values                                      *
 *****************************************************************/
// fetchSensorDHT ausgelagert nach sensors/dht22.cpp (Issue #7 Phase 1, Schiller-Gymnasium-Refactor)

/*****************************************************************
 * read HTU21D sensor values                                     *
 *****************************************************************/
// fetchSensorHTU21D ausgelagert nach sensors/htu21d.cpp (Issue #7 Phase 1, Schiller-Gymnasium-Refactor)

// fetchSensorAHT20 ist ausgelagert nach sensors/aht20.cpp
// (Issue #7 Phase 1 Pilot, hibbes-Fork-Refactor)

/*****************************************************************
 * read BMP180 sensor values                                     *
 *****************************************************************/
// fetchSensorBMP ausgelagert nach sensors/bmp180.cpp (Issue #7 Phase 1, Schiller-Gymnasium-Refactor)

/*****************************************************************
 * read SHT3x sensor values                                      *
 *****************************************************************/
// fetchSensorSHT3x ausgelagert nach sensors/sht3x.cpp (Issue #7 Phase 1, Schiller-Gymnasium-Refactor)

/*****************************************************************
 * read SCD30 sensor values                                      *
 *****************************************************************/
// fetchSensorSCD30 ausgelagert nach sensors/scd30.cpp (Issue #7 Phase 1, Schiller-Gymnasium-Refactor)

/*****************************************************************
 * read BMP280/BME280 sensor values                              *
 *****************************************************************/
// fetchSensorBMX280 ausgelagert nach sensors/bmx280.cpp (Issue #7 Phase 1, Schiller-Gymnasium-Refactor)

/*****************************************************************
 * read DS18B20 sensor values                                    *
 *****************************************************************/
// fetchSensorDS18B20 ausgelagert nach sensors/ds18b20.cpp (Issue #7 Phase 1, Schiller-Gymnasium-Refactor)

/*****************************************************************
 * read SDS011 sensor values                                     *
 *****************************************************************/
// fetchSensorSDS ausgelagert nach sensors/sds011.cpp (Issue #7 Phase 1, Schiller-Gymnasium-Refactor)

/*****************************************************************
 * read Plantronic PM sensor sensor values                       *
 *****************************************************************/
// fetchSensorPMS ausgelagert nach sensors/pms.cpp (Issue #7 Phase 1, Schiller-Gymnasium-Refactor)

/*****************************************************************
 * read Honeywell PM sensor sensor values                        *
 *****************************************************************/
// fetchSensorHPM ausgelagert nach sensors/hpm.cpp (Issue #7 Phase 1, Schiller-Gymnasium-Refactor)

/*****************************************************************
 * read Tera Sensor Next PM sensor sensor values                 *
 *****************************************************************/

// fetchSensorNPM ausgelagert nach sensors/npm.cpp (Issue #7 Phase 1, Schiller-Gymnasium-Refactor)


/*****************************************************************
 * read Piera Systems IPS-7100 sensor sensor values                 *
 *****************************************************************/

// fetchSensorIPS ausgelagert nach sensors/ips.cpp (Issue #7 Phase 1, Schiller-Gymnasium-Refactor)

/*****************************************************************
 * read PPD42NS sensor values                                    *
 *****************************************************************/
// fetchSensorPPD ausgelagert nach sensors/ppd42ns.cpp (Issue #7 Phase 1, Schiller-Gymnasium-Refactor)

/*****************************************************************
   read SPS30 PM sensor values
 *****************************************************************/
// fetchSensorSPS30 ausgelagert nach sensors/sps30.cpp (Issue #7 Phase 1, Schiller-Gymnasium-Refactor)

/*****************************************************************
   read DNMS values
 *****************************************************************/
// fetchSensorDNMS ausgelagert nach sensors/dnms.cpp (Issue #7 Phase 1, Schiller-Gymnasium-Refactor)

/*****************************************************************
 * read GPS sensor values                                        *
 *****************************************************************/
// fetchSensorGPS ausgelagert nach sensors/gps.cpp (Issue #7 Phase 1, Schiller-Gymnasium-Refactor)

/*****************************************************************
 * OTAUpdate                                                     *
 *****************************************************************/

static bool fwDownloadStream(WiFiClientSecure &client, const String &url, Stream *ostream)
{

	HTTPClient http;
	int bytes_written = -1;

	// work with 128kbit/s downlinks
	http.setTimeout(60 * 1000);
	String agent(SOFTWARE_VERSION);
	agent += ' ';
	agent += esp_chipid;
	agent += "/";
	agent += esp_mac_id;
	agent += ' ';
	if (cfg::npm_read)
	{
		agent += NPM_version_date();
	}else if (cfg::ips_read)
	{
		agent += IPS_version_date();

	}
	else
	{
		agent += SDS_version_date();
	}
	agent += ' ';
	agent += String(cfg::current_lang);
	agent += ' ';
	agent += String(CURRENT_LANG);
	agent += ' ';
	if (cfg::use_beta)
	{
		agent += F("BETA");
	}

	http.setUserAgent(agent);
	http.setReuse(false);

	// hibbes-Patch: cfg::ota_host kann den Default-Server überschreiben (Issue #2).
	String ota_host_eff = strlen(cfg::ota_host) ? String(cfg::ota_host) : String(FPSTR(FW_DOWNLOAD_HOST));
	debug_outln_verbose(F("HTTP GET: "), ota_host_eff + ':' + String(FW_DOWNLOAD_PORT) + url);

	if (http.begin(client, ota_host_eff, FW_DOWNLOAD_PORT, url))
	{
		int r = http.GET();
		debug_outln_verbose(F("GET r: "), String(r));
		last_update_returncode = r;
		if (r == HTTP_CODE_OK)
		{
			bytes_written = http.writeToStream(ostream);
		}
		http.end();
	}

	debug_outln_verbose(F("bytes written: "), String(bytes_written));

	if (bytes_written > 0)
		return true;

	last_update_returncode = bytes_written ;
	Debug.println( http.errorToString(bytes_written) );
	return false;
}

static bool fwDownloadStreamFile(WiFiClientSecure &client, const String &url, const String &fname)
{

	String fname_new(fname);
	fname_new += F(".new");
	bool downloadSuccess = false;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
	File fwFile = SPIFFS.open(fname_new, "w");
	if (fwFile)
	{
		downloadSuccess = fwDownloadStream(client, url, &fwFile);
		fwFile.close();
		if (downloadSuccess)
		{
			SPIFFS.remove(fname);
			SPIFFS.rename(fname_new, fname);
			debug_outln_info(F("Success downloading: "), url);
		}
	}

	if (downloadSuccess)
		return true;

	SPIFFS.remove(fname_new);
#pragma GCC diagnostic pop
	return false;
}

static void twoStageOTAUpdate()
{

	if (!cfg::auto_update)
		return;

#if defined(ESP8266)
	debug_outln_info(F("twoStageOTAUpdate"));

	String lang_variant(cfg::current_lang);
	if (lang_variant.length() != 2)
	{
		lang_variant = CURRENT_LANG;
	}
	lang_variant.toLowerCase();

	String fetch_name(F(OTA_BASENAME "/update/latest_"));
	if (cfg::use_beta)
	{
		fetch_name = F(OTA_BASENAME "/beta/latest_");
	}
	fetch_name += lang_variant;
	fetch_name += F(".bin");

	WiFiClientSecure client;
	BearSSL::Session clientSession;

	client.setBufferSizes(1024, TCP_MSS > 1024 ? 2048 : 1024);
	client.setCiphersLessSecure();
	client.setSession(&clientSession);
	configureCACertTrustAnchor(&client);

	String fetch_md5_name(fetch_name);
	fetch_md5_name += F(".md5");

	StreamString newFwmd5;
	if (!fwDownloadStream(client, fetch_md5_name, &newFwmd5))
		return;

	newFwmd5.trim();
	if (newFwmd5 == ESP.getSketchMD5())
	{
		display_debug(FPSTR(DBG_TXT_UPDATE), FPSTR(DBG_TXT_UPDATE_NO_UPDATE));
		debug_outln_verbose(F("No newer version available."));
		return;
	}

	debug_outln_info(F("Update md5: "), newFwmd5);
	debug_outln_info(F("Sketch md5: "), ESP.getSketchMD5());

	// We're entering update phase, kill off everything else
	WiFiUDP::stopAll();
	WiFiClient::stopAllExcept(&client);
	delay(100);

	String firmware_name(F("/firmware.bin"));
	String firmware_md5(F("/firmware.bin.md5"));
	String loader_name(F("/loader.bin"));
	if (!fwDownloadStreamFile(client, fetch_name, firmware_name))
		return;
	if (!fwDownloadStreamFile(client, fetch_md5_name, firmware_md5))
		return;
	if (!fwDownloadStreamFile(client, FPSTR(FW_2ND_LOADER_URL), loader_name))
		return;

		// SPIFFS is deprecated, we know
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
	File fwFile = SPIFFS.open(firmware_name, "r");
	if (!fwFile)
	{
		SPIFFS.remove(firmware_name);
		SPIFFS.remove(firmware_md5);
		debug_outln_error(F("Failed reopening fw file.."));
		return;
	}
	size_t fwSize = fwFile.size();
	MD5Builder md5;
	md5.begin();
	md5.addStream(fwFile, fwSize);
	md5.calculate();
	fwFile.close();
	String md5String = md5.toString();

	// Firmware is always at least 128 kB and padded to 16 bytes
	if (fwSize < (1 << 17) || (fwSize % 16 != 0) || newFwmd5 != md5String)
	{
		debug_outln_info(F("FW download failed validation.. deleting"));
		SPIFFS.remove(firmware_name);
		SPIFFS.remove(firmware_md5);
		return;
	}

	StreamString loaderMD5;
	if (!fwDownloadStream(client, String(FPSTR(FW_2ND_LOADER_URL)) + F(".md5"), &loaderMD5))
		return;

	loaderMD5.trim();

	debug_outln_info(F("launching 2nd stage"));
	if (!launchUpdateLoader(loaderMD5))
	{
		debug_outln_error(FPSTR(DBG_TXT_UPDATE_FAILED));
		display_debug(FPSTR(DBG_TXT_UPDATE), FPSTR(DBG_TXT_UPDATE_FAILED));
		SPIFFS.remove(firmware_name);
		SPIFFS.remove(firmware_md5);
		return;
	}
#pragma GCC diagnostic pop

	sensor_restart();
#endif
}

static String displayGenerateFooter(unsigned int screen_count)
{
	String display_footer;
	for (unsigned int i = 0; i < screen_count; ++i)
	{
		display_footer += (i != (next_display_count % screen_count)) ? " . " : " o ";
	}
	return display_footer;
}

/*****************************************************************
 * display values                                                *
 *****************************************************************/
static void display_values()
{
	float t_value = -128.0;
	float h_value = -1.0;
	float p_value = -1.0;
	String t_sensor, h_sensor, p_sensor;
	float pm001_value = -1.0;
	float pm003_value = -1.0;
	float pm005_value = -1.0;
	float pm25_value = -1.0;
	float pm01_value = -1.0;
	float pm04_value = -1.0;
	float pm05_value = -1.0;
	float pm10_value = -1.0;
	String pm01_sensor;
	String pm10_sensor;
	String pm25_sensor;
	String pm001_sensor;
	String pm003_sensor;
	String pm005_sensor;
	String pm05_sensor;
	float nc001_value = -1.0;
	float nc003_value = -1.0;
	float nc005_value = -1.0;
	float nc010_value = -1.0;
	float nc025_value = -1.0;
	float nc040_value = -1.0;
	float nc050_value = -1.0;
	float nc100_value = -1.0;
	float la_eq_value = -1.0;
	float la_max_value = -1.0;
	float la_min_value = -1.0;
	String la_sensor;
	float tps_value = -1.0;
	double lat_value = -200.0;
	double lon_value = -200.0;
	double alt_value = -1000.0;
	String display_header;
	String display_lines[3] = {"", "", ""};
	uint8_t screen_count = 0;
	uint8_t screens[8];
	int line_count = 0;
	debug_outln_info(F("output values to display..."));
	if (cfg::ppd_read)
	{
		pm10_value = last_value_PPD_P1;
		pm10_sensor = FPSTR(SENSORS_PPD42NS);
		pm25_value = last_value_PPD_P2;
		pm25_sensor = FPSTR(SENSORS_PPD42NS);
	}
	if (cfg::pms_read)
	{
		pm10_value = last_value_PMS_P1;
		pm10_sensor = FPSTR(SENSORS_PMSx003);
		pm25_value = last_value_PMS_P2;
		pm25_sensor = FPSTR(SENSORS_PMSx003);
	}
	if (cfg::hpm_read)
	{
		pm10_value = last_value_HPM_P1;
		pm10_sensor = FPSTR(SENSORS_HPM);
		pm25_value = last_value_HPM_P2;
		pm25_sensor = FPSTR(SENSORS_HPM);
	}
	if (cfg::npm_read)
	{
		pm01_value = last_value_NPM_P0;
		pm10_value = last_value_NPM_P1;
		pm25_value = last_value_NPM_P2;
		pm01_sensor = FPSTR(SENSORS_NPM);
		pm10_sensor = FPSTR(SENSORS_NPM);
		pm25_sensor = FPSTR(SENSORS_NPM);
		nc010_value = last_value_NPM_N1;
		nc100_value = last_value_NPM_N10;
		nc025_value = last_value_NPM_N25;
	}
	if (cfg::ips_read)
	{
		pm01_value = last_value_IPS_P0;
		pm10_value = last_value_IPS_P1;
		pm25_value = last_value_IPS_P2;
		pm001_value = last_value_IPS_P01;
		pm003_value = last_value_IPS_P03;
		pm005_value = last_value_IPS_P05;
		pm05_value = last_value_IPS_P5;
		pm001_sensor = FPSTR(SENSORS_IPS);
		pm003_sensor = FPSTR(SENSORS_IPS);
		pm005_sensor = FPSTR(SENSORS_IPS);
		pm01_sensor = FPSTR(SENSORS_IPS);
		pm10_sensor = FPSTR(SENSORS_IPS);
		pm25_sensor = FPSTR(SENSORS_IPS);
		pm05_sensor = FPSTR(SENSORS_IPS);
		nc010_value = last_value_IPS_N1;
		nc100_value = last_value_IPS_N10;
		nc025_value = last_value_IPS_N25;
		nc001_value = last_value_IPS_N01;
		nc003_value = last_value_IPS_N03;
		nc005_value = last_value_IPS_N05;
		nc050_value = last_value_IPS_N5;
	}
	if (cfg::sps30_read)
	{
		pm10_sensor = FPSTR(SENSORS_SPS30);
		pm25_sensor = FPSTR(SENSORS_SPS30);
		pm01_value = last_value_SPS30_P0;
		pm25_value = last_value_SPS30_P2;
		pm04_value = last_value_SPS30_P4;
		pm10_value = last_value_SPS30_P1;
		nc005_value = last_value_SPS30_N05;
		nc010_value = last_value_SPS30_N1;
		nc025_value = last_value_SPS30_N25;
		nc040_value = last_value_SPS30_N4;
		nc100_value = last_value_SPS30_N10;
		tps_value = last_value_SPS30_TS;
	}
	if (cfg::sds_read)
	{
		pm10_sensor = pm25_sensor = FPSTR(SENSORS_SDS011);
		pm10_value = last_value_SDS_P1;
		pm25_value = last_value_SDS_P2;
	}
	if (cfg::dht_read)
	{
		t_sensor = h_sensor = FPSTR(SENSORS_DHT22);
		t_value = last_value_DHT_T;
		h_value = last_value_DHT_H;
	}
	if (cfg::ds18b20_read)
	{
		t_sensor = FPSTR(SENSORS_DS18B20);
		t_value = last_value_DS18B20_T;
	}
	if (cfg::htu21d_read)
	{
		h_sensor = t_sensor = FPSTR(SENSORS_HTU21D);
		t_value = last_value_HTU21D_T;
		h_value = last_value_HTU21D_H;
	}
	if (cfg::aht20_read)
	{
		h_sensor = t_sensor = FPSTR(SENSORS_AHT20);
		t_value = last_value_AHT20_T;
		h_value = last_value_AHT20_H;
	}
	if (cfg::bmp_read)
	{
		t_sensor = h_sensor = FPSTR(SENSORS_BMP180);
		t_value = last_value_BMP_T;
		p_value = last_value_BMP_P;
	}
	if (cfg::bmx280_read)
	{
		t_sensor = p_sensor = FPSTR(SENSORS_BMP280);
		t_value = last_value_BMX280_T;
		p_value = last_value_BMX280_P;
		if (bmx280.sensorID() == BME280_SENSOR_ID)
		{
			h_sensor = t_sensor = FPSTR(SENSORS_BME280);
			h_value = last_value_BME280_H;
		}
	}
	if (cfg::sht3x_read)
	{
		h_sensor = t_sensor = FPSTR(SENSORS_SHT3X);
		t_value = last_value_SHT3X_T;
		h_value = last_value_SHT3X_H;
	}
	if (cfg::dnms_read)
	{
		la_sensor = FPSTR(SENSORS_DNMS);
		la_eq_value = last_value_dnms_laeq;
		la_max_value = last_value_dnms_la_max;
		la_min_value = last_value_dnms_la_min;
	}
	if (cfg::gps_read)
	{
		lat_value = last_value_GPS_lat;
		lon_value = last_value_GPS_lon;
		alt_value = last_value_GPS_alt;
	}
	if (cfg::ppd_read || cfg::pms_read || cfg::hpm_read || cfg::sds_read)
	{
		screens[screen_count++] = 1;
	}
	if (cfg::npm_read)
	{
		screens[screen_count++] = 9;
		screens[screen_count++] = 10; 
	}
	if (cfg::ips_read)
	{
		screens[screen_count++] = 11;  //A VOIR POUR AJOUTER DES ÈCRANS
	}
	if (cfg::sps30_read)
	{
		screens[screen_count++] = 2;
	}
	if (cfg::dht_read || cfg::ds18b20_read || cfg::htu21d_read || cfg::aht20_read || cfg::bmp_read || cfg::bmx280_read || cfg::sht3x_read)
	{
		screens[screen_count++] = 3;
	}
	if (cfg::scd30_read)
	{
		screens[screen_count++] = 4;
	}
	if (cfg::gps_read)
	{
		screens[screen_count++] = 5;
	}
	if (cfg::dnms_read)
	{
		screens[screen_count++] = 6;
	}
	if (cfg::display_wifi_info)
	{
		screens[screen_count++] = 7; // Wifi info
	}
	if (cfg::display_device_info)
	{
		screens[screen_count++] = 8; // chipID, firmware and count of measurements
	}
	// update size of "screens" when adding more screens!
	if (cfg::has_display || cfg::has_sh1106 || lcd_2004)
	{
		switch (screens[next_display_count % screen_count])
		{
		case 1:
			display_header = pm25_sensor;
			if (pm25_sensor != pm10_sensor)
			{
				display_header += " / " + pm10_sensor;
			}
			display_lines[0] = std::move(tmpl(F("PM2.5: {v} µg/m³"), check_display_value(pm25_value, -1, 1, 6)));
			display_lines[1] = std::move(tmpl(F("PM10: {v} µg/m³"), check_display_value(pm10_value, -1, 1, 6)));
			display_lines[2] = emptyString;
			break;
		case 2:
			display_header = FPSTR(SENSORS_SPS30);
			display_lines[0] = "PM: " + check_display_value(pm01_value, -1, 1, 4) + " " + check_display_value(pm25_value, -1, 1, 4) + " " + check_display_value(pm04_value, -1, 1, 4) + " " + check_display_value(pm10_value, -1, 1, 4);
			display_lines[1] = "NC: " + check_display_value(nc005_value, -1, 0, 3) + " " + check_display_value(nc010_value, -1, 0, 3) + " " + check_display_value(nc025_value, -1, 0, 3) + " " + check_display_value(nc040_value, -1, 0, 3) + " " + check_display_value(nc100_value, -1, 0, 3);
			display_lines[2] = std::move(tmpl(F("TPS: {v} µm"), check_display_value(tps_value, -1, 2, 5)));
			break;
		case 3:
			display_header = t_sensor;
			if (h_sensor && t_sensor != h_sensor)
			{
				display_header += " / " + h_sensor;
			}
			if ((h_sensor && p_sensor && (h_sensor != p_sensor)) || (h_sensor == "" && p_sensor && (t_sensor != p_sensor)))
			{
				display_header += " / " + p_sensor;
			}
			if (t_sensor != "")
			{
				display_lines[line_count] = "Temp.: ";
				display_lines[line_count] += check_display_value(t_value, -128, 1, 6);
				display_lines[line_count++] += " °C";
			}
			if (h_sensor != "")
			{
				display_lines[line_count] = "Hum.:  ";
				display_lines[line_count] += check_display_value(h_value, -1, 1, 6);
				display_lines[line_count++] += " %";
			}
			if (p_sensor != "")
			{
				display_lines[line_count] = "Pres.: ";
				display_lines[line_count] += check_display_value(p_value / 100, (-1 / 100.0), 1, 6);
				display_lines[line_count++] += " hPa";
			}
			while (line_count < 3)
			{
				display_lines[line_count++] = emptyString;
			}
			break;
		case 4:
			display_header = "SCD30";
			display_lines[0] = "Temp.: ";
			display_lines[0] += check_display_value(last_value_SCD30_T, -128, 1, 5);
			display_lines[0] += " °C";
			display_lines[1] = "Hum.:  ";
			display_lines[1] += check_display_value(last_value_SCD30_H, -1, 1, 5);
			display_lines[1] += " %";
			display_lines[2] = "CO2:   ";
			display_lines[2] += check_display_value(last_value_SCD30_CO2, 0, 0, 5);
			display_lines[2] += " ppm";
			break;
		case 5:
			display_header = "NEO6M";
			display_lines[0] = "Lat: ";
			display_lines[0] += check_display_value(lat_value, -200.0, 6, 10);
			display_lines[1] = "Lon: ";
			display_lines[1] += check_display_value(lon_value, -200.0, 6, 10);
			display_lines[2] = "Alt: ";
			display_lines[2] += check_display_value(alt_value, -1000.0, 2, 10);
			break;
		case 6:
			display_header = FPSTR(SENSORS_DNMS);
			display_lines[0] = std::move(tmpl(F("LAeq: {v} db(A)"), check_display_value(la_eq_value, -1, 1, 6)));
			display_lines[1] = std::move(tmpl(F("LA_max: {v} db(A)"), check_display_value(la_max_value, -1, 1, 6)));
			display_lines[2] = std::move(tmpl(F("LA_min: {v} db(A)"), check_display_value(la_min_value, -1, 1, 6)));
			break;
		case 7:
			display_header = F("Wifi info");
			display_lines[0] = "IP: ";
			display_lines[0] += WiFi.localIP().toString();
			display_lines[1] = "SSID: ";
			display_lines[1] += WiFi.SSID();
			display_lines[2] = std::move(tmpl(F("Signal: {v} %"), String(calcWiFiSignalQuality(last_signal_strength))));
			break;
		case 8:
			display_header = F("Device Info");
			display_lines[0] = "ID: ";
			display_lines[0] += esp_chipid;
			display_lines[1] = "FW: ";
			display_lines[1] += SOFTWARE_VERSION;
			display_lines[2] = F("Measurements: ");
			display_lines[2] += String(count_sends);
			break;
		case 9:
		    display_header = F("Tera Next PM");
			display_lines[0] = std::move(tmpl(F("PM1: {v} µg/m³"), check_display_value(pm01_value, -1, 1, 6)));
			display_lines[1] = std::move(tmpl(F("PM2.5: {v} µg/m³"), check_display_value(pm25_value, -1, 1, 6)));
			display_lines[2] = std::move(tmpl(F("PM10: {v} µg/m³"), check_display_value(pm10_value, -1, 1, 6)));
			break;
		case 10:
		    display_header = F("Tera Next PM");
			display_lines[0] = current_state_npm;
			display_lines[1] = F("T_NPM / RH_NPM");
			display_lines[2] = current_th_npm;
			break;
		case 11:
		    display_header = F("Piera IPS-7100");
			display_lines[0] = std::move(tmpl(F("PM1: {v} µg/m³"), check_display_value(pm01_value, -1, 1, 6)));
			display_lines[1] = std::move(tmpl(F("PM2.5: {v} µg/m³"), check_display_value(pm25_value, -1, 1, 6)));
			display_lines[2] = std::move(tmpl(F("PM10: {v} µg/m³"), check_display_value(pm10_value, -1, 1, 6)));
			break;
		}

		if (oled_ssd1306)
		{
			oled_ssd1306->clear();
			oled_ssd1306->displayOn();
			oled_ssd1306->setTextAlignment(TEXT_ALIGN_CENTER);
			oled_ssd1306->drawString(64, 1, display_header);
			oled_ssd1306->setTextAlignment(TEXT_ALIGN_LEFT);
			oled_ssd1306->drawString(0, 16, display_lines[0]);
			oled_ssd1306->drawString(0, 28, display_lines[1]);
			oled_ssd1306->drawString(0, 40, display_lines[2]);
			oled_ssd1306->setTextAlignment(TEXT_ALIGN_CENTER);
			oled_ssd1306->drawString(64, 52, displayGenerateFooter(screen_count));
			oled_ssd1306->display();
		}
		if (oled_sh1106)
		{
			oled_sh1106->clear();
			oled_sh1106->displayOn();
			oled_sh1106->setTextAlignment(TEXT_ALIGN_CENTER);
			oled_sh1106->drawString(64, 1, display_header);
			oled_sh1106->setTextAlignment(TEXT_ALIGN_LEFT);
			oled_sh1106->drawString(0, 16, display_lines[0]);
			oled_sh1106->drawString(0, 28, display_lines[1]);
			oled_sh1106->drawString(0, 40, display_lines[2]);
			oled_sh1106->setTextAlignment(TEXT_ALIGN_CENTER);
			oled_sh1106->drawString(64, 52, displayGenerateFooter(screen_count));
			oled_sh1106->display();
		}
		if (lcd_2004)
		{
			display_header = std::move(String((next_display_count % screen_count) + 1) + '/' + String(screen_count) + ' ' + display_header);
			display_lines[0].replace(" µg/m³", emptyString);
			display_lines[0].replace("°", String(char(223)));
			display_lines[1].replace(" µg/m³", emptyString);
			display_lines[2].replace(" µg/m³", emptyString);
			lcd_2004->clear();
			lcd_2004->setCursor(0, 0);
			lcd_2004->print(display_header);
			lcd_2004->setCursor(0, 1);
			lcd_2004->print(display_lines[0]);
			lcd_2004->setCursor(0, 2);
			lcd_2004->print(display_lines[1]);
			lcd_2004->setCursor(0, 3);
			lcd_2004->print(display_lines[2]);
		}
	}

	// ----5----0----5----0
	// PM10/2.5: 1999/999
	// T/H: -10.0°C/100.0%
	// T/P: -10.0°C/1000hPa

	if (lcd_1602)
	{
		switch (screens[next_display_count % screen_count])
		{
		case 1:
			display_lines[0] = "PM2.5: ";
			display_lines[0] += check_display_value(pm25_value, -1, 1, 6);
			display_lines[1] = "PM10:  ";
			display_lines[1] += check_display_value(pm10_value, -1, 1, 6);
			break;
		case 2:
			display_lines[0] = "PM1.0: ";
			display_lines[0] += check_display_value(pm01_value, -1, 1, 4);
			display_lines[1] = "PM4: ";
			display_lines[1] += check_display_value(pm04_value, -1, 1, 4);
			break;
		case 3:
			display_lines[0] = std::move(tmpl(F("T: {v} °C"), check_display_value(t_value, -128, 1, 6)));
			display_lines[1] = std::move(tmpl(F("H: {v} %"), check_display_value(h_value, -1, 1, 6)));
			break;
		case 4:
			display_lines[0] = std::move(tmpl(F("T/H: {v}"), check_display_value(last_value_SCD30_T, -128, 1, 5) + " / " + check_display_value(last_value_SCD30_H, -1, 0, 3)));
			display_lines[1] = std::move(tmpl(F("CO2: {v} ppm"), check_display_value(last_value_SCD30_CO2, 0, 0, 6)));
			break;
		case 5:
			display_lines[0] = "Lat: ";
			display_lines[0] += check_display_value(lat_value, -200.0, 6, 11);
			display_lines[1] = "Lon: ";
			display_lines[1] += check_display_value(lon_value, -200.0, 6, 11);
			break;
		case 6:
			display_lines[0] = std::move(tmpl(F("LAeq: {v} db(A)"), check_display_value(la_eq_value, -1, 1, 6)));
			display_lines[1] = std::move(tmpl(F("LA_max: {v} db(A)"), check_display_value(la_max_value, -1, 1, 6)));
			break;
		case 7:
			display_lines[0] = WiFi.localIP().toString();
			display_lines[1] = WiFi.SSID();
			break;
		case 8:
			display_lines[0] = "ID: ";
			display_lines[0] += esp_chipid;
			display_lines[1] = "FW: ";
			display_lines[1] += SOFTWARE_VERSION;
			break;
		case 9:
			display_lines[0] = "PM1: ";
			display_lines[0] += check_display_value(pm01_value, -1, 1, 6);
			display_lines[1] = "PM2.5: ";
			display_lines[1] += check_display_value(pm25_value, -1, 1, 6);
			break;
		case 10:
			display_lines[0] = current_state_npm;
			display_lines[1] = current_th_npm;
			break;
		case 11:
			display_lines[0] = "PM1: ";
			display_lines[0] += check_display_value(pm01_value, -1, 1, 6);
			display_lines[1] = "PM2.5: ";
			display_lines[1] += check_display_value(pm25_value, -1, 1, 6);
			break;
		}

		display_lines[0].replace("°", String(char(223)));

		lcd_1602->clear();
		lcd_1602->setCursor(0, 0);
		lcd_1602->print(display_lines[0]);
		lcd_1602->setCursor(0, 1);
		lcd_1602->print(display_lines[1]);
	}
	yield();
	next_display_count++;
}

/*****************************************************************
 * Init LCD/OLED display                                         *
 *****************************************************************/
static void init_display()
{
	if (cfg::has_display)
	{
		oled_ssd1306 = new SSD1306(0x3c, I2C_PIN_SDA, I2C_PIN_SCL);
		oled_ssd1306->init();
		if (cfg::has_flipped_display)
		{
			oled_ssd1306->flipScreenVertically();
		}
	}
	if (cfg::has_sh1106)
	{
		oled_sh1106 = new SH1106(0x3c, I2C_PIN_SDA, I2C_PIN_SCL);
		oled_sh1106->init();
		if (cfg::has_flipped_display)
		{
			oled_sh1106->flipScreenVertically();
		}
	}
	if (cfg::has_lcd1602) {
		lcd_1602 = new LiquidCrystal_I2C(
			lcd_1602_default_i2c_address, 
			lcd_1602_columns, 
			lcd_1602_rows);
	} else if (cfg::has_lcd1602_27) {
		lcd_1602 = new LiquidCrystal_I2C(
			lcd_1602_alternate_i2c_address, 
			lcd_1602_columns, 
			lcd_1602_rows);
	}
	if (lcd_1602)
	{
		lcd_1602->init();
		lcd_1602->backlight();
	}
	if (cfg::has_lcd2004) {
		lcd_2004 = new LiquidCrystal_I2C(
			lcd_2004_default_i2c_address, 
			lcd_2004_columns, 
			lcd_2004_rows);
	} else if (cfg::has_lcd2004_27) {
		lcd_2004 = new LiquidCrystal_I2C(
			lcd_2004_alternate_i2c_address, 
			lcd_2004_columns, 
			lcd_2004_rows);
	}
	if (lcd_2004)
	{
		lcd_2004->init();
		lcd_2004->backlight();
	}

	// reset back to 100k as the OLEDDisplay initialization is
	// modifying the I2C speed to 400k, which overwhelms some of the
	// sensors.
	Wire.setClock(100000);
	Wire.setClockStretchLimit(150000);
}

/*****************************************************************
 * Init BMP280/BME280                                            *
 *****************************************************************/
static bool initBMX280(char addr)
{
	debug_out(String(F("Trying BMx280 sensor on ")) + String(addr, HEX), DEBUG_MIN_INFO);

	if (bmx280.begin(addr))
	{
		// Issue #5: sensorID() kann direkt nach begin() Garbage liefern,
		// wenn der I²C-Bus noch nicht stabil ist. 50ms Wait + Retry-Loop
		// (3 Versuche, Mehrheit gewinnt) macht die BMP/BME-Erkennung robust.
		delay(50);
		uint8_t ids[3];
		for (uint8_t i = 0; i < 3; ++i)
		{
			ids[i] = bmx280.sensorID();
			delay(5);
		}
		uint8_t majority_id = (ids[0] == ids[1] || ids[0] == ids[2]) ? ids[0] : ids[1];
		if (ids[0] != ids[1] || ids[1] != ids[2])
		{
			debug_outln_info(F("BMx280 sensorID unstable, using majority: 0x"), String(majority_id, HEX));
		}

		debug_outln_info(FPSTR(DBG_TXT_FOUND));
		bmx280.setSampling(
			BMX280::MODE_FORCED,
			BMX280::SAMPLING_X1,
			BMX280::SAMPLING_X1,
			BMX280::SAMPLING_X1);
		return true;
	}
	else
	{
		debug_outln_info(FPSTR(DBG_TXT_NOT_FOUND));
		return false;
	}
}

/*****************************************************************
   Init SPS30 PM Sensor
 *****************************************************************/
static void initSPS30()
{
	char serial[SPS_MAX_SERIAL_LEN];
	debug_out(F("Trying SPS30 sensor on 0x69H "), DEBUG_MIN_INFO);
	sps30_reset();
	delay(200);
	if (sps30_get_serial(serial) != 0)
	{
		debug_outln_info(FPSTR(DBG_TXT_NOT_FOUND));

		debug_outln_info(F("Check SPS30 wiring"));
		sps30_init_failed = true;
		return;
	}
	debug_outln_info(F(" ... found, Serial-No.: "), String(serial));
	if (sps30_set_fan_auto_cleaning_interval(SPS30_AUTO_CLEANING_INTERVAL) != 0)
	{
		debug_outln_error(F("setting of Auto Cleaning Intervall SPS30 failed!"));
		sps30_init_failed = true;
		return;
	}
	delay(100);
	if (sps30_start_measurement() != 0)
	{
		debug_outln_error(F("SPS30 error starting measurement"));
		sps30_init_failed = true;
		return;
	}
}

/*****************************************************************
   Init DNMS - Digital Noise Measurement Sensor
 *****************************************************************/
static void initDNMS()
{
	char dnms_version[DNMS_MAX_VERSION_LEN + 1];

	debug_out(F("Trying DNMS sensor on 0x55H "), DEBUG_MIN_INFO);
	dnms_reset();
	delay(1000);
	if (dnms_read_version(dnms_version) != 0)
	{
		debug_outln_info(FPSTR(DBG_TXT_NOT_FOUND));
		debug_outln_error(F("Check DNMS wiring"));
		dnms_init_failed = true;
	}
	else
	{
		dnms_version[DNMS_MAX_VERSION_LEN] = 0;
		debug_outln_info(FPSTR(DBG_TXT_FOUND), String(": ") + String(dnms_version));
	}
}

/*****************************************************************
   Functions
 *****************************************************************/

static void powerOnTestSensors()
{
	if (cfg::ppd_read)
	{
		pinMode(PPD_PIN_PM1, INPUT_PULLUP); // Listen at the designated PIN
		pinMode(PPD_PIN_PM2, INPUT_PULLUP); // Listen at the designated PIN
		debug_outln_info(F("Read PPD..."));
	}

	if (cfg::sds_read)
	{
		debug_outln_info(F("Read SDS...: "), SDS_version_date());
		SDS_cmd(PmSensorCmd::ContinuousMode);
		delay(100);
		debug_outln_info(F("Stopping SDS011..."));
		is_SDS_running = SDS_cmd(PmSensorCmd::Stop);
	}

	if (cfg::pms_read)
	{
		debug_outln_info(F("Read PMS(1,3,5,6,7)003..."));
		PMS_cmd(PmSensorCmd::Start);
		delay(100);
		PMS_cmd(PmSensorCmd::ContinuousMode);
		delay(100);
		debug_outln_info(F("Stopping PMS..."));
		is_PMS_running = PMS_cmd(PmSensorCmd::Stop);
	}

	if (cfg::hpm_read)
	{
		debug_outln_info(F("Read HPM..."));
		HPM_cmd(PmSensorCmd::Start);
		delay(100);
		HPM_cmd(PmSensorCmd::ContinuousMode);
		delay(100);
		debug_outln_info(F("Stopping HPM..."));
		is_HPM_running = HPM_cmd(PmSensorCmd::Stop);
	}

	if (cfg::npm_read)
	{
		uint8_t test_state;
		delay(15000); //wait a bit to be sure Next PM is ready to receive instructions.
		test_state = NPM_get_state();
		if (test_state == 0x00)
		{
			debug_outln_info(F("NPM already started..."));
			is_NPM_running = true;
		}
		else if (test_state == 0x01)
		{
			debug_outln_info(F("Force start NPM...")); // to read the firmware version
			is_NPM_running = NPM_start_stop();
		}
		else
		{
			if (bitRead(test_state, 1) == 1)
			{
				debug_outln_info(F("Degraded state"));
			}
			else
			{
				debug_outln_info(F("Default state"));
			}
			if (bitRead(test_state, 2) == 1)
			{
				debug_outln_info(F("Not ready"));
			}
			if (bitRead(test_state, 3) == 1)
			{
				debug_outln_info(F("Heat error"));
			}
			if (bitRead(test_state, 4) == 1)
			{
				debug_outln_info(F("T/RH error"));
			}
			if (bitRead(test_state, 5) == 1)
			{
				debug_outln_info(F("Fan error"));

				// if (bitRead(test_state, 0) == 1){
				// 	debug_outln_info(F("Force start NPM..."));
				// 	is_NPM_running = NPM_start_stop();
				// 	delay(5000);
				// }
				// NPM_fan_speed();
				// delay(5000);
			}
			if (bitRead(test_state, 6) == 1)
			{
				debug_outln_info(F("Memory error"));
			}
			if (bitRead(test_state, 7) == 1)
			{
				debug_outln_info(F("Laser error"));
			}

			if (bitRead(test_state, 0) == 0)
			{
				debug_outln_info(F("NPM already started..."));
			}
			else
			{
				debug_outln_info(F("Force start NPM..."));
				is_NPM_running = NPM_start_stop();
			}
		}

		delay(15000); //prevent any buffer overload on ESP82666
		NPM_version_date();
		delay(3000); //prevent any buffer overload on ESP82666
		NPM_temp_humi();
		delay(2000); 

		if(!cfg::npm_fulltime) {
			is_NPM_running = NPM_start_stop();
			delay(2000); //prevent any buffer overload on ESP82666
		}else{
			is_NPM_running = true;
		}
	}

	if (cfg::ips_read)
	{
		IPS_cmd(PmSensorCmd3::Factory); //set to Factory
		delay(1000);
		IPS_version_date();
		delay(1000);
		IPS_cmd(PmSensorCmd3::Smoke); // no smoke detection
		delay(1000);
		IPS_cmd(PmSensorCmd3::Interval); //Set interval to 0 = manual mode
		delay(1000);
		IPS_cmd(PmSensorCmd3::Stop); 
		delay(1000);
		is_IPS_running = false;
	}

	if (cfg::sps30_read)
	{
		debug_outln_info(F("Read SPS30..."));
		initSPS30();
	}

	if (cfg::dht_read)
	{
		dht.begin(); // Start DHT
		debug_outln_info(F("Read DHT..."));
	}

	if (cfg::htu21d_read)
	{
		debug_outln_info(F("Read HTU21D..."));
		// begin() might return false when using Si7021
		// so validate reading via Humidity (will return 0.0 when failed)
		if (!htu21d.begin() && htu21d.readHumidity() < 1.0f)
		{
			debug_outln_error(F("Check HTU21D wiring"));
			htu21d_init_failed = true;
		}
	}

	if (cfg::aht20_read)
	{
		debug_outln_info(F("Read AHT20..."));
		if (!aht20.begin())
		{
			debug_outln_error(F("Check AHT20 wiring"));
			aht20_init_failed = true;
		}
	}

	if (cfg::bmp_read)
	{
		debug_outln_info(F("Read BMP..."));
		if (!bmp.begin())
		{
			debug_outln_error(F("No valid BMP085 sensor, check wiring!"));
			bmp_init_failed = true;
		}
	}

	if (cfg::bmx280_read)
	{
		debug_outln_info(F("Read BMx280..."));
		if (!initBMX280(bmx280_default_i2c_address) && !initBMX280(bmx280_alternate_i2c_address)) {
			debug_outln_error(F("Check BMx280 wiring"));
			bmx280_init_failed = true;
		}
	}

	if (cfg::sht3x_read)
	{
		debug_outln_info(F("Read SHT3x..."));
		if (!sht3x.begin())
		{
			debug_outln_error(F("Check SHT3x wiring"));
			sht3x_init_failed = true;
		}
	}

	if (cfg::scd30_read)
	{
		debug_outln_info(F("Read SCD30..."));
		if (!scd30.begin())
		{
			debug_outln_error(F("Check SCD30 wiring"));
			scd30_init_failed = true;
		}
/*		else
		{
			scd30.setMeasurementInterval(30);
		} */
	}

	if (cfg::ds18b20_read)
	{
		oneWire.begin(ONEWIRE_PIN);
		ds18b20.begin(); // Start DS18B20
		debug_outln_info(F("Read DS18B20..."));
	}

	if (cfg::dnms_read)
	{
		debug_outln_info(F("Read DNMS..."));
		initDNMS();
	}
}

static void logEnabledAPIs()
{
	debug_outln_info(F("Send to :"));
	if (cfg::send2dusti)
	{
		debug_outln_info(F("sensor.community"));
	}

	if (cfg::send2fsapp)
	{
		debug_outln_info(F("Feinstaub-App"));
	}

	if (cfg::send2madavi)
	{
		debug_outln_info(F("Madavi.de"));
	}

	if (cfg::send2csv)
	{
		debug_outln_info(F("Serial as CSV"));
	}

	if (cfg::send2custom)
	{
		debug_outln_info(F("custom API"));
	}

	if (cfg::send2aircms)
	{
		debug_outln_info(F("aircms API"));
	}

	if (cfg::send2influx)
	{
		debug_outln_info(F("custom influx DB"));
	}
	debug_outln_info(FPSTR(DBG_TXT_SEP));
	if (cfg::auto_update)
	{
		debug_outln_info(F("Auto-Update active..."));
	}
}

static void logEnabledDisplays()
{
	if (cfg::has_display || cfg::has_sh1106)
	{
		debug_outln_info(F("Show on OLED..."));
	}
	if (lcd_1602)
	{
		debug_outln_info(F("Show on LCD 1602 ..."));
	}
	if (lcd_2004)
	{
		debug_outln_info(F("Show on LCD 2004 ..."));
	}
}

static void setupNetworkTime()
{
	// server name ptrs must be persisted after the call to configTime because internally
	// the pointers are stored see implementation of lwip sntp_setservername()
	static char ntpServer1[18], ntpServer2[18];
#if defined(ESP8266)
	settimeofday_cb([]()
					{
						if (!sntp_time_set)
						{
							time_t now = time(nullptr);
							debug_outln_info(F("SNTP synced: "), ctime(&now));
							twoStageOTAUpdate();
							last_update_attempt = millis();
						}
						sntp_time_set++;
					});
#endif
	strcpy_P(ntpServer1, NTP_SERVER_1);
	strcpy_P(ntpServer2, NTP_SERVER_2);
	configTime(0, 0, ntpServer1, ntpServer2);
}

/*****************************************************************
 * sendWundergroundUpdate (hibbes-Patch, Issue #16)               *
 * Direkter Push an Wunderground PWS-Upload-API ersetzt den       *
 * fein2wunder-PHP-Wrapper. Liest aktive T/H/P-Sensoren + SDS-PM, *
 * konvertiert Einheiten in WU-Format und macht GET an die        *
 * updateweatherstation.php-URL. Höhen-Korrektur via              *
 * cfg::height_above_sealevel (gleiche Logik wie fein2wunder).    *
 *****************************************************************/
static unsigned long sendWundergroundUpdate()
{
	if (!cfg::send2wunderground || !*cfg::wu_station_id || !*cfg::wu_password)
	{
		return 0;
	}

	// Picke T+H aus aktiven Sensoren (Priorität: AHT20 > DHT > HTU21D > BME280)
	float t_c = -128.0f;
	float h_pct = -1.0f;
	if (cfg::aht20_read && !aht20_init_failed && last_value_AHT20_T > -100.0f)
	{
		t_c = last_value_AHT20_T;
		h_pct = last_value_AHT20_H;
	}
	else if (cfg::dht_read && last_value_DHT_T > -100.0f)
	{
		t_c = last_value_DHT_T;
		h_pct = last_value_DHT_H;
	}
	else if (cfg::htu21d_read && !htu21d_init_failed && last_value_HTU21D_T > -100.0f)
	{
		t_c = last_value_HTU21D_T;
		h_pct = last_value_HTU21D_H;
	}
	else if (cfg::bmx280_read && !bmx280_init_failed && bmx280.sensorID() == BME280_SENSOR_ID && last_value_BMX280_T > -100.0f)
	{
		t_c = last_value_BMX280_T;
		h_pct = last_value_BME280_H;
	}

	// Picke Druck aus BMx280 oder BMP180
	float p_pa = -1.0f;
	if (cfg::bmx280_read && !bmx280_init_failed && last_value_BMX280_P > 0.0f)
	{
		p_pa = last_value_BMX280_P;
	}
	else if (cfg::bmp_read && last_value_BMP_P > 0.0f)
	{
		p_pa = last_value_BMP_P;
	}

	if (t_c <= -100.0f || h_pct < 0.0f || p_pa <= 0.0f)
	{
		debug_outln_info(F("WU: skip - T/H/P unvollständig"));
		return 0;
	}

	// Konvertiere
	float tempf = t_c * 1.8f + 32.0f;
	float dew_c = dew_point(t_c, h_pct);
	float dewptf = dew_c * 1.8f + 32.0f;
	float p_hpa = p_pa / 100.0f;
	float p_msl = pressure_at_sealevel(t_c, p_hpa);
	float baroinch = p_msl / 33.8638866667f;

	// Build URL
	String url(FPSTR(URL_WUNDERGROUND_BASE));
	url.reserve(384);
	url += F("?ID=");
	url += cfg::wu_station_id;
	url += F("&PASSWORD=");
	url += cfg::wu_password;
	url += F("&dateutc=now");
	url += F("&tempf=");
	url += String(tempf, 2);
	url += F("&dewptf=");
	url += String(dewptf, 2);
	url += F("&humidity=");
	url += String(h_pct, 1);
	url += F("&baromin=");
	url += String(baroinch, 4);
	if (cfg::sds_read && last_value_SDS_P1 >= 0.0f && last_value_SDS_P2 >= 0.0f)
	{
		url += F("&AqPM2.5=");
		url += String(last_value_SDS_P2, 2);
		url += F("&AqPM10=");
		url += String(last_value_SDS_P1, 2);
	}
	url += F("&softwaretype=airrohr-");
	url += SOFTWARE_VERSION;
	url += F("&action=updateraw");

	// GET-Push (WU akzeptiert nur GET, sendData() macht POST -> dedicated path)
	cycle_send_attempts++;
	unsigned long start_send = millis();

	// Issue #11: cycle-Budget check.
	if (cycle_send_deadline && millis() > cycle_send_deadline)
	{
		debug_outln_info(F("WU: cycle deadline exceeded, skipping"));
		return 0;
	}

	std::unique_ptr<WiFiClient> client(getNewLoggerWiFiClient(LoggerWunderground));
	HTTPClient http;
	http.setTimeout(15 * 1000);
	http.setUserAgent(SOFTWARE_VERSION);
	// Issue #10: siehe sendData()-Kommentar — kein Schaden, BearSSL-Session-Cache greift.
	http.setReuse(true);

	bool send_success = false;
	debug_outln_info(F("WU: pushing to "), FPSTR(HOST_WUNDERGROUND));
	if (http.begin(*client, FPSTR(HOST_WUNDERGROUND), PORT_WUNDERGROUND, url, true))
	{
		int result = http.GET();
		if (result >= HTTP_CODE_OK && result <= HTTP_CODE_ALREADY_REPORTED)
		{
			debug_outln_info(F("WU response: "), http.getString());
			send_success = true;
			cycle_send_successes++;
		}
		else
		{
			debug_outln_info(F("WU GET failed HTTP "), String(result));
		}
		http.end();
	}
	else
	{
		debug_outln_info(F("WU connect failed"));
	}

	if (!send_success)
	{
		loggerConfigs[LoggerWunderground].errors++;
	}

	return millis() - start_send;
}

static unsigned long sendDataToOptionalApis(const String &data)
{
	unsigned long sum_send_time = 0;

	if (cfg::send2madavi)
	{
		debug_outln_info(FPSTR(DBG_TXT_SENDING_TO), F("madavi.de: "));
		sum_send_time += sendData(LoggerMadavi, data, 0, HOST_MADAVI, URL_MADAVI);
	}

	if (cfg::send2sensemap && (cfg::senseboxid[0] != '\0'))
	{
		debug_outln_info(FPSTR(DBG_TXT_SENDING_TO), F("opensensemap: "));
		String sensemap_path(tmpl(FPSTR(URL_SENSEMAP), cfg::senseboxid));
		sum_send_time += sendData(LoggerSensemap, data, 0, HOST_SENSEMAP, sensemap_path.c_str());
	}

	if (cfg::send2fsapp)
	{
		debug_outln_info(FPSTR(DBG_TXT_SENDING_TO), F("Server FS App: "));
		sum_send_time += sendData(LoggerFSapp, data, 0, HOST_FSAPP, URL_FSAPP);
	}

	if (cfg::send2aircms)
	{
		debug_outln_info(FPSTR(DBG_TXT_SENDING_TO), F("aircms.online: "));
		unsigned long ts = millis() / 1000;
		String token = WiFi.macAddress();
		String aircms_data("L=");
		aircms_data += esp_chipid;
		aircms_data += "&t=";
		aircms_data += String(ts, DEC);
		aircms_data += F("&airrohr=");
		aircms_data += data;
		String aircms_url(FPSTR(URL_AIRCMS));
		aircms_url += hmac1(sha1Hex(token), aircms_data + token);

		sum_send_time += sendData(Loggeraircms, aircms_data, 0, HOST_AIRCMS, aircms_url.c_str());
	}

	if (cfg::send2influx)
	{
		debug_outln_info(FPSTR(DBG_TXT_SENDING_TO), F("custom influx db: "));
		RESERVE_STRING(data_4_influxdb, LARGE_STR);
		create_influxdb_string_from_data(data_4_influxdb, data);
		sum_send_time += sendData(LoggerInflux, data_4_influxdb, 0, cfg::host_influx, cfg::url_influx);
	}

	if (cfg::send2custom)
	{
		String data_to_send = data;
		data_to_send.remove(0, 1);
		String data_4_custom(F("{\"esp8266id\": \""));
		data_4_custom += esp_chipid;
		data_4_custom += "\", ";
		data_4_custom += data_to_send;
		debug_outln_info(FPSTR(DBG_TXT_SENDING_TO), F("custom api: "));
		sum_send_time += sendData(LoggerCustom, data_4_custom, 0, cfg::host_custom, cfg::url_custom);
	}

	if (cfg::send2csv)
	{
		debug_outln_info(F("## Sending as csv: "));
		send_csv(data);
	}

	// hibbes-Patch (Issue #16): direkter Wunderground-PWS-Push
	if (cfg::send2wunderground)
	{
		debug_outln_info(FPSTR(DBG_TXT_SENDING_TO), F("wunderground.com: "));
		sum_send_time += sendWundergroundUpdate();
	}

	return sum_send_time;
}

/*****************************************************************
 * The Setup                                                     *
 *****************************************************************/

void setup(void)
{
	Debug.begin(9600); // Output to Serial at 9600 baud

#if defined(ESP8266)
	esp_chipid = std::move(String(ESP.getChipId()));
	esp_mac_id = std::move(String(WiFi.macAddress().c_str()));
	esp_mac_id.replace(":", "");
	esp_mac_id.toLowerCase();
#endif
#if defined(ESP32)
	uint64_t chipid_num;
	chipid_num = ESP.getEfuseMac();
	esp_chipid = String((uint16_t)(chipid_num >> 32), HEX);
	esp_chipid += String((uint32_t)chipid_num, HEX);
#endif
	cfg::initNonTrivials(esp_chipid.c_str());
	WiFi.persistent(false);

	debug_outln_info(F("airRohr: " SOFTWARE_VERSION_STR "/"), String(CURRENT_LANG));

#if defined(ESP8266)
	if ((airrohr_selftest_failed = !ESP.checkFlashConfig() /* after 2.7.0 update: || !ESP.checkFlashCRC() */))
	{
		debug_outln_error(F("ERROR: SELF TEST FAILED!"));
		SOFTWARE_VERSION += F("-STF");
	}
#endif

	init_config(); 

	Wire.begin(I2C_PIN_SDA, I2C_PIN_SCL);

if (cfg::npm_read)
	{
#if defined(ESP8266)
		serialNPM.begin(115200, SWSERIAL_8E1, PM_SERIAL_RX, PM_SERIAL_TX);
		serialNPM.enableIntTx(false);
#endif
#if defined(ESP32)
		serialNPM.begin(115200, SERIAL_8E1, PM_SERIAL_RX, PM_SERIAL_TX);
#endif
		Debug.println("Read Next PM... serialNPM 115200 8E1");
		serialNPM.setTimeout(400);
	}
else if (cfg::ips_read)
	{
//#define SERIAL_BUFFER_SIZE 256
#if defined(ESP8266)
		serialIPS.begin(115200, SWSERIAL_8N1, PM_SERIAL_RX, PM_SERIAL_TX);
		serialIPS.enableIntTx(false);
#endif
#if defined(ESP32)
		serialIPS.begin(115200, SERIAL_8N1, PM_SERIAL_RX, PM_SERIAL_TX);
#endif
		Debug.println("Read IPS... serialIPS 115200 8N1"); //will be set to 9600 8N1 afterwards
		serialIPS.setTimeout(900); //Which timeout?
	}
	else
	{
#if defined(ESP8266)
		serialSDS.begin(9600, SWSERIAL_8N1, PM_SERIAL_RX, PM_SERIAL_TX);
		serialSDS.enableIntTx(true);
#endif

#if defined(ESP32)
		serialSDS.begin(9600, SERIAL_8N1, PM_SERIAL_RX, PM_SERIAL_TX);
#endif
		Debug.println("No Next PM... serialSDS 9600 8N1");
		serialSDS.setTimeout((4 * 12 * 1000) / 9600);
	}

#if defined(WIFI_LoRa_32_V2)
	// reset the OLED display, e.g. of the heltec_wifi_lora_32 board
	pinMode(RST_OLED, OUTPUT);
	digitalWrite(RST_OLED, LOW);
	delay(50);
	digitalWrite(RST_OLED, HIGH);
#endif

	init_display();
	setupNetworkTime();
	connectWifi();
	setup_webserver();
	createLoggerConfigs();
	debug_outln_info(F("\nChipId: "), esp_chipid);
	debug_outln_info(F("\nMAC Id: "), esp_mac_id);

	if (cfg::gps_read)
	{
#if defined(ESP8266)
		serialGPS = new SoftwareSerial;
		serialGPS->begin(9600, SWSERIAL_8N1, GPS_SERIAL_RX, GPS_SERIAL_TX, false, 128);
#endif
#if defined(ESP32)
		serialGPS->begin(9600, SERIAL_8N1, GPS_SERIAL_RX, GPS_SERIAL_TX);
#endif
		debug_outln_info(F("Read GPS..."));
		disable_unneeded_nmea();
	}

	powerOnTestSensors();
	logEnabledAPIs();
	logEnabledDisplays();

	delay(50);

	starttime = millis(); // store the start time
	last_update_attempt = time_point_device_start_ms = starttime;
	if (cfg::npm_read)
	{
		last_display_millis = starttime_NPM = starttime;
	}
	else
	{
		last_display_millis = starttime_SDS = starttime;
	}
}

/*****************************************************************
 * And action                                                    *
 *****************************************************************/
void loop(void)
{
	unsigned long sleep = SLEEPTIME_MS;
	String result_PPD, result_SDS, result_PMS, result_HPM, result_NPM, result_IPS;
	String result_GPS, result_DNMS;

	unsigned sum_send_time = 0;

	act_micro = micros();
	act_milli = millis();
	send_now = msSince(starttime) > cfg::sending_intervall_ms;

	if (send_now)
	{
		sleep = 0;
	}

	// Wait at least 30s for each NTP server to sync
	if (!sntp_time_set && send_now &&
		msSince(time_point_device_start_ms) < 1000 * 2 * 30 + 5000)
	{
		debug_outln_info(F("NTP sync not finished yet, skipping send"));
		send_now = false;
		starttime = act_milli;
	}

	sample_count++;
	if (last_micro != 0)
	{
		unsigned long diff_micro = act_micro - last_micro;
		UPDATE_MIN_MAX(min_micro, max_micro, diff_micro);
	}
	last_micro = act_micro;

	if (cfg::sps30_read && (!sps30_init_failed))
	{
		if ((msSince(starttime) - SPS30_read_timer) > SPS30_WAITING_AFTER_LAST_READ)
		{
			struct sps30_measurement sps30_values;
			int16_t ret_SPS30;

			SPS30_read_timer = msSince(starttime);

			ret_SPS30 = sps30_read_measurement(&sps30_values);
			++SPS30_read_counter;
			if (ret_SPS30 < 0)
			{
				debug_outln_info(F("SPS30 error reading measurement"));
				SPS30_read_error_counter++;
			}
			else
			{
				if (SPS_IS_ERR_STATE(ret_SPS30))
				{
					debug_outln_info(F("SPS30 measurements may not be accurate"));
					SPS30_read_error_counter++;
				}
				value_SPS30_P0 += sps30_values.mc_1p0;
				value_SPS30_P2 += sps30_values.mc_2p5;
				value_SPS30_P4 += sps30_values.mc_4p0;
				value_SPS30_P1 += sps30_values.mc_10p0;
				value_SPS30_N05 += sps30_values.nc_0p5;
				value_SPS30_N1 += sps30_values.nc_1p0;
				value_SPS30_N25 += sps30_values.nc_2p5;
				value_SPS30_N4 += sps30_values.nc_4p0;
				value_SPS30_N10 += sps30_values.nc_10p0;
				value_SPS30_TS += sps30_values.tps;
				++SPS30_measurement_count;
			}
		}
	}

	if (cfg::ppd_read)
	{
		fetchSensorPPD(result_PPD);
	}

	if (cfg::npm_read)
	{
		if ((msSince(starttime_NPM) > SAMPLETIME_NPM_MS) || send_now)
		{
			starttime_NPM = act_milli;
			fetchSensorNPM(result_NPM);
		}	
	}else if(cfg::ips_read)
	{
		if ((msSince(starttime_IPS) > SAMPLETIME_IPS_MS) || send_now)
		{
			starttime_IPS = act_milli;
			fetchSensorIPS(result_IPS);
		}	
	}
	else
	{
		if ((msSince(starttime_SDS) > SAMPLETIME_SDS_MS) || send_now)
		{
			starttime_SDS = act_milli;
			if (cfg::sds_read)
			{
				fetchSensorSDS(result_SDS);
			}

			if (cfg::pms_read)
			{
				fetchSensorPMS(result_PMS);
			}

			if (cfg::hpm_read)
			{
				fetchSensorHPM(result_HPM);
			}
		}
	}

	if (cfg::gps_read && !gps_init_failed)
	{
		// process serial GPS data..
		while (serialGPS->available() > 0)
		{
			gps.encode(serialGPS->read());
		}

		if ((msSince(starttime_GPS) > SAMPLETIME_GPS_MS) || send_now)
		{
			// getting GPS coordinates
			fetchSensorGPS(result_GPS);
			starttime_GPS = act_milli;
		}
	}

	if ((msSince(last_display_millis) > DISPLAY_UPDATE_INTERVAL_MS) &&
		(cfg::has_display || cfg::has_sh1106 || lcd_1602 || lcd_2004))
	{
		display_values();
		last_display_millis = act_milli;
	}

	server.handleClient();
	yield();

	if (send_now)
	{
		// Issue #11: cycle deadline neu setzen — sobald überschritten,
		// werden weitere sendData()/sendWundergroundUpdate() früh-returned.
		cycle_send_deadline = millis() + CYCLE_SEND_BUDGET_MS;
		last_signal_strength = WiFi.RSSI();
		RESERVE_STRING(data, LARGE_STR);
		data = FPSTR(data_first_part);
		RESERVE_STRING(result, MED_STR);

		if (cfg::ppd_read)
		{
			data += result_PPD;
			sum_send_time += sendSensorCommunity(result_PPD, PPD_API_PIN, FPSTR(SENSORS_PPD42NS), "PPD_");
		}
		if (cfg::sds_read)
		{
			data += result_SDS;
			sum_send_time += sendSensorCommunity(result_SDS, SDS_API_PIN, FPSTR(SENSORS_SDS011), "SDS_");
		}
		if (cfg::pms_read)
		{
			data += result_PMS;
			sum_send_time += sendSensorCommunity(result_PMS, PMS_API_PIN, FPSTR(SENSORS_PMSx003), "PMS_");
		}
		if (cfg::hpm_read)
		{
			data += result_HPM;
			sum_send_time += sendSensorCommunity(result_HPM, HPM_API_PIN, FPSTR(SENSORS_HPM), "HPM_");
		}
		if (cfg::npm_read)
		{
			data += result_NPM;
			sum_send_time += sendSensorCommunity(result_NPM, NPM_API_PIN, FPSTR(SENSORS_NPM), "NPM_");
		}
		if (cfg::ips_read)
		{
			data += result_IPS;
			sum_send_time += sendSensorCommunity(result_IPS, IPS_API_PIN, FPSTR(SENSORS_IPS), "IPS_");
		}
		if (cfg::sps30_read && (!sps30_init_failed))
		{
			fetchSensorSPS30(result);
			data += result;
			sum_send_time += sendSensorCommunity(result, SPS30_API_PIN, FPSTR(SENSORS_SPS30), "SPS30_");
			result = emptyString;
		}
		if (cfg::dht_read)
		{
			// getting temperature and humidity (optional)
			fetchSensorDHT(result);
			data += result;
			sum_send_time += sendSensorCommunity(result, DHT_API_PIN, FPSTR(SENSORS_DHT22), "DHT_");
			result = emptyString;
		}
		if (cfg::htu21d_read && (!htu21d_init_failed))
		{
			// getting temperature and humidity (optional)
			fetchSensorHTU21D(result);
			data += result;
			sum_send_time += sendSensorCommunity(result, HTU21D_API_PIN, FPSTR(SENSORS_HTU21D), "HTU21D_");
			result = emptyString;
		}
		if (cfg::aht20_read && (!aht20_init_failed))
		{
			// getting temperature and humidity from AHT20
			fetchSensorAHT20(result);
			data += result;
			sum_send_time += sendSensorCommunity(result, AHT20_API_PIN, FPSTR(SENSORS_AHT20), "AHT20_");
			result = emptyString;
		}
		if (cfg::bmp_read && (!bmp_init_failed))
		{
			// getting temperature and pressure (optional)
			fetchSensorBMP(result);
			data += result;
			sum_send_time += sendSensorCommunity(result, BMP_API_PIN, FPSTR(SENSORS_BMP180), "BMP_");
			result = emptyString;
		}
		if (cfg::bmx280_read && (!bmx280_init_failed))
		{
			// getting temperature, humidity and pressure (optional)
			fetchSensorBMX280(result);
			data += result;
			if (bmx280.sensorID() == BME280_SENSOR_ID)
			{
				sum_send_time += sendSensorCommunity(result, BME280_API_PIN, FPSTR(SENSORS_BME280), "BME280_");
			}
			else
			{
				// Maskerade: prefix "BMP_" damit Strip die BMP_*-Keys auf bare temperature/pressure reduziert (BMP180-Pin-3-Format)
				sum_send_time += sendSensorCommunity(result, BMP280_API_PIN, FPSTR(SENSORS_BMP280), "BMP_");
			}
			result = emptyString;
		}
		if (cfg::sht3x_read && (!sht3x_init_failed))
		{
			// getting temperature and humidity (optional)
			fetchSensorSHT3x(result);
			data += result;
			sum_send_time += sendSensorCommunity(result, SHT3X_API_PIN, FPSTR(SENSORS_SHT3X), "SHT3X_");
			result = emptyString;
		}
		if (cfg::scd30_read && (!scd30_init_failed))
		{
			// getting temperature and humidity (optional)
			fetchSensorSCD30(result);
			data += result;
			sum_send_time += sendSensorCommunity(result, SCD30_API_PIN, FPSTR(SENSORS_SCD30), "SCD30_");
			result = emptyString;
		}
		if (cfg::ds18b20_read)
		{
			// getting temperature (optional)
			fetchSensorDS18B20(result);
			data += result;
			sum_send_time += sendSensorCommunity(result, DS18B20_API_PIN, FPSTR(SENSORS_DS18B20), "DS18B20_");
			result = emptyString;
		}
		if (cfg::dnms_read && (!dnms_init_failed))
		{
			// getting noise measurement values from dnms (optional)
			fetchSensorDNMS(result);
			data += result;
			sum_send_time += sendSensorCommunity(result, DNMS_API_PIN, FPSTR(SENSORS_DNMS), "DNMS_");
			result = emptyString;
		}
		if (cfg::gps_read)
		{
			data += result_GPS;
			sum_send_time += sendSensorCommunity(result_GPS, GPS_API_PIN, F("GPS"), "GPS_");
			result = emptyString;
		}
		add_Value2Json(data, F("samples"), String(sample_count));
		add_Value2Json(data, F("min_micro"), String(min_micro));
		add_Value2Json(data, F("max_micro"), String(max_micro));
		add_Value2Json(data, F("interval"), String(cfg::sending_intervall_ms));
		add_Value2Json(data, F("signal"), String(last_signal_strength));

		if ((unsigned)(data.lastIndexOf(',') + 1) == data.length())
		{
			data.remove(data.length() - 1);
		}
		data += "]}";
		debug_outln_verbose(F("Data to send: "), data);
		yield();

		sum_send_time += sendDataToOptionalApis(data);

		// https://en.wikipedia.org/wiki/Moving_average#Cumulative_moving_average
		sending_time = (3 * sending_time + sum_send_time) / 4;
		if (sum_send_time > 0)
		{
			debug_outln_info(F("Time for Sending (ms): "), String(sending_time));
		}

		// hibbes-Patch (Issue #4): Silent-failure-Detection
		// Wenn alle Push-Targets im Cycle scheitern (cycle_send_attempts > 0
		// && cycle_send_successes == 0), ist der WLAN-Stack vermutlich kaputt
		// trotz WL_CONNECTED. Nach SILENT_FAILURE_THRESHOLD Cycles in Folge
		// erzwingen wir einen harten Disconnect+Reconnect.
		if (cycle_send_attempts > 0 && cycle_send_successes == 0)
		{
			++consecutive_silent_failures;
			debug_outln_info(F("All pushes failed; consecutive silent failures: "),
							 String(consecutive_silent_failures));
		}
		else if (cycle_send_successes > 0)
		{
			consecutive_silent_failures = 0;
		}
		cycle_send_attempts = 0;
		cycle_send_successes = 0;

		if (consecutive_silent_failures >= SILENT_FAILURE_THRESHOLD)
		{
			debug_outln_info(F("Forcing WiFi reset due to silent failures"));
			WiFi_error_count++;
			WiFi.disconnect(true);
			delay(500);
			WiFi.begin(cfg::wlanssid, cfg::wlanpwd);
			waitForWifiToConnect(20);
			consecutive_silent_failures = 0;
		}

		// reconnect to WiFi if disconnected
		if (WiFi.status() != WL_CONNECTED)
		{
			debug_outln_info(F("Connection lost, reconnecting "));
			WiFi_error_count++;
			WiFi.reconnect();
			waitForWifiToConnect(20);
		}

		// only do a restart after finishing sending
		if (msSince(time_point_device_start_ms) > DURATION_BEFORE_FORCED_RESTART_MS)
		{
			sensor_restart();
		}

		// time for a OTA attempt?
		if (msSince(last_update_attempt) > PAUSE_BETWEEN_UPDATE_ATTEMPTS_MS)
		{
			twoStageOTAUpdate();
			last_update_attempt = act_milli;
		}

		// Resetting for next sampling
		last_data_string = std::move(data);
		lowpulseoccupancyP1 = 0;
		lowpulseoccupancyP2 = 0;
		sample_count = 0;
		last_micro = 0;
		min_micro = 1000000000;
		max_micro = 0;
		sum_send_time = 0;
		starttime = millis(); // store the start time
		count_sends++;
	}

#if defined(ESP8266)
	MDNS.update();
	if (cfg::npm_read)
	{
		serialNPM.perform_work();
	}
	else
	{
		serialSDS.perform_work();
	}

#endif

	// Sleep if all of the tasks have an event in the future. The chip can then
	// enter a lower power mode.
	if (cfg::powersave) {
		delay(sleep);
	}

	if (sample_count % 500 == 0)
	{
		//		Serial.println(ESP.getFreeHeap(),DEC);
	}
}
