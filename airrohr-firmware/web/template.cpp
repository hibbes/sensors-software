/**
 * Templating- und Form-Builder-Helper für die Web-Pages.
 *
 * Issue #18 Phase C — ausgelagert aus airrohr-firmware.ino. Sammelt die
 * HTML-/Form-/Sensor-Render-Helper, die alle Pages teilen, an einem Ort.
 */
#include "page_helpers.h"
#include <cmath>

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include <WiFi.h>
#endif

float dew_point(const float temperature, const float humidity)
{
	const float k2 = 17.62;
	const float k3 = 243.12;
	return k3 * (((k2 * temperature) / (k3 + temperature)) + log(humidity / 100.0f)) /
		   (((k2 * k3) / (k3 + temperature)) - log(humidity / 100.0f));
}

float pressure_at_sealevel(const float temperature, const float pressure)
{
	return pressure * pow(((temperature + 273.15f) /
						   (temperature + 273.15f + (0.0065f * readCorrectionOffset(cfg::height_above_sealevel)))),
						  -5.255f);
}

void start_html_page(String &page_content, const String &title)
{
	last_page_load = millis();

	RESERVE_STRING(s, LARGE_STR);
	s = FPSTR(WEB_PAGE_HEADER);
	s.replace("{t}", title);
	server.setContentLength(CONTENT_LENGTH_UNKNOWN);
	server.send(200, FPSTR(TXT_CONTENT_TYPE_TEXT_HTML), s);

	server.sendContent_P(WEB_PAGE_HEADER_HEAD);

	s = FPSTR(WEB_PAGE_HEADER_BODY);
	s.replace("{t}", title);
	if (title != " ")
	{
		s.replace("{n}", F("&raquo;"));
	}
	else
	{
		s.replace("{n}", emptyString);
	}
	s.replace("{id}", esp_chipid);
	s.replace("{macid}", esp_mac_id);
	s.replace("{mac}", WiFi.macAddress());
	page_content += s;
}

void end_html_page(String &page_content)
{
	if (page_content.length())
	{
		server.sendContent(page_content);
	}
	server.sendContent_P(WEB_PAGE_FOOTER);
}

void add_form_input(String &page_content, const ConfigShapeId cfgid,
					const __FlashStringHelper *info, const int length)
{
	RESERVE_STRING(s, MED_STR);
	s = F("<tr>"
		  "<td title='[&lt;= {l}]'>{i}:&nbsp;</td>"
		  "<td style='width:{l}em'>"
		  "<input type='{t}' name='{n}' id='{n}' placeholder='{i}' value='{v}' maxlength='{l}'/>"
		  "</td></tr>");
	String t_value;
	ConfigShapeEntry c;
	memcpy_P(&c, &configShape[cfgid], sizeof(ConfigShapeEntry));
	switch (c.cfg_type)
	{
	case Config_Type_UInt:
		t_value = String(*c.cfg_val.as_uint);
		s.replace("{t}", F("number"));
		break;
	case Config_Type_Time:
		t_value = String((*c.cfg_val.as_uint) / 1000);
		s.replace("{t}", F("number"));
		break;
	default:
		if (c.cfg_type == Config_Type_Password)
		{
			s.replace("{t}", F("password"));
			info = FPSTR(INTL_PASSWORD);
		}
		else
		{
			t_value = c.cfg_val.as_str;
			t_value.replace("'", "&#39;");
			s.replace("{t}", F("text"));
		}
	}
	s.replace("{i}", info);
	s.replace("{n}", String(c.cfg_key()));
	s.replace("{v}", t_value);
	s.replace("{l}", String(length));
	page_content += s;
}

String form_checkbox(const ConfigShapeId cfgid, const String &info, const bool linebreak)
{
	RESERVE_STRING(s, MED_STR);
	s = F("<label for='{n}'>"
		  "<input type='checkbox' name='{n}' value='1' id='{n}' {c}/>"
		  "<input type='hidden' name='{n}' value='0'/>"
		  "{i}</label><br/>");
	if (*configShape[cfgid].cfg_val.as_bool)
	{
		s.replace("{c}", F(" checked='checked'"));
	}
	else
	{
		s.replace("{c}", emptyString);
	}
	s.replace("{i}", info);
	s.replace("{n}", String(configShape[cfgid].cfg_key()));
	if (!linebreak)
	{
		s.replace("<br/>", emptyString);
	}
	return s;
}

String form_submit(const String &value)
{
	String s = F("<tr>"
				 "<td>&nbsp;</td>"
				 "<td>"
				 "<input type='submit' name='submit' value='{v}' />"
				 "</td>"
				 "</tr>");
	s.replace("{v}", value);
	return s;
}

