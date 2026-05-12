/**
 * /reset Handler — Factory-Reset-Dialog + sensor_restart().
 *
 * Issue #18 Phase B — ausgelagert aus airrohr-firmware.ino.
 */
#include "../page_helpers.h"
#include "../../utils.h"
#include "../../intl.h"

void webserver_reset()
{
	if (!webserver_request_auth())
	{
		return;
	}

	String page_content;
	page_content.reserve(512);

	start_html_page(page_content, FPSTR(INTL_RESTART_SENSOR));
	debug_outln_info(F("ws: reset ..."));

	if (server.method() == HTTP_GET)
	{
		page_content += FPSTR(WEB_RESET_CONTENT);
	}
	else
	{
		sensor_restart();
	}
	end_html_page(page_content);
}
