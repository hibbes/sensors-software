/**
 * /values Handler — aktuelle Sensor-Lesewerte als HTML-Tabelle.
 *
 * Issue #18 Phase B — ausgelagert aus airrohr-firmware.ino.
 */
#include "../page_helpers.h"
#include <cmath>

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include <WiFi.h>
#endif

void webserver_values()
{
	if (WiFi.status() != WL_CONNECTED)
	{
		sendHttpRedirect();
		return;
	}

	RESERVE_STRING(page_content, XLARGE_STR);
	start_html_page(page_content, FPSTR(INTL_CURRENT_DATA));
	const String unit_Deg("°");
	const String unit_P("hPa");
	const String unit_T("°C");
	const String unit_CO2("ppm");
	const String unit_LA(F("dB(A)"));
	float dew_point_temp;

	const int signal_quality = calcWiFiSignalQuality(last_signal_strength);
	debug_outln_info(F("ws: values ..."));
	if (!count_sends)
	{
		page_content += F("<b style='color:red'>");
		add_warning_first_cycle(page_content);
		page_content += FPSTR(WEB_B_BR_BR);
	}
	else
	{
		add_age_last_values(page_content);
	}

	auto add_table_pm_value = [&page_content](const __FlashStringHelper *sensor, const __FlashStringHelper *param, const float &value)
	{
		add_table_row_from_value(page_content, sensor, param, check_display_value(value, -1, 1, 0), F("µg/m³"));
	};

	auto add_table_nc_value = [&page_content](const __FlashStringHelper *sensor, const __FlashStringHelper *param, const float value)
	{
		add_table_row_from_value(page_content, sensor, param, check_display_value(value, -1, 1, 0), F("#/cm³"));
	};

	auto add_table_t_value = [&page_content](const __FlashStringHelper *sensor, const __FlashStringHelper *param, const float value)
	{
		add_table_row_from_value(page_content, sensor, param, check_display_value(value, -128, 1, 0), "°C");
	};

	auto add_table_h_value = [&page_content](const __FlashStringHelper *sensor, const __FlashStringHelper *param, const float value)
	{
		add_table_row_from_value(page_content, sensor, param, check_display_value(value, -1, 1, 0), "%");
	};

	auto add_table_value = [&page_content](const __FlashStringHelper *sensor, const __FlashStringHelper *param, const String &value, const String &unit)
	{
		add_table_row_from_value(page_content, sensor, param, value, unit);
	};

	server.sendContent(page_content);
	page_content = F("<table cellspacing='0' cellpadding='5' class='v'>\n"
					 "<thead><tr><th>" INTL_SENSOR "</th><th> " INTL_PARAMETER "</th><th>" INTL_VALUE "</th></tr></thead>");
	if (cfg::ppd_read)
	{
		add_table_value(FPSTR(SENSORS_PPD42NS), FPSTR(WEB_PM1), check_display_value(last_value_PPD_P1, -1, 1, 0), FPSTR(INTL_PARTICLES_PER_LITER));
		add_table_value(FPSTR(SENSORS_PPD42NS), FPSTR(WEB_PM25), check_display_value(last_value_PPD_P2, -1, 1, 0), FPSTR(INTL_PARTICLES_PER_LITER));
		page_content += FPSTR(EMPTY_ROW);
	}
	if (cfg::sds_read)
	{
		add_table_pm_value(FPSTR(SENSORS_SDS011), FPSTR(WEB_PM25), last_value_SDS_P2);
		add_table_pm_value(FPSTR(SENSORS_SDS011), FPSTR(WEB_PM10), last_value_SDS_P1);
		page_content += FPSTR(EMPTY_ROW);
	}
	if (cfg::pms_read)
	{
		add_table_pm_value(FPSTR(SENSORS_PMSx003), FPSTR(WEB_PM1), last_value_PMS_P0);
		add_table_pm_value(FPSTR(SENSORS_PMSx003), FPSTR(WEB_PM25), last_value_PMS_P2);
		add_table_pm_value(FPSTR(SENSORS_PMSx003), FPSTR(WEB_PM10), last_value_PMS_P1);
		page_content += FPSTR(EMPTY_ROW);
	}
	if (cfg::hpm_read)
	{
		add_table_pm_value(FPSTR(SENSORS_HPM), FPSTR(WEB_PM25), last_value_HPM_P2);
		add_table_pm_value(FPSTR(SENSORS_HPM), FPSTR(WEB_PM10), last_value_HPM_P1);
		page_content += FPSTR(EMPTY_ROW);
	}
	if (cfg::npm_read)
	{
		add_table_pm_value(FPSTR(SENSORS_NPM), FPSTR(WEB_PM1), last_value_NPM_P0);
		add_table_pm_value(FPSTR(SENSORS_NPM), FPSTR(WEB_PM25), last_value_NPM_P2);
		add_table_pm_value(FPSTR(SENSORS_NPM), FPSTR(WEB_PM10), last_value_NPM_P1);
		add_table_nc_value(FPSTR(SENSORS_NPM), FPSTR(WEB_NC1k0), last_value_NPM_N1);
		add_table_nc_value(FPSTR(SENSORS_NPM), FPSTR(WEB_NC2k5), last_value_NPM_N25);
		add_table_nc_value(FPSTR(SENSORS_NPM), FPSTR(WEB_NC10), last_value_NPM_N10);
		page_content += FPSTR(EMPTY_ROW);
	}
	if (cfg::ips_read)
	{
		add_table_pm_value(FPSTR(SENSORS_IPS), FPSTR(WEB_PM01), last_value_IPS_P01);
		add_table_pm_value(FPSTR(SENSORS_IPS), FPSTR(WEB_PM03), last_value_IPS_P03);
		add_table_pm_value(FPSTR(SENSORS_IPS), FPSTR(WEB_PM05), last_value_IPS_P05);
		add_table_pm_value(FPSTR(SENSORS_IPS), FPSTR(WEB_PM1), last_value_IPS_P0);
		add_table_pm_value(FPSTR(SENSORS_IPS), FPSTR(WEB_PM25), last_value_IPS_P2);
		add_table_pm_value(FPSTR(SENSORS_IPS), FPSTR(WEB_PM5), last_value_IPS_P5);
		add_table_pm_value(FPSTR(SENSORS_IPS), FPSTR(WEB_PM10), last_value_IPS_P1);
		add_table_nc_value(FPSTR(SENSORS_IPS), FPSTR(WEB_NC0k1), last_value_IPS_N01);
		add_table_nc_value(FPSTR(SENSORS_IPS), FPSTR(WEB_NC0k3), last_value_IPS_N03);
		add_table_nc_value(FPSTR(SENSORS_IPS), FPSTR(WEB_NC0k5), last_value_IPS_N05);
		add_table_nc_value(FPSTR(SENSORS_IPS), FPSTR(WEB_NC1k0), last_value_IPS_N1);
		add_table_nc_value(FPSTR(SENSORS_IPS), FPSTR(WEB_NC2k5), last_value_IPS_N25);
		add_table_nc_value(FPSTR(SENSORS_IPS), FPSTR(WEB_NC5k0), last_value_IPS_N5);
		add_table_nc_value(FPSTR(SENSORS_IPS), FPSTR(WEB_NC10), last_value_IPS_N10);
		page_content += FPSTR(EMPTY_ROW);
	}
	if (cfg::sps30_read)
	{
		add_table_pm_value(FPSTR(SENSORS_SPS30), FPSTR(WEB_PM1), last_value_SPS30_P0);
		add_table_pm_value(FPSTR(SENSORS_SPS30), FPSTR(WEB_PM25), last_value_SPS30_P2);
		add_table_pm_value(FPSTR(SENSORS_SPS30), FPSTR(WEB_PM4), last_value_SPS30_P4);
		add_table_pm_value(FPSTR(SENSORS_SPS30), FPSTR(WEB_PM10), last_value_SPS30_P1);
		add_table_nc_value(FPSTR(SENSORS_SPS30), FPSTR(WEB_NC0k5), last_value_SPS30_N05);
		add_table_nc_value(FPSTR(SENSORS_SPS30), FPSTR(WEB_NC1k0), last_value_SPS30_N1);
		add_table_nc_value(FPSTR(SENSORS_SPS30), FPSTR(WEB_NC2k5), last_value_SPS30_N25);
		add_table_nc_value(FPSTR(SENSORS_SPS30), FPSTR(WEB_NC4k0), last_value_SPS30_N4);
		add_table_nc_value(FPSTR(SENSORS_SPS30), FPSTR(WEB_NC10), last_value_SPS30_N10);
		add_table_value(FPSTR(SENSORS_SPS30), FPSTR(WEB_TPS), check_display_value(last_value_SPS30_TS, -1, 1, 0), "µm");
		page_content += FPSTR(EMPTY_ROW);
	}
	if (cfg::dht_read)
	{
		add_table_t_value(FPSTR(SENSORS_DHT22), FPSTR(INTL_TEMPERATURE), last_value_DHT_T);
		add_table_h_value(FPSTR(SENSORS_DHT22), FPSTR(INTL_HUMIDITY), last_value_DHT_H);
		page_content += FPSTR(EMPTY_ROW);
	}
	if (cfg::htu21d_read)
	{
		add_table_t_value(FPSTR(SENSORS_HTU21D), FPSTR(INTL_TEMPERATURE), last_value_HTU21D_T);
		add_table_h_value(FPSTR(SENSORS_HTU21D), FPSTR(INTL_HUMIDITY), last_value_HTU21D_H);
		dew_point_temp = dew_point(last_value_HTU21D_T, last_value_HTU21D_H);
		add_table_value(FPSTR(SENSORS_HTU21D), FPSTR(INTL_DEW_POINT), isnan(dew_point_temp) ? "-" : String(dew_point_temp, 1), unit_T);
		page_content += FPSTR(EMPTY_ROW);
	}
	if (cfg::aht20_read)
	{
		add_table_t_value(FPSTR(SENSORS_AHT20), FPSTR(INTL_TEMPERATURE), last_value_AHT20_T);
		add_table_h_value(FPSTR(SENSORS_AHT20), FPSTR(INTL_HUMIDITY), last_value_AHT20_H);
		dew_point_temp = dew_point(last_value_AHT20_T, last_value_AHT20_H);
		add_table_value(FPSTR(SENSORS_AHT20), FPSTR(INTL_DEW_POINT), isnan(dew_point_temp) ? "-" : String(dew_point_temp, 1), unit_T);
		page_content += FPSTR(EMPTY_ROW);
	}
	if (cfg::bmp_read)
	{
		add_table_t_value(FPSTR(SENSORS_BMP180), FPSTR(INTL_TEMPERATURE), last_value_BMP_T);
		add_table_value(FPSTR(SENSORS_BMP180), FPSTR(INTL_PRESSURE), check_display_value(last_value_BMP_P / 100.0f, (-1 / 100.0f), 2, 0), unit_P);
		add_table_value(FPSTR(SENSORS_BMP180), FPSTR(INTL_PRESSURE_AT_SEALEVEL), last_value_BMP_P != -1.0f ? String(pressure_at_sealevel(last_value_BMP_T, last_value_BMP_P / 100.0f), 2) : "-", unit_P);
		page_content += FPSTR(EMPTY_ROW);
	}
	if (cfg::bmx280_read)
	{
		const char *const sensor_name = (bmx280.sensorID() == BME280_SENSOR_ID) ? SENSORS_BME280 : SENSORS_BMP280;
		add_table_t_value(FPSTR(sensor_name), FPSTR(INTL_TEMPERATURE), last_value_BMX280_T);
		add_table_value(FPSTR(sensor_name), FPSTR(INTL_PRESSURE), check_display_value(last_value_BMX280_P / 100.0f, (-1 / 100.0f), 2, 0), unit_P);
		add_table_value(FPSTR(sensor_name), FPSTR(INTL_PRESSURE_AT_SEALEVEL), last_value_BMX280_P != -1.0f ? String(pressure_at_sealevel(last_value_BMX280_T, last_value_BMX280_P / 100.0f), 2) : "-", unit_P);
		if (bmx280.sensorID() == BME280_SENSOR_ID)
		{
			add_table_h_value(FPSTR(sensor_name), FPSTR(INTL_HUMIDITY), last_value_BME280_H);
			dew_point_temp = dew_point(last_value_BMX280_T, last_value_BME280_H);
			add_table_value(FPSTR(sensor_name), FPSTR(INTL_DEW_POINT), isnan(dew_point_temp) ? "-" : String(dew_point_temp, 1), unit_T);
		}
		page_content += FPSTR(EMPTY_ROW);
	}
	if (cfg::sht3x_read)
	{
		add_table_t_value(FPSTR(SENSORS_SHT3X), FPSTR(INTL_TEMPERATURE), last_value_SHT3X_T);
		add_table_h_value(FPSTR(SENSORS_SHT3X), FPSTR(INTL_HUMIDITY), last_value_SHT3X_H);
		dew_point_temp = dew_point(last_value_SHT3X_T, last_value_SHT3X_H);
		add_table_value(FPSTR(SENSORS_SHT3X), FPSTR(INTL_DEW_POINT), isnan(dew_point_temp) ? "-" : String(dew_point_temp, 1), unit_T);
		page_content += FPSTR(EMPTY_ROW);
	}
	if (cfg::scd30_read)
	{
		add_table_t_value(FPSTR(SENSORS_SCD30), FPSTR(INTL_TEMPERATURE), last_value_SCD30_T);
		add_table_h_value(FPSTR(SENSORS_SCD30), FPSTR(INTL_HUMIDITY), last_value_SCD30_H);
		add_table_value(FPSTR(SENSORS_SCD30), FPSTR(INTL_CO2_PPM), check_display_value(last_value_SCD30_CO2, 0, 0, 0), unit_CO2);
		dew_point_temp = dew_point(last_value_SCD30_T, last_value_SCD30_H);
		add_table_value(FPSTR(SENSORS_SCD30), FPSTR(INTL_DEW_POINT), isnan(dew_point_temp) ? "-" : String(dew_point_temp, 1), unit_T);
		page_content += FPSTR(EMPTY_ROW);
	}
	if (cfg::ds18b20_read)
	{
		add_table_t_value(FPSTR(SENSORS_DS18B20), FPSTR(INTL_TEMPERATURE), last_value_DS18B20_T);
		page_content += FPSTR(EMPTY_ROW);
	}
	if (cfg::dnms_read)
	{
		add_table_value(FPSTR(SENSORS_DNMS), FPSTR(INTL_LEQ_A), check_display_value(last_value_dnms_laeq, -1, 1, 0), unit_LA);
		add_table_value(FPSTR(SENSORS_DNMS), FPSTR(INTL_LA_MIN), check_display_value(last_value_dnms_la_min, -1, 1, 0), unit_LA);
		add_table_value(FPSTR(SENSORS_DNMS), FPSTR(INTL_LA_MAX), check_display_value(last_value_dnms_la_max, -1, 1, 0), unit_LA);
		page_content += FPSTR(EMPTY_ROW);
	}
	if (cfg::gps_read)
	{
		add_table_value(FPSTR(WEB_GPS), FPSTR(INTL_LATITUDE), check_display_value(last_value_GPS_lat, -200.0, 6, 0), unit_Deg);
		add_table_value(FPSTR(WEB_GPS), FPSTR(INTL_LONGITUDE), check_display_value(last_value_GPS_lon, -200.0, 6, 0), unit_Deg);
		add_table_value(FPSTR(WEB_GPS), FPSTR(INTL_ALTITUDE), check_display_value(last_value_GPS_alt, -1000.0, 2, 0), "m");
		add_table_value(FPSTR(WEB_GPS), FPSTR(INTL_TIME_UTC), last_value_GPS_timestamp, emptyString);
		page_content += FPSTR(EMPTY_ROW);
	}

	server.sendContent(page_content);
	page_content = emptyString;

	add_table_value(F("WiFi"), FPSTR(INTL_SIGNAL_STRENGTH), String(last_signal_strength), "dBm");
	add_table_value(F("WiFi"), FPSTR(INTL_SIGNAL_QUALITY), String(signal_quality), "%");

	page_content += FPSTR(TABLE_TAG_CLOSE_BR);
	page_content += FPSTR(BR_TAG);
	end_html_page(page_content);
}
