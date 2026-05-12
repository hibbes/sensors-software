/**
 * /status Handler — Firmware-Version, Heap, NTP, Sensor-Versions, Error-Counter.
 *
 * Issue #18 Phase B — ausgelagert aus airrohr-firmware.ino.
 */
#include "../page_helpers.h"

#if defined(ESP8266)
#include <ESP8266HTTPClient.h>
#include <coredecls.h>
#include <sntp.h>
#elif defined(ESP32)
#include <HTTPClient.h>
#endif

#include <StreamString.h>
#include <time.h>

void webserver_status()
{
	if (WiFi.status() != WL_CONNECTED)
	{
		sendHttpRedirect();
		return;
	}

	RESERVE_STRING(page_content, XLARGE_STR);
	start_html_page(page_content, FPSTR(INTL_DEVICE_STATUS));

	debug_outln_info(F("ws: status ..."));
	server.sendContent(page_content);
	page_content = F("<table cellspacing='0' cellpadding='5' class='v'>\n"
					 "<thead><tr><th> " INTL_PARAMETER "</th><th>" INTL_VALUE "</th></tr></thead>");
	String versionHtml(SOFTWARE_VERSION);
	versionHtml += F("/ST:");
	versionHtml += String(!airrohr_selftest_failed);
	versionHtml += '/';
#if defined(ESP8266)
	versionHtml += ESP.getFullVersion();
#endif
	versionHtml.replace("/", FPSTR(BR_TAG));
	add_table_row_from_value(page_content, FPSTR(INTL_FIRMWARE), versionHtml);
	add_table_row_from_value(page_content, F("Free Memory"), String(ESP.getFreeHeap()));
#if defined(ESP8266)
	add_table_row_from_value(page_content, F("Heap Fragmentation"), String(ESP.getHeapFragmentation()), "%");
#endif
	if (cfg::auto_update)
	{
		add_table_row_from_value(page_content, F("Last OTA"), delayToString(millis() - last_update_attempt));
	}
#if defined(ESP8266)
	add_table_row_from_value(page_content, F("NTP Sync"), String(sntp_time_set));
	StreamString ntpinfo;

	for (unsigned i = 0; i < SNTP_MAX_SERVERS; i++)
	{
		const ip_addr_t *sntp = sntp_getserver(i);
		if (sntp && !ip_addr_isany(sntp))
		{
			const char *name = sntp_getservername(i);
			if (!ntpinfo.isEmpty())
			{
				ntpinfo.print(FPSTR(BR_TAG));
			}
			ntpinfo.printf_P(PSTR("%s (%s)"), IPAddress(*sntp).toString().c_str(), name ? name : "?");
			ntpinfo.printf_P(PSTR(" reachable: %s"), sntp_getreachability(i) ? "Yes" : "No");
		}
	}
	add_table_row_from_value(page_content, F("NTP Info"), ntpinfo);
#endif

	time_t now = time(nullptr);
	add_table_row_from_value(page_content, FPSTR(INTL_TIME_UTC), ctime(&now));
	add_table_row_from_value(page_content, F("Uptime"), delayToString(millis() - time_point_device_start_ms));
#if defined(ESP8266)
	add_table_row_from_value(page_content, F("Reset Reason"), ESP.getResetReason());
#endif
	if (cfg::sds_read)
	{
		page_content += FPSTR(EMPTY_ROW);
		add_table_row_from_value(page_content, FPSTR(SENSORS_SDS011), last_value_SDS_version);
	}
	if (cfg::npm_read)
	{
		page_content += FPSTR(EMPTY_ROW);
		add_table_row_from_value(page_content, FPSTR(SENSORS_NPM), last_value_NPM_version);
	}
	if (cfg::scd30_read)
	{
		if (scd30.getAutoSelfCalibration() == true)
			add_table_row_from_value(page_content, F("SCD30 Auto Calibration"), "enabled");
		else
			add_table_row_from_value(page_content, F("SCD30 Auto Calibration"), "disabled");
		uint16_t settingVal;
		scd30.getMeasurementInterval(&settingVal);
		add_table_row_from_value(page_content, F("SCD30 measurement interval"), String(settingVal));
		scd30.getTemperatureOffset(&settingVal);
		add_table_row_from_value(page_content, F("SCD30 temperature offset"), String(settingVal));
	}

	page_content += FPSTR(EMPTY_ROW);
	page_content += F("<tr><td colspan='2'><b>" INTL_ERROR "</b></td></tr>");
	String wifiStatus(WiFi_error_count);
	wifiStatus += '/';
	wifiStatus += String(last_signal_strength);
	wifiStatus += '/';
	wifiStatus += String(last_disconnect_reason);
	add_table_row_from_value(page_content, F("WiFi"), wifiStatus);

	if (last_update_returncode != 0)
	{
		add_table_row_from_value(page_content, F("OTA Return"),
								 last_update_returncode > 0 ? String(last_update_returncode) : HTTPClient::errorToString(last_update_returncode));
	}
	for (unsigned int i = 0; i < LoggerCount; ++i)
	{
		if (loggerConfigs[i].errors)
		{
			const __FlashStringHelper *logger = loggerDescription(i);
			if (logger)
			{
				add_table_row_from_value(page_content, logger, String(loggerConfigs[i].errors));
			}
		}
	}

	if (last_sendData_returncode != 0)
	{
		add_table_row_from_value(page_content, F("Data Send Return"),
								 last_sendData_returncode > 0 ? String(last_sendData_returncode) : HTTPClient::errorToString(last_sendData_returncode));
	}
	if (cfg::sds_read)
	{
		add_table_row_from_value(page_content, FPSTR(SENSORS_SDS011), String(SDS_error_count));
	}
	if (cfg::npm_read)
	{
		add_table_row_from_value(page_content, FPSTR(SENSORS_NPM), String(NPM_error_count));
	}
	if (cfg::sps30_read)
	{
		add_table_row_from_value(page_content, FPSTR(SENSORS_SPS30), String(SPS30_read_error_counter));
	}

	server.sendContent(page_content);
	page_content = emptyString;

	if (count_sends > 0)
	{
		page_content += FPSTR(EMPTY_ROW);
		add_table_row_from_value(page_content, F(INTL_NUMBER_OF_MEASUREMENTS), String(count_sends));
		if (sending_time > 0)
		{
			add_table_row_from_value(page_content, F(INTL_TIME_SENDING_MS), String(sending_time), "ms");
		}
	}

	page_content += FPSTR(TABLE_TAG_CLOSE_BR);
	end_html_page(page_content);
}
