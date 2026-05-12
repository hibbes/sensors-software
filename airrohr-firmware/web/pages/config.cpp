/**
 * /config Handler — größte HTML-Page der airrohr-Firmware.
 *
 * Drei Funktionen ausgegliedert:
 *  - webserver_config_send_body_get(): rendert das HTML-Formular mit 4 Tab-Panels
 *  - webserver_config_send_body_post(): wendet POST-Daten auf cfg::* an (configShape[])
 *  - webserver_config(): orchestriert GET/POST + writeConfig + sensor_restart
 *
 * Plus /config.json (Cfg-Backup/Restore über JSON) als sauberer Roundtrip-Endpoint.
 *
 * Phase B (B): ausgelagert aus airrohr-firmware.ino.
 * Phase C-2: Tab-Bodies in 4 static Helpers gesplittet — render_config_tab_*.
 *
 * Issue #18.
 */
#include "../page_helpers.h"
#include <ArduinoJson.h>

namespace
{

inline void add_form_checkbox(String &p, const ConfigShapeId cfgid, const String &info)
{
	p += form_checkbox(cfgid, info, true);
}

inline void add_form_checkbox_sensor(String &p, const ConfigShapeId cfgid, const __FlashStringHelper *info)
{
	add_form_checkbox(p, cfgid, add_sensor_type(info));
}

void render_config_tabs_header(String &page_content)
{
	page_content += F("<form method='POST' action='/config' style='width:100%;'>\n"
					  "<input class='radio' id='r1' name='group' type='radio' checked>"
					  "<input class='radio' id='r2' name='group' type='radio'>"
					  "<input class='radio' id='r3' name='group' type='radio'>"
					  "<input class='radio' id='r4' name='group' type='radio'>"
					  "<div class='tabs'>"
					  "<label class='tab' id='tab1' for='r1'>" INTL_WIFI_SETTINGS "</label>"
					  "<label class='tab' id='tab2' for='r2'>");
	page_content += FPSTR(INTL_MORE_SETTINGS);
	page_content += F("</label>"
					  "<label class='tab' id='tab3' for='r3'>");
	page_content += FPSTR(INTL_SENSORS);
	page_content += F("</label>"
					  "<label class='tab' id='tab4' for='r4'>APIs"
					  "</label></div><div class='panels'>"
					  "<div class='panel' id='panel1'>");
}

// Tab 1: WLAN-Settings + BasicAuth + AP-Mode-Settings
void render_config_tab_wifi(String &page_content)
{
	if (wificonfig_loop)
	{
		page_content += F("<div id='wifilist'>" INTL_WIFI_NETWORKS "</div><br/>");
	}
	page_content += FPSTR(TABLE_TAG_OPEN);
	add_form_input(page_content, Config_wlanssid, FPSTR(INTL_FS_WIFI_NAME), LEN_WLANSSID - 1);
	add_form_input(page_content, Config_wlanpwd, FPSTR(INTL_PASSWORD), LEN_CFG_PASSWORD - 1);
	page_content += FPSTR(TABLE_TAG_CLOSE_BR);
	page_content += F("<hr/>");

	page_content += FPSTR(WEB_BR_LF_B);
	page_content += FPSTR(INTL_AB_HIER_NUR_ANDERN);
	page_content += FPSTR(WEB_B_BR);
	page_content += FPSTR(BR_TAG);

	server.sendContent(page_content);
	page_content = emptyString;

	add_form_checkbox(page_content, Config_www_basicauth_enabled, FPSTR(INTL_BASICAUTH));
	page_content += FPSTR(TABLE_TAG_OPEN);
	add_form_input(page_content, Config_www_username, FPSTR(INTL_USER), LEN_WWW_USERNAME - 1);
	add_form_input(page_content, Config_www_password, FPSTR(INTL_PASSWORD), LEN_CFG_PASSWORD - 1);
	page_content += FPSTR(TABLE_TAG_CLOSE_BR);
	page_content += FPSTR(BR_TAG);

	server.sendContent(page_content);

	if (!wificonfig_loop)
	{
		page_content = FPSTR(INTL_FS_WIFI_DESCRIPTION);
		page_content += FPSTR(BR_TAG);

		page_content += FPSTR(TABLE_TAG_OPEN);
		add_form_input(page_content, Config_fs_ssid, FPSTR(INTL_FS_WIFI_NAME), LEN_FS_SSID - 1);
		add_form_input(page_content, Config_fs_pwd, FPSTR(INTL_PASSWORD), LEN_CFG_PASSWORD - 1);
		page_content += FPSTR(TABLE_TAG_CLOSE_BR);

		server.sendContent(page_content);
	}
}

// Tab 2: Display + Static-IP + Firmware/OTA + Powersave + Debug-Level
void render_config_tab_more(String &page_content)
{
	page_content = tmpl(FPSTR(WEB_DIV_PANEL), String(2));

	add_form_checkbox(page_content, Config_has_display, FPSTR(INTL_DISPLAY));
	add_form_checkbox(page_content, Config_has_sh1106, FPSTR(INTL_SH1106));
	add_form_checkbox(page_content, Config_has_flipped_display, FPSTR(INTL_FLIP_DISPLAY));
	add_form_checkbox(page_content, Config_has_lcd1602_27, FPSTR(INTL_LCD1602_27));
	add_form_checkbox(page_content, Config_has_lcd1602, FPSTR(INTL_LCD1602_3F));

	server.sendContent(page_content);
	page_content = emptyString;

	add_form_checkbox(page_content, Config_has_lcd2004_27, FPSTR(INTL_LCD2004_27));
	add_form_checkbox(page_content, Config_has_lcd2004, FPSTR(INTL_LCD2004_3F));
	add_form_checkbox(page_content, Config_display_wifi_info, FPSTR(INTL_DISPLAY_WIFI_INFO));
	add_form_checkbox(page_content, Config_display_device_info, FPSTR(INTL_DISPLAY_DEVICE_INFO));

	page_content += FPSTR(WEB_BR_LF_B);
	page_content += F(INTL_STATIC_IP_TEXT);
	page_content += FPSTR(WEB_B_BR);
	page_content += FPSTR(TABLE_TAG_OPEN);
	add_form_input(page_content, Config_static_ip, FPSTR(INTL_STATIC_IP), 15);
	add_form_input(page_content, Config_static_subnet, FPSTR(INTL_STATIC_SUBNET), 15);
	add_form_input(page_content, Config_static_gateway, FPSTR(INTL_STATIC_GATEWAY), 15);
	add_form_input(page_content, Config_static_dns, FPSTR(INTL_STATIC_DNS), 15);
	page_content += FPSTR(TABLE_TAG_CLOSE_BR);

	server.sendContent(page_content);
	page_content = FPSTR(WEB_BR_LF_B);
	page_content += F(INTL_FIRMWARE);
	page_content += FPSTR(WEB_B_BR);
	add_form_checkbox(page_content, Config_auto_update, FPSTR(INTL_AUTO_UPDATE));
	add_form_checkbox(page_content, Config_use_beta, FPSTR(INTL_USE_BETA));

	page_content += FPSTR(TABLE_TAG_OPEN);
	page_content += form_select_lang();
	// hibbes-Patch: konfigurierbarer Update-Server (leer = Default firmware.sensor.community)
	add_form_input(page_content, Config_ota_host, F("Update-Host (leer = Default)"), LEN_OTA_HOST - 1);
	page_content += FPSTR(TABLE_TAG_CLOSE_BR);

	page_content += F("<script>"
					  "var $ = function(e) { return document.getElementById(e); };"
					  "function updateOTAOptions() { "
					  "$('current_lang').disabled = $('use_beta').disabled = $('ota_host').disabled = !$('auto_update').checked; "
					  "}; updateOTAOptions(); $('auto_update').onchange = updateOTAOptions;"
					  "</script>");

	page_content += FPSTR(WEB_BR_LF_B);
	page_content += FPSTR(INTL_AB_HIER_NUR_ANDERN);
	page_content += FPSTR(WEB_B_BR);
	page_content += FPSTR(BR_TAG);

	add_form_checkbox(page_content, Config_powersave, FPSTR(INTL_POWERSAVE));
	page_content += FPSTR(TABLE_TAG_OPEN);
	add_form_input(page_content, Config_debug, FPSTR(INTL_DEBUG_LEVEL), 1);
	add_form_input(page_content, Config_sending_intervall_ms, FPSTR(INTL_MEASUREMENT_INTERVAL), 5);
	add_form_input(page_content, Config_time_for_wifi_config, FPSTR(INTL_DURATION_ROUTER_MODE), 5);
	page_content += FPSTR(TABLE_TAG_CLOSE_BR);

	server.sendContent(page_content);
}

// Tab 3: Sensor-Toggle-Checkboxes + Korrekturwerte
void render_config_tab_sensors(String &page_content)
{
	page_content = tmpl(FPSTR(WEB_DIV_PANEL), String(3));
	add_form_checkbox_sensor(page_content, Config_sds_read, FPSTR(INTL_SDS011));
	add_form_checkbox_sensor(page_content, Config_hpm_read, FPSTR(INTL_HPM));
	add_form_checkbox_sensor(page_content, Config_sps30_read, FPSTR(INTL_SPS30));

	server.sendContent(page_content);
	page_content = emptyString;

	add_form_checkbox_sensor(page_content, Config_dht_read, FPSTR(INTL_DHT22));
	add_form_checkbox_sensor(page_content, Config_htu21d_read, FPSTR(INTL_HTU21D));
	add_form_checkbox_sensor(page_content, Config_aht20_read, FPSTR(INTL_AHT20));
	add_form_checkbox_sensor(page_content, Config_bmx280_read, FPSTR(INTL_BMX280));
	add_form_checkbox_sensor(page_content, Config_sht3x_read, FPSTR(INTL_SHT3X));
	add_form_checkbox_sensor(page_content, Config_scd30_read, FPSTR(INTL_SCD30));

	server.sendContent(page_content);
	page_content = emptyString;

	add_form_checkbox_sensor(page_content, Config_dnms_read, FPSTR(INTL_DNMS));
	page_content += FPSTR(TABLE_TAG_OPEN);
	add_form_input(page_content, Config_dnms_correction, FPSTR(INTL_DNMS_CORRECTION), LEN_DNMS_CORRECTION - 1);
	add_form_input(page_content, Config_temp_correction, FPSTR(INTL_TEMP_CORRECTION), LEN_TEMP_CORRECTION - 1);
	add_form_input(page_content, Config_height_above_sealevel, FPSTR(INTL_HEIGHT_ABOVE_SEALEVEL), LEN_HEIGHT_ABOVE_SEALEVEL - 1);
	page_content += FPSTR(TABLE_TAG_CLOSE_BR);

	page_content += FPSTR(WEB_BR_LF_B);
	page_content += FPSTR(INTL_MORE_SENSORS);
	page_content += FPSTR(WEB_B_BR);

	add_form_checkbox_sensor(page_content, Config_ds18b20_read, FPSTR(INTL_DS18B20));
	add_form_checkbox_sensor(page_content, Config_pms_read, FPSTR(INTL_PMS));
	add_form_checkbox_sensor(page_content, Config_npm_read, FPSTR(INTL_NPM));
	add_form_checkbox_sensor(page_content, Config_npm_fulltime, FPSTR(INTL_NPM_FULLTIME));
	add_form_checkbox_sensor(page_content, Config_ips_read, FPSTR(INTL_IPS));
	add_form_checkbox_sensor(page_content, Config_bmp_read, FPSTR(INTL_BMP180));
	add_form_checkbox(page_content, Config_gps_read, FPSTR(INTL_NEO6M));

	server.sendContent(page_content);
}

// Tab 4: Public APIs (SC/Madavi/Sensemap/AirCMS/CSV/FSapp), Custom-Endpoint,
// Wunderground PWS, InfluxDB
void render_config_tab_apis(String &page_content)
{
	page_content = tmpl(FPSTR(WEB_DIV_PANEL), String(4));

	page_content += tmpl(FPSTR(INTL_SEND_TO), F("APIs"));
	page_content += FPSTR(BR_TAG);
	page_content += form_checkbox(Config_send2dusti, FPSTR(WEB_SENSORCOMMUNITY), false);
	page_content += FPSTR(WEB_NBSP_NBSP_BRACE);
	page_content += form_checkbox(Config_ssl_dusti, FPSTR(WEB_HTTPS), false);
	page_content += FPSTR(WEB_BRACE_BR);
	page_content += form_checkbox(Config_send2madavi, FPSTR(WEB_MADAVI), false);
	page_content += FPSTR(WEB_NBSP_NBSP_BRACE);
	page_content += form_checkbox(Config_ssl_madavi, FPSTR(WEB_HTTPS), false);
	page_content += FPSTR(WEB_BRACE_BR);
	add_form_checkbox(page_content, Config_send2csv, FPSTR(WEB_CSV));
	add_form_checkbox(page_content, Config_send2fsapp, FPSTR(WEB_FEINSTAUB_APP));
	add_form_checkbox(page_content, Config_send2aircms, FPSTR(WEB_AIRCMS));
	add_form_checkbox(page_content, Config_send2sensemap, FPSTR(WEB_OPENSENSEMAP));
	page_content += FPSTR(TABLE_TAG_OPEN);
	add_form_input(page_content, Config_senseboxid, F("senseBox&nbsp;ID"), LEN_SENSEBOXID - 1);

	server.sendContent(page_content);
	page_content = FPSTR(TABLE_TAG_CLOSE_BR);
	page_content += FPSTR(BR_TAG);
	page_content += form_checkbox(Config_send2custom, FPSTR(INTL_SEND_TO_OWN_API), false);
	page_content += FPSTR(WEB_NBSP_NBSP_BRACE);
	page_content += form_checkbox(Config_ssl_custom, FPSTR(WEB_HTTPS), false);
	page_content += FPSTR(WEB_BRACE_BR);

	server.sendContent(page_content);
	page_content = FPSTR(TABLE_TAG_OPEN);
	add_form_input(page_content, Config_host_custom, FPSTR(INTL_SERVER), LEN_HOST_CUSTOM - 1);
	add_form_input(page_content, Config_url_custom, FPSTR(INTL_PATH), LEN_URL_CUSTOM - 1);
	add_form_input(page_content, Config_port_custom, FPSTR(INTL_PORT), MAX_PORT_DIGITS);
	add_form_input(page_content, Config_user_custom, FPSTR(INTL_USER), LEN_USER_CUSTOM - 1);
	add_form_input(page_content, Config_pwd_custom, FPSTR(INTL_PASSWORD), LEN_CFG_PASSWORD - 1);
	page_content += FPSTR(TABLE_TAG_CLOSE_BR);

	page_content += FPSTR(BR_TAG);

	// hibbes-Patch (Issue #16): Wunderground PWS-Upload-API direct
	server.sendContent(page_content);
	page_content = form_checkbox(Config_send2wunderground, F("Wunderground PWS"), false);
	page_content += FPSTR(BR_TAG);
	page_content += FPSTR(TABLE_TAG_OPEN);
	add_form_input(page_content, Config_wu_station_id, F("Station-ID (z.B. ***REDACTED-STATION-ID***)"), LEN_WU_STATION_ID - 1);
	add_form_input(page_content, Config_wu_password, F("PWS-Upload-Password"), LEN_CFG_PASSWORD - 1);
	page_content += FPSTR(TABLE_TAG_CLOSE_BR);

	page_content += FPSTR(BR_TAG);

	server.sendContent(page_content);
	page_content = form_checkbox(Config_send2influx, tmpl(FPSTR(INTL_SEND_TO), F("InfluxDB")), false);

	page_content += FPSTR(WEB_NBSP_NBSP_BRACE);
	page_content += form_checkbox(Config_ssl_influx, FPSTR(WEB_HTTPS), false);
	page_content += FPSTR(WEB_BRACE_BR);
	page_content += FPSTR(TABLE_TAG_OPEN);
	add_form_input(page_content, Config_host_influx, FPSTR(INTL_SERVER), LEN_HOST_INFLUX - 1);
	add_form_input(page_content, Config_url_influx, FPSTR(INTL_PATH), LEN_URL_INFLUX - 1);
	add_form_input(page_content, Config_port_influx, FPSTR(INTL_PORT), MAX_PORT_DIGITS);
	add_form_input(page_content, Config_user_influx, FPSTR(INTL_USER), LEN_USER_INFLUX - 1);
	add_form_input(page_content, Config_pwd_influx, FPSTR(INTL_PASSWORD), LEN_PASS_INFLUX - 1);
	add_form_input(page_content, Config_measurement_name_influx, FPSTR(INTL_MEASUREMENT), LEN_MEASUREMENT_NAME_INFLUX - 1);
	page_content += FPSTR(TABLE_TAG_CLOSE_BR);
}

void render_config_form_footer(String &page_content)
{
	page_content += F("</div></div>");
	page_content += form_submit(FPSTR(INTL_SAVE_AND_RESTART));
	page_content += FPSTR(BR_TAG);
	page_content += FPSTR(WEB_BR_FORM);
	if (wificonfig_loop)
	{
		page_content += F("<script>window.setTimeout(load_wifi_list,1000);</script>");
	}

	server.sendContent(page_content);
	page_content = emptyString;
}

} // namespace

