/**
 * extern-Declarations für alle INTL_*-Symbole (Issue #18 Phase B).
 *
 * intl_<lang>.h-Files behalten die Definitionen; dieser Header gibt allen
 * TUs außer airrohr-firmware.ino die Symbole als extern bekannt, ohne
 * Multi-Definition-Konflikte am Linker.
 */
#pragma once

#include <pgmspace.h>

extern const char INTL_CONFIGURATION[] PROGMEM;
extern const char INTL_NO_NETWORKS[] PROGMEM;
extern const char INTL_NETWORKS_FOUND[] PROGMEM;
extern const char INTL_AB_HIER_NUR_ANDERN[] PROGMEM;
extern const char INTL_SAVE[] PROGMEM;
extern const char INTL_SENSORS[] PROGMEM;
extern const char INTL_MORE_SENSORS[] PROGMEM;
extern const char INTL_SDS011[] PROGMEM;
extern const char INTL_PMS[] PROGMEM;
extern const char INTL_HPM[] PROGMEM;
extern const char INTL_NPM[] PROGMEM;
extern const char INTL_NPM_FULLTIME[] PROGMEM;
extern const char INTL_IPS[] PROGMEM;
extern const char INTL_SPS30[] PROGMEM;
extern const char INTL_PPD42NS[] PROGMEM;
extern const char INTL_DHT22[] PROGMEM;
extern const char INTL_HTU21D[] PROGMEM;
extern const char INTL_AHT20[] PROGMEM;
extern const char INTL_BMP180[] PROGMEM;
extern const char INTL_BMX280[] PROGMEM;
extern const char INTL_SHT3X[] PROGMEM;
extern const char INTL_SCD30[] PROGMEM;
extern const char INTL_DS18B20[] PROGMEM;
extern const char INTL_DNMS[] PROGMEM;
extern const char INTL_DNMS_CORRECTION[] PROGMEM;
extern const char INTL_TEMP_CORRECTION[] PROGMEM;
extern const char INTL_HEIGHT_ABOVE_SEALEVEL[] PROGMEM;
extern const char INTL_PRESSURE_AT_SEALEVEL[] PROGMEM;
extern const char INTL_NEO6M[] PROGMEM;
extern const char INTL_BASICAUTH[] PROGMEM;
extern const char INTL_FS_WIFI_DESCRIPTION[] PROGMEM;
extern const char INTL_FS_WIFI_NAME[] PROGMEM;
extern const char INTL_MORE_SETTINGS[] PROGMEM;
extern const char INTL_AUTO_UPDATE[] PROGMEM;
extern const char INTL_USE_BETA[] PROGMEM;
extern const char INTL_DISPLAY[] PROGMEM;
extern const char INTL_SH1106[] PROGMEM;
extern const char INTL_FLIP_DISPLAY[] PROGMEM;
extern const char INTL_LCD1602_27[] PROGMEM;
extern const char INTL_LCD1602_3F[] PROGMEM;
extern const char INTL_LCD2004_27[] PROGMEM;
extern const char INTL_LCD2004_3F[] PROGMEM;
extern const char INTL_DISPLAY_WIFI_INFO[] PROGMEM;
extern const char INTL_DISPLAY_DEVICE_INFO[] PROGMEM;
extern const char INTL_STATIC_IP[] PROGMEM;
extern const char INTL_STATIC_SUBNET[] PROGMEM;
extern const char INTL_STATIC_GATEWAY[] PROGMEM;
extern const char INTL_STATIC_DNS[] PROGMEM;
extern const char INTL_DEBUG_LEVEL[] PROGMEM;
extern const char INTL_MEASUREMENT_INTERVAL[] PROGMEM;
extern const char INTL_DURATION_ROUTER_MODE[] PROGMEM;
extern const char INTL_POWERSAVE[] PROGMEM;
extern const char INTL_MORE_APIS[] PROGMEM;
extern const char INTL_SEND_TO_OWN_API[] PROGMEM;
extern const char INTL_SERVER[] PROGMEM;
extern const char INTL_PATH[] PROGMEM;
extern const char INTL_PORT[] PROGMEM;
extern const char INTL_USER[] PROGMEM;
extern const char INTL_PASSWORD[] PROGMEM;
extern const char INTL_MEASUREMENT[] PROGMEM;
extern const char INTL_SEND_TO[] PROGMEM;
extern const char INTL_READ_FROM[] PROGMEM;
extern const char INTL_SENSOR_IS_REBOOTING[] PROGMEM;
extern const char INTL_RESTART_DEVICE[] PROGMEM;
extern const char INTL_DELETE_CONFIG[] PROGMEM;
extern const char INTL_RESTART_SENSOR[] PROGMEM;
extern const char INTL_CURRENT_DATA[] PROGMEM;
extern const char INTL_DEVICE_STATUS[] PROGMEM;
extern const char INTL_SAVE_AND_RESTART[] PROGMEM;
extern const char INTL_DEBUG_SETTING_TO[] PROGMEM;
extern const char INTL_TIME_TO_FIRST_MEASUREMENT[] PROGMEM;
extern const char INTL_TIME_SINCE_LAST_MEASUREMENT[] PROGMEM;
extern const char INTL_PARTICLES_PER_LITER[] PROGMEM;
extern const char INTL_PARTICULATE_MATTER[] PROGMEM;
extern const char INTL_TEMPERATURE[] PROGMEM;
extern const char INTL_HUMIDITY[] PROGMEM;
extern const char INTL_PRESSURE[] PROGMEM;
extern const char INTL_DEW_POINT[] PROGMEM;
extern const char INTL_CO2_PPM[] PROGMEM;
extern const char INTL_LEQ_A[] PROGMEM;
extern const char INTL_LA_MIN[] PROGMEM;
extern const char INTL_LA_MAX[] PROGMEM;
extern const char INTL_LATITUDE[] PROGMEM;
extern const char INTL_LONGITUDE[] PROGMEM;
extern const char INTL_ALTITUDE[] PROGMEM;
extern const char INTL_TIME_UTC[] PROGMEM;
extern const char INTL_SIGNAL_STRENGTH[] PROGMEM;
extern const char INTL_SIGNAL_QUALITY[] PROGMEM;
