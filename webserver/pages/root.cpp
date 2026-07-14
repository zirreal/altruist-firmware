#include "pages.h"
#include "../../intl.h"
#include "../html-content.h"
#include "../../config_manager/config_helpers.h"
#include "../../utils.h"
#include "../utils.h"
#include "../../defines.h"
#include <WiFi.h>

namespace {

void append_api_status_pill(String& page, const device_status_t& status, const char* api_name,
                            const __FlashStringHelper* ok_label, const __FlashStringHelper* err_label) {
	const auto api_it = status.apis_status.find(api_name);
	if (api_it == status.apis_status.end()) {
		return;
	}
	page += F("<span class='status-pill status-pill--");
	page += api_it->second.is_ok ? F("ok") : F("warn");
	page += F("'>");
	page += api_it->second.is_ok ? ok_label : err_label;
	page += F("</span>");
}

void append_dashboard_group_start(String& page, const char* group_id, const __FlashStringHelper* title,
                                  const __FlashStringHelper* intro) {
	page += F("<section class='dash-group dash-group--");
	page += group_id;
	page += F("'><header class='dash-group__head'><h3 class='dash-group__title'>");
	page += title;
	page += F("</h3><p class='dash-group__intro'>");
	page += intro;
	page += F("</p></header><div class='dash-group__list'>");
}

void append_dashboard_group_end(String& page) {
	page += F("</div></section>");
}

void append_dashboard_row(String& page, const char* href, const __FlashStringHelper* label,
                          const __FlashStringHelper* desc, bool external = false, const char* tone = nullptr) {
	page += F("<a class='dash-row");
	if (external) {
		page += F(" dash-row--external");
	}
	if (tone != nullptr) {
		page += F(" dash-row--");
		page += tone;
	}
	page += F("' href='");
	page += href;
	page += F("'");
	if (external) {
		page += F(" target='_blank' rel='noreferrer'");
	}
	page += F("><span class='dash-row__body'><span class='dash-row__label'>");
	page += label;
	page += F("</span><span class='dash-row__desc'>");
	page += desc;
	page += F("</span></span><span class='dash-row__chev' aria-hidden='true'>");
	page += external ? F("&#8599;") : F("&#8250;");
	page += F("</span></a>");
}

} // namespace

void webserver_root(String& page_content, const String& robonomics_address, device_status_t& status) {
	page_content += F("<div class='dashboard'>");

	page_content += F("<div class='dashboard-health'><p class='dashboard-health__title'>");
	page_content += F(INTL_DASH_HEALTH_TITLE);
	page_content += F("</p><div class='dashboard-health__pills'>");
	const bool wifi_ok = WiFi.status() == WL_CONNECTED;
	page_content += F("<span class='status-pill status-pill--");
	page_content += wifi_ok ? F("ok") : F("warn");
	page_content += F("'>");
	page_content += wifi_ok ? F(INTL_DASH_WIFI_OK) : F(INTL_DASH_WIFI_OFF);
	page_content += F("</span>");
	if (wifi_ok) {
		const int signal_quality = calcWiFiSignalQuality(WiFi.RSSI());
		page_content += F("<span class='dashboard-health__item'>");
		page_content += String(signal_quality);
		page_content += F("%</span>");
	}
	page_content += F("<span class='dashboard-health__item'>");
	page_content += FPSTR(INTL_UPTIME);
	page_content += F(": ");
	page_content += delayToString(millis() - status.time_point_device_start_ms);
	page_content += F("</span>");

	append_api_status_pill(page_content, status, "Robonomics Datalog", F(INTL_DASH_DATALOG_OK), F(INTL_DASH_DATALOG_ERR));
	append_api_status_pill(page_content, status, "Robonomics Map", F(INTL_DASH_MAP_OK), F(INTL_DASH_MAP_ERR));
	page_content += F("</div></div>");

	append_dashboard_group_start(page_content, "monitor", F(INTL_NAV_MONITOR), F(INTL_DASH_SECTION_MONITOR_INTRO));
	append_dashboard_row(page_content, "/values", F(INTL_NAV_READINGS), F(INTL_DASH_READINGS_DESC));
	append_dashboard_row(page_content, "/status", F(INTL_NAV_STATUS), F(INTL_DASH_STATUS_DESC));
#if !defined(ALTRUIST_URBAN_C3_LITE)
	const char* sensor_ss58 = (robonomics_address.length() > 0) ? robonomics_address.c_str() : nullptr;
	const String map_url = buildSensorsSocialMapUrl(sensor_ss58);
	append_dashboard_row(page_content, map_url.c_str(), FPSTR(INTL_ACTIVE_SENSORS_MAP), F(INTL_DASH_MAP_DESC), true);
#endif
	append_dashboard_group_end(page_content);

	append_dashboard_group_start(page_content, "settings", F(INTL_NAV_SETTINGS), F(INTL_DASH_SECTION_SETTINGS_INTRO));
	append_dashboard_row(page_content, "/config", FPSTR(INTL_CONFIGURATION), F(INTL_DASH_CONFIG_DESC));
	append_dashboard_row(page_content, "/group", FPSTR(INTL_GROUP_MENU), F(INTL_DASH_GROUP_DESC));
	append_dashboard_row(page_content, "/ota", FPSTR(INTL_OTA_UPDATE), F(INTL_DASH_OTA_DESC));
#ifdef ALTRUIST_INSIGHT
	append_dashboard_row(page_content, "/screen", FPSTR(INTL_SCREEN_MENU), F(INTL_DASH_SCREEN_DESC));
#endif
	append_dashboard_group_end(page_content);

	append_dashboard_group_start(page_content, "maintenance", F(INTL_NAV_MAINTENANCE), F(INTL_DASH_SECTION_MAINTENANCE_INTRO));
	append_dashboard_row(page_content, "/debug", FPSTR(INTL_DEBUG_LEVEL), F(INTL_DASH_DEBUG_DESC));
	append_dashboard_row(page_content, "/restart", FPSTR(INTL_RESTART_SENSOR), F(INTL_DASH_RESTART_DESC));
	append_dashboard_row(page_content, "/removeConfig", F(INTL_CONFIGURATION_DELETE), F(INTL_DASH_DELETE_CONFIG_DESC), false, "danger");
	append_dashboard_group_end(page_content);

	page_content += F("</div>");
}
