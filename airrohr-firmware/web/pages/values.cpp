/**
 * /values Handler — aktuelle Sensor-Lesewerte als HTML-Tabelle.
 *
 * Issue #18 Phase B/E — Pages-Module + Sensor-Registry-Pattern.
 * Pro Sensor rendert sensors/<name>.cpp seine eigene Tabellen-Sektion;
 * /values iteriert nur noch über die cfg-Flags und ruft die Renderer auf.
 */
#include "../page_helpers.h"
#include "../../sensors/ppd42ns.h"
#include "../../sensors/sds011.h"
#include "../../sensors/pms.h"
#include "../../sensors/hpm.h"
#include "../../sensors/npm.h"
#include "../../sensors/ips.h"
#include "../../sensors/sps30.h"
#include "../../sensors/dht22.h"
#include "../../sensors/htu21d.h"
#include "../../sensors/aht20.h"
#include "../../sensors/sht3x.h"
#include "../../sensors/scd30.h"
#include "../../sensors/ds18b20.h"
#include "../../sensors/dnms.h"
#include "../../sensors/bmp180.h"
#include "../../sensors/bmx280.h"
#include "../../sensors/gps.h"

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

	server.sendContent(page_content);
	page_content = F("<table cellspacing='0' cellpadding='5' class='v'>\n"
					 "<thead><tr><th>" INTL_SENSOR "</th><th> " INTL_PARAMETER "</th><th>" INTL_VALUE "</th></tr></thead>");

	if (cfg::ppd_read)    render_ppd_values(page_content);
	if (cfg::sds_read)    render_sds011_values(page_content);
	if (cfg::pms_read)    render_pms_values(page_content);
	if (cfg::hpm_read)    render_hpm_values(page_content);
	if (cfg::npm_read)    render_npm_values(page_content);
	if (cfg::ips_read)    render_ips_values(page_content);
	if (cfg::sps30_read)  render_sps30_values(page_content);
	if (cfg::dht_read)    render_dht_values(page_content);
	if (cfg::htu21d_read) render_htu21d_values(page_content);
	if (cfg::aht20_read)  render_aht20_values(page_content);
	if (cfg::bmp_read)    render_bmp180_values(page_content);
	if (cfg::bmx280_read) render_bmx280_values(page_content);
	if (cfg::sht3x_read)  render_sht3x_values(page_content);
	if (cfg::scd30_read)  render_scd30_values(page_content);
	if (cfg::ds18b20_read) render_ds18b20_values(page_content);
	if (cfg::dnms_read)   render_dnms_values(page_content);
	if (cfg::gps_read)    render_gps_values(page_content);

	server.sendContent(page_content);
	page_content = emptyString;

	add_table_row_from_value(page_content, F("WiFi"), FPSTR(INTL_SIGNAL_STRENGTH),
							 String(last_signal_strength), "dBm");
	add_table_row_from_value(page_content, F("WiFi"), FPSTR(INTL_SIGNAL_QUALITY),
							 String(signal_quality), "%");

	page_content += FPSTR(TABLE_TAG_CLOSE_BR);
	page_content += FPSTR(BR_TAG);
	end_html_page(page_content);
}
