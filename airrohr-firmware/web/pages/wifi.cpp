/**
 * /wifi Handler — WLAN-Scan-Ergebnis als HTML-Tabelle.
 *
 * Issue #18 Phase B — ausgelagert aus airrohr-firmware.ino.
 */
#include "../page_helpers.h"
#include <memory>

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include <WiFi.h>
#endif

void webserver_wifi()
{
	String page_content;

	debug_outln_info(F("wifi networks found: "), String(count_wifiInfo));
	if (count_wifiInfo == 0)
	{
		page_content += FPSTR(BR_TAG);
		page_content += FPSTR(INTL_NO_NETWORKS);
		page_content += FPSTR(BR_TAG);
	}
	else
	{
		std::unique_ptr<int[]> indices(new int[count_wifiInfo]);
		debug_outln_info(F("ws: wifi ..."));
		for (unsigned i = 0; i < count_wifiInfo; ++i)
		{
			indices[i] = i;
		}
		for (unsigned i = 0; i < count_wifiInfo; i++)
		{
			for (unsigned j = i + 1; j < count_wifiInfo; j++)
			{
				if (wifiInfo[indices[j]].RSSI > wifiInfo[indices[i]].RSSI)
				{
					std::swap(indices[i], indices[j]);
				}
			}
		}
		int duplicateSsids = 0;
		for (int i = 0; i < count_wifiInfo; i++)
		{
			if (indices[i] == -1)
			{
				continue;
			}
			for (int j = i + 1; j < count_wifiInfo; j++)
			{
				if (strncmp(wifiInfo[indices[i]].ssid, wifiInfo[indices[j]].ssid, sizeof(wifiInfo[0].ssid)) == 0)
				{
					indices[j] = -1;
					++duplicateSsids;
				}
			}
		}

		page_content += FPSTR(INTL_NETWORKS_FOUND);
		page_content += String(count_wifiInfo - duplicateSsids);
		page_content += FPSTR(BR_TAG);
		page_content += FPSTR(BR_TAG);
		page_content += FPSTR(TABLE_TAG_OPEN);
		for (int i = 0; i < count_wifiInfo; ++i)
		{
			if (indices[i] == -1
#if defined(ESP8266)
				|| wifiInfo[indices[i]].isHidden
#endif
			)
			{
				continue;
			}
#if defined(ESP8266)
			page_content += wlan_ssid_to_table_row(wifiInfo[indices[i]].ssid, ((wifiInfo[indices[i]].encryptionType == ENC_TYPE_NONE) ? " " : u8"🔒"), wifiInfo[indices[i]].RSSI);
#endif
#if defined(ESP32)
			page_content += wlan_ssid_to_table_row(wifiInfo[indices[i]].ssid, ((wifiInfo[indices[i]].encryptionType == WIFI_AUTH_OPEN) ? " " : u8"🔒"), wifiInfo[indices[i]].RSSI);
#endif
		}
		page_content += FPSTR(TABLE_TAG_CLOSE_BR);
		page_content += FPSTR(BR_TAG);
	}
	server.send(200, FPSTR(TXT_CONTENT_TYPE_TEXT_HTML), page_content);
}