static void webserver_config_send_body_get(String &page_content)
{
	debug_outln_info(F("begin webserver_config_body_get ..."));
	render_config_tabs_header(page_content);
	render_config_tab_wifi(page_content);
	render_config_tab_more(page_content);
	render_config_tab_sensors(page_content);
	render_config_tab_apis(page_content);
	render_config_form_footer(page_content);
}

static void webserver_config_send_body_post(String &page_content)
{
	for (unsigned e = 0; e < sizeof(configShape) / sizeof(configShape[0]); ++e)
	{
		ConfigShapeEntry c;
		memcpy_P(&c, &configShape[e], sizeof(ConfigShapeEntry));
		const String s_param(c.cfg_key());
		if (!server.hasArg(s_param))
		{
			continue;
		}
		const String server_arg(server.arg(s_param));

		switch (c.cfg_type)
		{
		case Config_Type_UInt:
			*(c.cfg_val.as_uint) = server_arg.toInt();
			break;
		case Config_Type_Time:
			*(c.cfg_val.as_uint) = server_arg.toInt() * 1000;
			break;
		case Config_Type_Bool:
			*(c.cfg_val.as_bool) = (server_arg == "1");
			break;
		case Config_Type_String:
			strncpy(c.cfg_val.as_str, server_arg.c_str(), c.cfg_len);
			c.cfg_val.as_str[c.cfg_len] = '\0';
			break;
		case Config_Type_Password:
			if (server_arg.length())
			{
				server_arg.toCharArray(c.cfg_val.as_str, LEN_CFG_PASSWORD);
			}
			break;
		}
	}

	page_content += FPSTR(INTL_SENSOR_IS_REBOOTING);

	server.sendContent(page_content);
	page_content = emptyString;
}

