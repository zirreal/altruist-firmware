#include "pages.h"
#include "../../intl.h"
#include "../html-content.h"
#include "../../utils.h"
#include <SPIFFS.h>

/*****************************************************************
 * Webserver remove config                                       *
 *****************************************************************/
void webserver_removeConfig(String &page_content, bool is_HTTP_GET) {
	debug_outln_info(F("ws: removeConfig ..."));

	if (is_HTTP_GET) {
		page_content += FPSTR(WEB_REMOVE_CONFIG_CONTENT);

	} else {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored  "-Wdeprecated-declarations"
		// Silently remove the desaster backup
		SPIFFS.remove(F("/config.json.old"));
		if (SPIFFS.exists(F("/config.json"))) {	//file exists
			debug_outln_info(F("removing config.json..."));
			if (SPIFFS.remove(F("/config.json"))) {
				page_content += F("<h3>" INTL_CONFIG_DELETED ".</h3>");
			} else {
				page_content += F("<h3>" INTL_CONFIG_CAN_NOT_BE_DELETED ".</h3>");
			}
		} else {
			page_content += F("<h3>" INTL_CONFIG_NOT_FOUND ".</h3>");
		}
#pragma GCC diagnostic pop
	}
}