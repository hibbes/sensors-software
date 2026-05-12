/**
 * PROGMEM declarations for static HTML/CSS/JS fragments and protocol strings.
 *
 * Issue #18 Phase B: split into header (decls) + cpp (defs) so any TU can include
 * the header without causing multiple-definition errors at link time.
 *
 * Definitions live in html-content.cpp.
 */
#pragma once

#include "./intl-decls.h"

#define STATIC_PREFIX "/" INTL_LANG "_s1"

extern const char TXT_CONTENT_TYPE_JSON[] PROGMEM;
extern const char TXT_CONTENT_TYPE_INFLUXDB[] PROGMEM;
extern const char TXT_CONTENT_TYPE_TEXT_HTML[] PROGMEM;
extern const char TXT_CONTENT_TYPE_TEXT_CSS[] PROGMEM;
extern const char TXT_CONTENT_TYPE_TEXT_PLAIN[] PROGMEM;
extern const char TXT_CONTENT_TYPE_IMAGE_PNG[] PROGMEM;
extern const char DBG_TXT_TEMPERATURE[] PROGMEM;
extern const char DBG_TXT_HUMIDITY[] PROGMEM;
extern const char DBG_TXT_PRESSURE[] PROGMEM;
extern const char DBG_TXT_CO2PPM[] PROGMEM;
extern const char DBG_TXT_START_READING[] PROGMEM;
extern const char DBG_TXT_END_READING[] PROGMEM;
extern const char DBG_TXT_CHECKSUM_IS[] PROGMEM;
extern const char DBG_TXT_CHECKSUM_SHOULD[] PROGMEM;
extern const char DBG_TXT_DATA_READ_FAILED[] PROGMEM;
extern const char DBG_TXT_UPDATE[] PROGMEM;
extern const char DBG_TXT_UPDATE_FAILED[] PROGMEM;
extern const char DBG_TXT_UPDATE_NO_UPDATE[] PROGMEM;
extern const char DBG_TXT_SENDING_TO[] PROGMEM;
extern const char DBG_TXT_SDS011_VERSION_DATE[] PROGMEM;
extern const char DBG_TXT_NPM_VERSION_DATE[] PROGMEM;
extern const char DBG_TXT_CONNECTING_TO[] PROGMEM;
extern const char DBG_TXT_FOUND[] PROGMEM;
extern const char DBG_TXT_NOT_FOUND[] PROGMEM;
extern const char DBG_TXT_SEP[] PROGMEM;
extern const char SENSORS_SDS011[] PROGMEM;
extern const char SENSORS_PPD42NS[] PROGMEM;
extern const char SENSORS_PMSx003[] PROGMEM;
extern const char SENSORS_HPM[] PROGMEM;
extern const char SENSORS_NPM[] PROGMEM;
extern const char SENSORS_IPS[] PROGMEM;
extern const char SENSORS_SPS30[] PROGMEM;
extern const char SENSORS_DHT22[] PROGMEM;
extern const char SENSORS_DS18B20[] PROGMEM;
extern const char SENSORS_HTU21D[] PROGMEM;
extern const char SENSORS_AHT20[] PROGMEM;
extern const char SENSORS_SHT3X[] PROGMEM;
extern const char SENSORS_SCD30[] PROGMEM;
extern const char SENSORS_BMP180[] PROGMEM;
extern const char SENSORS_BME280[] PROGMEM;
extern const char SENSORS_BMP280[] PROGMEM;
extern const char SENSORS_DNMS[] PROGMEM;
extern const char WEB_PAGE_HEADER[] PROGMEM;
// WEB_PAGE_STATIC_CSS moved to web/assets/style.css (Issue #18 Phase D)
// → use ASSET_STYLE_CSS from web/assets_generated.h
extern const char WEB_PAGE_HEADER_HEAD[] PROGMEM;
extern const char WEB_PAGE_HEADER_BODY[] PROGMEM;
extern const char BR_TAG[] PROGMEM;
extern const char WEB_DIV_PANEL[] PROGMEM;
extern const char TABLE_TAG_OPEN[] PROGMEM;
extern const char TABLE_TAG_CLOSE_BR[] PROGMEM;
extern const char EMPTY_ROW[] PROGMEM;
extern const char WEB_PAGE_FOOTER[] PROGMEM;
extern const char WEB_ROOT_PAGE_CONTENT[] PROGMEM;
extern const char WEB_CONFIG_SCRIPT[] PROGMEM;
extern const char WEB_REMOVE_CONFIG_CONTENT[] PROGMEM;
extern const char WEB_RESET_CONTENT[] PROGMEM;
extern const char WEB_IOS_REDIRECT[] PROGMEM;
extern const char WEB_B_BR_BR[] PROGMEM;
extern const char WEB_BRACE_BR[] PROGMEM;
extern const char WEB_B_BR[] PROGMEM;
extern const char WEB_BR_BR[] PROGMEM;
extern const char WEB_BR_FORM[] PROGMEM;
extern const char WEB_BR_LF_B[] PROGMEM;
extern const char WEB_LF_B[] PROGMEM;
extern const char WEB_CSV[] PROGMEM;
extern const char WEB_FEINSTAUB_APP[] PROGMEM;
extern const char WEB_OPENSENSEMAP[] PROGMEM;
extern const char WEB_AIRCMS[] PROGMEM;
extern const char WEB_MADAVI[] PROGMEM;
extern const char WEB_SENSORCOMMUNITY[] PROGMEM;
extern const char WEB_HTTPS[] PROGMEM;
extern const char WEB_NBSP_NBSP_BRACE[] PROGMEM;
extern const char WEB_REPLN_REPLV[] PROGMEM;
extern const char WEB_PM1[] PROGMEM;
extern const char WEB_PM25[] PROGMEM;
extern const char WEB_PM10[] PROGMEM;
extern const char WEB_PM4[] PROGMEM;
extern const char WEB_PM01[] PROGMEM;
extern const char WEB_PM03[] PROGMEM;
extern const char WEB_PM05[] PROGMEM;
extern const char WEB_PM5[] PROGMEM;
extern const char WEB_NC0k1[] PROGMEM;
extern const char WEB_NC0k3[] PROGMEM;
extern const char WEB_NC0k5[] PROGMEM;
extern const char WEB_NC1k0[] PROGMEM;
extern const char WEB_NC2k5[] PROGMEM;
extern const char WEB_NC4k0[] PROGMEM;
extern const char WEB_NC5k0[] PROGMEM;
extern const char WEB_NC10[] PROGMEM;
extern const char WEB_TPS[] PROGMEM;
extern const char WEB_GPS[] PROGMEM;