// 31 Sprach-Optionen als statische Datentabelle.
// Vorher hardgecodet als String-Literal in einer 50-Zeilen-Funktion.
// Neue Sprache hinzufügen = ein Eintrag hier + neue intl_<lang>.h-Datei +
// platformio.ini-env.
struct LangOption
{
	const char *code;
	const char *display;
};
static const LangOption LANG_OPTIONS[] PROGMEM = {
	{"BG", "Bulgarian (BG)"},
	{"CN", "中文 (CN)"},
	{"CZ", "Český (CZ)"},
	{"DE", "Deutsch (DE)"},
	{"DK", "Dansk (DK)"},
	{"EE", "Eesti keel (EE)"},
	{"EN", "English (EN)"},
	{"ES", "Español (ES)"},
	{"FR", "Français (FR)"},
	{"GR", "Ελληνικά (GR)"},
	{"HR", "Hrvatski (HR)"},
	{"IT", "Italiano (IT)"},
	{"JP", "日本語 (JP)"},
	{"LT", "Lietuvių kalba (LT)"},
	{"LU", "Lëtzebuergesch (LU)"},
	{"LV", "Latviešu valoda (LV)"},
	{"NL", "Nederlands (NL)"},
	{"HU", "Magyar (HU)"},
	{"PL", "Polski (PL)"},
	{"PT", "Português (PT)"},
	{"BR", "Português brasileiro (BR)"},
	{"RO", "Română (RO)"},
	{"RS", "Srpski (RS)"},
	{"RU", "Русский (RU)"},
	{"SI", "Slovenščina (SI)"},
	{"SK", "Slovák (SK)"},
	{"FI", "Suomi (FI)"},
	{"SE", "Svenska (SE)"},
	{"TR", "Türkçe (TR)"},
	{"UA", "український (UA)"},
};
static constexpr size_t LANG_OPTIONS_COUNT = sizeof(LANG_OPTIONS) / sizeof(LANG_OPTIONS[0]);

String form_select_lang()
{
	String s;
	s.reserve(1200);
	s = F("<tr><td>" INTL_LANGUAGE ":&nbsp;</td><td>"
		  "<select id='current_lang' name='current_lang'>");
	const String current(cfg::current_lang);
	for (size_t i = 0; i < LANG_OPTIONS_COUNT; ++i)
	{
		LangOption entry;
		memcpy_P(&entry, &LANG_OPTIONS[i], sizeof(LangOption));
		s += F("<option value='");
		s += entry.code;
		s += F("'");
		if (current == entry.code)
		{
			s += F(" selected='selected'");
		}
		s += F(">");
		s += entry.display;
		s += F("</option>");
	}
	s += F("</select></td></tr>");
	return s;
}

void add_table_pm_value(String &page_content, const __FlashStringHelper *sensor,
						const __FlashStringHelper *param, const float value)
{
	add_table_row_from_value(page_content, sensor, param,
							 check_display_value(value, -1, 1, 0), F("µg/m³"));
}

void add_table_nc_value(String &page_content, const __FlashStringHelper *sensor,
						const __FlashStringHelper *param, const float value)
{
	add_table_row_from_value(page_content, sensor, param,
							 check_display_value(value, -1, 1, 0), F("#/cm³"));
}

void add_table_t_value(String &page_content, const __FlashStringHelper *sensor,
					   const __FlashStringHelper *param, const float value)
{
	add_table_row_from_value(page_content, sensor, param,
							 check_display_value(value, -128, 1, 0), "°C");
}

void add_table_h_value(String &page_content, const __FlashStringHelper *sensor,
					   const __FlashStringHelper *param, const float value)
{
	add_table_row_from_value(page_content, sensor, param,
							 check_display_value(value, -1, 1, 0), "%");
}

void add_warning_first_cycle(String &page_content)
{
	String s = FPSTR(INTL_TIME_TO_FIRST_MEASUREMENT);
	unsigned int time_to_first = cfg::sending_intervall_ms - msSince(starttime);
	if (time_to_first > cfg::sending_intervall_ms)
	{
		time_to_first = 0;
	}
	s.replace("{v}", String(((time_to_first + 500) / 1000)));
	page_content += s;
}

void add_age_last_values(String &s)
{
	s += "<b>";
	unsigned int time_since_last = msSince(starttime);
	if (time_since_last > cfg::sending_intervall_ms)
	{
		time_since_last = 0;
	}
	s += String((time_since_last + 500) / 1000);
	s += FPSTR(INTL_TIME_SINCE_LAST_MEASUREMENT);
	s += FPSTR(WEB_B_BR_BR);
}

bool webserver_request_auth()
{
	if (cfg::www_basicauth_enabled && !wificonfig_loop)
	{
		debug_outln_info(F("validate request auth..."));
		if (!server.authenticate(cfg::www_username, cfg::www_password))
		{
			server.requestAuthentication(BASIC_AUTH, "Sensor Login", F("Authentication failed"));
			return false;
		}
	}
	return true;
}

// Auth-Gate für state-changing/OTA-Endpunkte (Config-Write, /update).
// Strenger als webserver_request_auth(): sobald ein www_password konfiguriert
// ist, wird auch dann Authentifizierung verlangt, wenn der optionale
// www_basicauth_enabled-Toggle aus ist. So sperrt man nie ein frisches
// Gerät ohne Credentials aus (dann offen), schützt aber jedes Gerät mit
// gesetztem Passwort vor unauthentifizierten LAN-Writes/OTA-Pushes.
// Im Soft-AP-/Provisioning-Fenster (wificonfig_loop) bleibt es offen, damit
// die Erst-Einrichtung funktioniert.
bool webserver_request_write_auth()
{
	if (!wificonfig_loop && strlen(cfg::www_password) > 0)
	{
		debug_outln_info(F("validate write/OTA auth..."));
		if (!server.authenticate(cfg::www_username, cfg::www_password))
		{
			server.requestAuthentication(BASIC_AUTH, "Sensor Login", F("Authentication failed"));
			return false;
		}
	}
	return true;
}

void sendHttpRedirect()
{
	const IPAddress defaultIP(default_ip_first_octet, default_ip_second_octet,
							  default_ip_third_octet, default_ip_fourth_octet);
	String defaultAddress = F("http://192.168.4.1/config");
	server.sendHeader(F("Location"), defaultAddress);
	server.send(302, FPSTR(TXT_CONTENT_TYPE_TEXT_HTML), emptyString);
}