/**
 * /config.json: GET = Cfg-Export, POST = Cfg-Import.
 * Symmetrisch zum HTML-Form-Handler, aber als JSON-Roundtrip nutzbar.
 *
 * GET-Output: alle Cfg-Felder per ConfigShape, Passwörter als leerer String maskiert.
 * POST-Input: JSON-Doc, Felder aus ConfigShape übernommen falls vorhanden.
 * Leer-Passwörter werden NICHT überschrieben (analog HTML-Form).
 */
void webserver_config_json()
{
	if (!webserver_request_auth())
	{
		return;
	}

	if (server.method() == HTTP_GET)
	{
		debug_outln_info(F("ws: config.json GET ..."));
		DynamicJsonDocument json(JSON_BUFFER_SIZE);
		json[F("SOFTWARE_VERSION")] = SOFTWARE_VERSION;

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
			case Config_Type_String:
				json[c.cfg_key()].set(c.cfg_val.as_str);
				break;
			case Config_Type_Password:
				json[c.cfg_key()].set("");
				break;
			}
		}

		String out;
		out.reserve(measureJson(json) + 1);
		serializeJson(json, out);
		server.send(200, FPSTR(TXT_CONTENT_TYPE_JSON), out);
		return;
	}

	if (server.method() == HTTP_POST)
	{
		debug_outln_info(F("ws: config.json POST ..."));
		String body = server.arg(F("plain"));
		if (body.length() == 0)
		{
			server.send(400, FPSTR(TXT_CONTENT_TYPE_JSON), F("{\"error\":\"empty body\"}"));
			return;
		}

		DynamicJsonDocument json(JSON_BUFFER_SIZE);
		DeserializationError err = deserializeJson(json, body);
		if (err)
		{
			String resp;
			resp.reserve(80);
			resp = F("{\"error\":\"json parse: ");
			resp += err.c_str();
			resp += F("\"}");
			server.send(400, FPSTR(TXT_CONTENT_TYPE_JSON), resp);
			return;
		}

		unsigned applied = 0;
		for (unsigned e = 0; e < sizeof(configShape) / sizeof(configShape[0]); ++e)
		{
			ConfigShapeEntry c;
			memcpy_P(&c, &configShape[e], sizeof(ConfigShapeEntry));
			JsonVariant v = json[c.cfg_key()];
			if (v.isNull())
			{
				continue;
			}

			switch (c.cfg_type)
			{
			case Config_Type_UInt:
			case Config_Type_Time:
				*(c.cfg_val.as_uint) = v.as<unsigned int>();
				++applied;
				break;
			case Config_Type_Bool:
				*(c.cfg_val.as_bool) = v.as<bool>();
				++applied;
				break;
			case Config_Type_String:
				{
					const char *s = v.as<const char *>();
					if (s)
					{
						strncpy(c.cfg_val.as_str, s, c.cfg_len);
						c.cfg_val.as_str[c.cfg_len] = '\0';
						++applied;
					}
				}
				break;
			case Config_Type_Password:
				{
					const char *s = v.as<const char *>();
					if (s && strlen(s))
					{
						strncpy(c.cfg_val.as_str, s, c.cfg_len);
						c.cfg_val.as_str[c.cfg_len] = '\0';
						++applied;
					}
				}
				break;
			}
		}

		writeConfig();
		String resp;
		resp.reserve(64);
		resp = F("{\"status\":\"saved, restarting\",\"applied\":");
		resp += applied;
		resp += F("}");
		server.send(200, FPSTR(TXT_CONTENT_TYPE_JSON), resp);
		delay(500);
		sensor_restart();
	}
}

void webserver_config()
{
	if (!webserver_request_auth())
	{
		return;
	}

	debug_outln_info(F("ws: config page ..."));

	server.sendHeader(F("Cache-Control"), F("no-cache, no-store, must-revalidate"));
	server.sendHeader(F("Pragma"), F("no-cache"));
	server.sendHeader(F("Expires"), F("0"));
	server.setContentLength(CONTENT_LENGTH_UNKNOWN);

	RESERVE_STRING(page_content, XLARGE_STR);

	start_html_page(page_content, FPSTR(INTL_CONFIGURATION));
	if (wificonfig_loop)
	{
		page_content += FPSTR(WEB_CONFIG_SCRIPT);
	}

	if (server.method() == HTTP_GET)
	{
		webserver_config_send_body_get(page_content);
	}
	else
	{
		webserver_config_send_body_post(page_content);
	}
	end_html_page(page_content);

	if (server.method() == HTTP_POST)
	{
		display_debug(F("Writing config"), emptyString);
		if (writeConfig())
		{
			display_debug(F("Writing config"), F("and restarting"));
			sensor_restart();
		}
	}
}
