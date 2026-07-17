#include "webserver.h"
#include "pages/pages.h"
#include "html-content.h"
#include <WiFi.h>
#if defined(ALTRUIST_URBAN_C3_LITE)
#include "script-js-lite.h"
#else
#include "script-js.h"
#endif
#include "utils.h"
#include "../config_manager/config_helpers.h"
#include "../apis/rws_group.h"
#include "../defines.h"
#include "../utils.h"
#include "../wifi_manager.h"
#include <Robonomics.h>
#include "web-header-logo-select.h"
#include "favicon.h"
#include "nav-icons.h"

extern Robonomics robonomics;

#ifdef ALTRUIST_INSIGHT
#include <ESPmDNS.h>
#include "display/display_manager.h"
extern DisplayManager displayManager;

static unsigned long s_insight_guest_finish_deadline_ms = 0;

static void insightGuestApplyStandaloneAndRestart() {
	cfg::standalone = true;
	cfg::use_custom_urban = false;
	cfg::chosen_altruist_urban[0] = '\0';
	cfg::custom_altruist_urban[0] = '\0';
	cfgApplyStandaloneModeEnabled();
	if (writeConfig()) {
		set_restart_reason(RESTART_REASON_CONFIG);
		sensor_restart();
	}
}

void insightGuestMarkFinishPending(void) {
	s_insight_guest_finish_deadline_ms = millis() + INSIGHT_GUEST_AUTO_FINISH_MS;
}

void insightGuestClearFinishPending(void) {
	s_insight_guest_finish_deadline_ms = 0;
}

void insightGuestProcessPendingFinish(void) {
	if (s_insight_guest_finish_deadline_ms == 0) {
		return;
	}
	if ((long)(millis() - s_insight_guest_finish_deadline_ms) < 0) {
		return;
	}
	if (!wifiGuestPortalStaReady()) {
		return;
	}
	s_insight_guest_finish_deadline_ms = 0;
	debug_outln_info(F("Insight guest setup: auto-finish (standalone)"));
	insightGuestApplyStandaloneAndRestart();
}
#endif

static SemaphoreHandle_t s_webserver_mutex = nullptr;

static bool webserverLock(uint32_t timeout_ms = 250) {
	if (!s_webserver_mutex) {
		s_webserver_mutex = xSemaphoreCreateMutex();
	}
	if (!s_webserver_mutex) {
		return false;
	}
	return xSemaphoreTake(s_webserver_mutex, pdMS_TO_TICKS(timeout_ms)) == pdTRUE;
}

static void webserverUnlock() {
	if (s_webserver_mutex) {
		xSemaphoreGive(s_webserver_mutex);
	}
}

void SensorWebServer::handleClient() {
	// Guard against races between loop() and the captive portal worker.
	// Retry briefly: a single 250ms miss drops phone requests (HTML + CSS + logo in parallel).
	for (uint8_t attempt = 0; attempt < 4; ++attempt) {
		if (!webserverLock(attempt == 0 ? 250 : 100)) {
			yield();
			continue;
		}
		for (uint8_t i = 0; i < 4; ++i) {
			server.handleClient();
			markMainLoopAlive();
			yield();
		}
		webserverUnlock();
		return;
	}
}


void SensorWebServer::setup() {
    www_username = cfg::www_username;
    www_password = cfg::www_password;
    uint64_t chipid_num;
	esp_chipid = get_chipid();

	server.on("/guest", std::bind(&SensorWebServer::_webserver_guest, this)); // x
	server.on("/", std::bind(&SensorWebServer::_webserver_hub_local, this)); // x
	server.on(F("/local"), std::bind(&SensorWebServer::_webserver_hub_local, this));
	server.on(F("/social"), std::bind(&SensorWebServer::_webserver_hub_social, this));
	server.on(F("/custom"), std::bind(&SensorWebServer::_webserver_hub_custom, this));
	server.on(F("/advanced"), std::bind(&SensorWebServer::_webserver_hub_advanced, this));
	server.on(F("/warnings"), std::bind(&SensorWebServer::_webserver_hub_warnings_redirect, this));
	server.on(F("/config"), std::bind(&SensorWebServer::_webserver_config, this)); // x
	server.on(F("/wifi"), std::bind(&SensorWebServer::_webserver_wifi, this)); // x
	server.on(F("/values"), std::bind(&SensorWebServer::_webserver_values, this)); // x
	server.on(F("/status"), std::bind(&SensorWebServer::_webserver_status, this));
	server.on(F("/generate_204"), std::bind(&SensorWebServer::_webserver_config, this)); // x
	server.on(F("/fwlink"), std::bind(&SensorWebServer::_webserver_config, this)); // x
	server.on(F("/debug"), std::bind(&SensorWebServer::_webserver_debug_level, this)); // x
	server.on(F("/serial"), std::bind(&SensorWebServer::_webserver_serial, this)); // x
	server.on(F("/removeConfig"), std::bind(&SensorWebServer::_webserver_removeConfig, this)); // x
	server.on(F("/restart"), std::bind(&SensorWebServer::_webserver_restart, this)); // x
	server.on(F("/data.json"), std::bind(&SensorWebServer::_webserver_data_json, this)); // x
	server.on(F("/favicon.ico"), std::bind(&SensorWebServer::_webserver_favicon, this)); // x
	server.on(F("/favicon-dark.ico"), std::bind(&SensorWebServer::_webserver_favicon_dark, this)); // x
	server.on(F(STATIC_PREFIX), std::bind(&SensorWebServer::_webserver_static, this)); // x
	server.on(F("/ota"), std::bind(&SensorWebServer::_webserver_ota, this));
	server.on(F("/group"), std::bind(&SensorWebServer::_webserver_group, this));
#ifdef ALTRUIST_INSIGHT
	server.on(F("/screen"), std::bind(&SensorWebServer::_webserver_screen, this));
	server.on(F("/select_urban"), std::bind(&SensorWebServer::_webserver_select_urban, this));
	server.on(F("/scan_urbans"), std::bind(&SensorWebServer::_webserver_scan_urbans, this));
	server.on(F("/guest_setup_ack"), [this]() {
		insightGuestClearFinishPending();
		server.send(204, FPSTR(TXT_CONTENT_TYPE_TEXT_PLAIN), emptyString);
	});
#endif
	server.onNotFound(std::bind(&SensorWebServer::_webserver_not_found, this)); // x

	debug_outln_info(F("Starting Webserver... "), WiFi.localIP().toString());
	server.begin();
}

void SensorWebServer::notifyStaIpRestored() {
#if defined(ESP32)
	// WebServer::begin() calls close() internally, but after a netif/IP change an explicit stop + short yield
	// avoids a stuck listen socket so phones/browsers can reach the device again on the LAN IP.
	debug_outln_info(F("Webserver: rebind after STA IP "), WiFi.localIP().toString());
	if (!webserverLock(2000)) {
		debug_outln_error(F("[WiFi] Webserver rebind skipped: mutex busy"));
		return;
	}
	server.stop();
	yield();
	delay(30);
	server.begin();
	webserverUnlock();
#endif
}

void SensorWebServer::_webserver_status() {
	if (WiFi.status() != WL_CONNECTED) {
		sendHttpRedirectGuest();
		return;
	}

	RESERVE_STRING(page_content, XLARGE_STR);
	start_html_page(page_content, FPSTR(INTL_DEVICE_STATUS), false, "status");

	debug_outln_info(F("ws: status ..."));
    webserver_status_part1(page_content, deviceStatus, server);
    web_page_flush_chunk(page_content, &server);
    webserver_status_part2(page_content, deviceStatus, server);

	page_content += F("</div></div>");
	end_html_page_app(page_content);
}

void SensorWebServer::_webserver_data_json() {
	String json_content;
	if (!xSemaphoreTake(mutex, pdMS_TO_TICKS(300))) {
		server.send(503, FPSTR(TXT_CONTENT_TYPE_JSON), F("{\"error\":\"busy\"}"));
		return;
	}
	webserver_data_json(sensors_data, esp_chipid, json_content);
	xSemaphoreGive(mutex);
    server.send(200, FPSTR(TXT_CONTENT_TYPE_JSON), json_content);
}

void SensorWebServer::_webserver_not_found() {
	debug_outln_info(F("ws: not found ..."));
	if (WiFi.status() != WL_CONNECTED) {
		if ((server.uri().indexOf(F("success.html")) != -1) || (server.uri().indexOf(F("detect.html")) != -1)) {
			server.send(200, FPSTR(TXT_CONTENT_TYPE_TEXT_HTML), FPSTR(WEB_IOS_REDIRECT));
		} else {
			sendHttpRedirectGuest();
		}
	} else {
		server.send(404, FPSTR(TXT_CONTENT_TYPE_TEXT_PLAIN), F("Not found."));
	}
}

void SensorWebServer::_webserver_static() {
	if (server.arg(String('r')) == F("logo")) {
		// Device-specific SVG (Insight vs Urban build); avoid stale PNG/SVG from old Cache-Control.
		server.sendHeader(F("Cache-Control"), F("no-cache"));
		server.send_P(200, TXT_CONTENT_TYPE_IMAGE_SVG, WEB_HEADER_LOGO_SVG, WEB_HEADER_LOGO_SIZE);
		return;
	}
	const String resource = server.arg(String('r'));
	if (resource == F("nav-local")) {
		server.sendHeader(F("Cache-Control"), F("max-age=2592000, public"));
		server.send_P(200, TXT_CONTENT_TYPE_IMAGE_PNG, WEB_NAV_ICON_LOCAL_PNG, WEB_NAV_ICON_LOCAL_PNG_SIZE);
		return;
	}
	if (resource == F("nav-map")) {
		server.sendHeader(F("Cache-Control"), F("max-age=2592000, public"));
		server.send_P(200, TXT_CONTENT_TYPE_IMAGE_PNG, WEB_NAV_ICON_MAP_PNG, WEB_NAV_ICON_MAP_PNG_SIZE);
		return;
	}
	if (resource == F("nav-custom")) {
		server.sendHeader(F("Cache-Control"), F("max-age=2592000, public"));
		server.send_P(200, TXT_CONTENT_TYPE_IMAGE_PNG, WEB_NAV_ICON_CUSTOM_PNG, WEB_NAV_ICON_CUSTOM_PNG_SIZE);
		return;
	}
	if (resource == F("nav-system")) {
		server.sendHeader(F("Cache-Control"), F("max-age=2592000, public"));
		server.send_P(200, TXT_CONTENT_TYPE_IMAGE_PNG, WEB_NAV_ICON_SYSTEM_PNG, WEB_NAV_ICON_SYSTEM_PNG_SIZE);
		return;
	}
	server.sendHeader(F("Cache-Control"), F("max-age=2592000, public"));

	if (server.arg(String('r')) == F("css")) {
		server.send_P(200, TXT_CONTENT_TYPE_TEXT_CSS,
			WEB_PAGE_STATIC_CSS, sizeof(WEB_PAGE_STATIC_CSS)-1);
	} else if (server.arg(String('r')) == F("js")) {
		server.send_P(200, TXT_CONTENT_TYPE_TEXT_JS,
			WEB_PAGE_STATIC_JS_CONFIG, sizeof(WEB_PAGE_STATIC_JS_CONFIG)-1);
	} else {
		_webserver_not_found();
	}
}

void SensorWebServer::_webserver_favicon() {
	server.sendHeader(F("Cache-Control"), F("max-age=86400, public"));
	server.send_P(200, TXT_CONTENT_TYPE_IMAGE_PNG, WEB_FAVICON_PNG, WEB_FAVICON_PNG_SIZE);
}

void SensorWebServer::_webserver_favicon_dark() {
	server.sendHeader(F("Cache-Control"), F("max-age=86400, public"));
	server.send_P(200, TXT_CONTENT_TYPE_IMAGE_PNG, WEB_FAVICON_DARK_PNG, WEB_FAVICON_DARK_PNG_SIZE);
}

void SensorWebServer::_webserver_restart() {
    if (!webserver_request_auth())
	{ return; }

	String page_content;
	page_content.reserve(512);

	start_html_page(page_content, FPSTR(INTL_RESTART_SENSOR), false, "settings");
	debug_outln_info(F("ws: reset ..."));

	append_app_page_body_start(page_content, F(INTL_PAGE_RESTART_INTRO));

	if (server.method() == HTTP_GET) {
		page_content += F("<section class='app-panel app-panel--confirm'>");
		page_content += FPSTR(WEB_RESET_CONTENT);
		page_content += F("</section>");
	} else {
		set_restart_reason(RESTART_REASON_USER);
		sensor_restart();
	}
	append_app_page_body_end(page_content);
	end_html_page_app(page_content);
}

void SensorWebServer::_webserver_removeConfig() {
    if (!webserver_request_auth())
	{ return; }

	RESERVE_STRING(page_content, LARGE_STR);
	start_html_page(page_content, String(F(INTL_CONFIGURATION_DELETE)), false, "settings");
	append_app_page_body_start(page_content, F(INTL_PAGE_DELETE_CONFIG_INTRO));
    bool is_HTTP_GET = server.method() == HTTP_GET;
	bool remove_all = false;
	if (server.hasArg("configType")) {
		const String server_arg(server.arg("configType"));
		remove_all = server_arg == "all";
	}
    webserver_removeConfig(page_content, is_HTTP_GET, remove_all);
	append_app_page_body_end(page_content);
    end_html_page_app(page_content);
    if (!is_HTTP_GET) {
        esp_restart();
    }
}

void SensorWebServer::_webserver_serial() {
    String s(Debug.popLines());

	server.send(s.length() ? 200 : 204, FPSTR(TXT_CONTENT_TYPE_TEXT_PLAIN), s);
}

void SensorWebServer::_webserver_wifi() {
    String page_content;
    webserver_wifi(wifiInfo, wifiInfoCount, page_content);
    server.send(200, FPSTR(TXT_CONTENT_TYPE_TEXT_HTML), page_content);
}

void SensorWebServer::_webserver_debug_level() {
    if (!webserver_request_auth())
	{ return; }

	RESERVE_STRING(page_content, LARGE_STR);
	start_html_page(page_content, FPSTR(INTL_DEBUG_LEVEL), false, "settings");
	append_app_page_body_start(page_content, F(INTL_PAGE_DEBUG_INTRO));
    webserver_debug_level(server, page_content);
	append_app_page_body_end(page_content);
    end_html_page_app(page_content);
}

void SensorWebServer::_webserver_group() {
	if (WiFi.status() != WL_CONNECTED) {
		sendHttpRedirectGuest();
		return;
	}
	if (!webserver_request_auth()) {
		return;
	}

	RESERVE_STRING(page_content, LARGE_STR);
	start_html_page(page_content, FPSTR(INTL_GROUP_MENU), false, "settings");

	const String self_ss58 = String(robonomics.getSs58Address());
	setRobonomicsAddress(self_ss58);
	rwsSyncGroupModeFromOwner(self_ss58);

	RwsGroupApplyResult save_result = RwsGroupApply_None;
	if (server.method() == HTTP_POST) {
		save_result = webserver_group_post(server, self_ss58);
	}

	webserver_group_page(page_content, self_ss58, &robonomics, save_result);
	end_html_page_app(page_content);
}

#ifdef ALTRUIST_INSIGHT
void SensorWebServer::_webserver_screen() {
	if (WiFi.status() != WL_CONNECTED) {
		sendHttpRedirectGuest();
		return;
	}
	if (!webserver_request_auth()) {
		return;
	}

	RESERVE_STRING(page_content, LARGE_STR);
	start_html_page(page_content, FPSTR(INTL_SCREEN_MENU), false, "settings");

	ScreenSaveResult save_result = ScreenSave_None;
	if (server.method() == HTTP_POST) {
		save_result = webserver_screen_post(server);
	}

	webserver_screen_page(page_content, save_result);
	end_html_page_app(page_content);
}
#endif

void SensorWebServer::_webserver_values() {
    if (WiFi.status() != WL_CONNECTED) {
		sendHttpRedirectGuest();
		return;
	}
    if (!webserver_request_auth())
		{ return; }
    RESERVE_STRING(page_content, XLARGE_STR);
    start_html_page(page_content, FPSTR(INTL_CURRENT_DATA), false, "values");
	DynamicJsonDocument values_snapshot(sensors_data.capacity());
	if (!xSemaphoreTake(mutex, pdMS_TO_TICKS(500))) {
		page_content = F("<p class='data-busy-msg'>");
		page_content += FPSTR(INTL_DATA_BUSY);
		page_content += F("</p>");
	} else {
		values_snapshot.set(sensors_data.as<JsonVariantConst>());
		xSemaphoreGive(mutex);
		if (values_snapshot.overflowed()) {
			page_content = F("<p class='data-busy-msg'>");
			page_content += FPSTR(INTL_DATA_BUSY);
			page_content += F("</p>");
		} else {
			webserver_values(values_snapshot, page_content, server);
		}
	}
    end_html_page_app(page_content);
}

void SensorWebServer::_webserver_guest() {
    server.sendHeader(F("Cache-Control"), F("no-cache, no-store, must-revalidate"));
	server.sendHeader(F("Pragma"), F("no-cache"));
    server.sendHeader(F("Expires"), F("0"));

	if (server.method() == HTTP_POST) {
		debug_outln_info(F("ws: guest POST ..."));
		webserver_config_send_body_post(server);

		stream_html_page_head(FPSTR(INTL_CONFIGURATION), true);
		server.sendContent_P(WEB_GUEST_CONNECT_STATUS);
		yield();

#if defined(ESP32) || defined(ESP8266)
		wifiGuestPortalPrepareStaJoin();
#endif
		if (cfg::wlannopwd) {
			debug_outln_info(F("No password"));
			WiFi.begin(cfg::wlanssid);
		} else {
			WiFi.begin(cfg::wlanssid, cfg::wlanpwd);
		}

		int counter = 0;
		while (!wifiGuestPortalStaReady()) {
			// Fail-fast: don't keep user stuck on "Connecting..." for too long.
			// 40 * 500ms = ~20s.
			if (counter > 40) {
				break;
			}
			yield();
			delay(500);
			counter++;
		}

		RESERVE_STRING(page_content, XLARGE_STR);

		if (wifiGuestPortalStaReady()) {
			String address = WiFi.localIP().toString();
			debug_outln_info(F("Connected to WiFi network: "), cfg::wlanssid);
			debug_outln_info(F("STA IP: "), address);
			page_content = F("<script>document.querySelector('.guest__connect-status--initial').classList.add('hide');</script>");
#ifdef ALTRUIST_INSIDE
			const unsigned insightAutoSec = (unsigned)(INSIGHT_GUEST_AUTO_FINISH_MS / 1000UL);
			page_content += F("<div class='guest-page'><div class='guest-card guest__setup-finish'>");
			page_content += F("<div class='guest__setup-header'>"
				"<span class='guest__step-label'>" INTL_GUEST_SETUP_STEP_2_LABEL "</span>"
				"<h2 class='guest__step-title'>" INTL_GUEST_WIFI_STEP_TITLE "</h2>"
				"</div>");
			page_content += F("<p class='form-hint guest-hint'>" INTL_GUEST_INSIGHT_FINISH_HINT "</p>");
			page_content += F("<p class='form-hint'>" INTL_GUEST_KEEP_OPEN_HINT "</p>");
			page_content += F("<div class='guest__reboot guest__reboot--ip'>" INTL_GUEST_IP_ADDRESS
				" <span class='ip-address guest-ip'>");
			page_content += address;
			page_content += F("</span> <button class='copy-btn' onclick='copyText()'></button></div>");
			page_content += F("<p class='form-hint' id='insight-auto-finish-hint'>"
				INTL_GUEST_INSIGHT_AUTO_FINISH_HINT " <strong id='insight-auto-sec'>");
			page_content += String(insightAutoSec);
			page_content += F("</strong> " INTL_GUEST_INSIGHT_AUTO_FINISH_SUFFIX "</p>");
			page_content += FPSTR(WEB_COPY_IP_JS);

			if (!writeConfig()) {
				page_content += F("<p class='guest__reboot error'>Failed to save configuration.</p></div></div>");
				end_html_page_guest(page_content);
				return;
			}
			insightGuestMarkFinishPending();

			page_content += F("<p class='form-hint'>" INTL_SETUP_INSIGHT_MODE_HINT "</p>");
			page_content += F("<form id='insight-finish-form' class='guest-wizard-form' method='POST' action='/select_urban'>"
				"<label class='guest-option'>"
				"<input type='checkbox' name='pair_with_urban' value='1'>"
				"<span>" INTL_SETUP_PAIR_WITH_URBAN "</span>"
				"</label>"
				"<button type='submit' class='submit-btn guest__setup-finish-btn'>");
			page_content += F(INTL_SETUP_CONTINUE);
			page_content += F("</button></form></div></div>");
			page_content += F("<script>(function(){var s=");
			page_content += String(insightAutoSec);
			page_content += F(",el=document.getElementById('insight-auto-sec'),form=document.getElementById('insight-finish-form');"
				"function tick(){if(s>0){if(el)el.textContent=String(s);s--;}else if(form)form.submit();}"
				"tick();setInterval(tick,1000);})();</script>");
			page_content += FPSTR(WEB_GUEST_WIZARD_SUBMIT_JS);
			end_html_page_guest(page_content);
			return;
#else
			page_content += F("<div class='guest-page'><div class='guest-card'>");
			page_content += F("<div class='guest__connected'><h2 class='guest__connect-title'>" INTL_GUEST_CONNECTED "</h2></div>");
			page_content += F("<div class='guest__reboot guest__reboot--ip'>" INTL_GUEST_IP_ADDRESS " <span class='ip-address guest-ip'>");
			page_content += address;
			page_content += F("</span> <button class='copy-btn' onclick='copyText()'></button></div>");
			page_content += F("<p class='form-hint'>" INTL_GUEST_OPEN_IP_HINT "</p>");
			page_content += FPSTR(WEB_COPY_IP_JS);

			if (!writeConfig()) {
				page_content += F("<p class='guest__reboot error'>Failed to save configuration.</p></div></div>");
				end_html_page_guest(page_content);
				return;
			}
			page_content += F("<p class='form-hint' id='guest-restart-hint'>");
			page_content += FPSTR(INTL_GUEST_RESTART_PAUSE_HINT);
			page_content += F("</p></div></div><script>(function(){var s=");
			page_content += String((unsigned)(GUEST_SUCCESS_PAGE_DELAY_MS / 1000UL));
			page_content += F(",el=document.getElementById('guest-restart-hint'),base=");
			page_content += F("'");
			page_content += FPSTR(INTL_GUEST_RESTART_PAUSE_HINT);
			page_content += F("';function tick(){if(!el)return;if(s>0){el.textContent=base+' ('+s+'s)';s--;}else{clearInterval(iv);}}tick();var iv=setInterval(tick,1000);})();</script>");
			end_html_page_guest(page_content);
			Serial.flush();
			delay(GUEST_SUCCESS_PAGE_DELAY_MS);
			wifiCaptivePortalRestartAfterSuccess();
			return;
#endif
		}

		page_content = F("<script>document.querySelector('.guest__connect-status--initial').classList.add('hide');</script>");
		page_content += F("<h2 class='guest__connect-subtitle error'>Connection Failed</h2>"
						"<p class='guest__reboot'>Failed to connect to: ");
		page_content += cfg::wlanssid;
		page_content += F("</p>");
#ifdef ALTRUIST_INSIDE
		page_content += F("<p class='guest__reboot'>Rebooting to WiFi setup… You can close this page and try again.</p>");
#else
		page_content += F("<p class='guest__reboot'>Check SSID and password, then try again.</p>");
#endif
		end_html_page_guest(page_content);
#ifdef ALTRUIST_INSIDE
		if (writeConfig()) {
			set_restart_reason(RESTART_REASON_CONFIG);
			sensor_restart();
		}
#endif
		return;
	}

	RESERVE_STRING(page_content, XLARGE_STR);

	start_html_page(page_content, FPSTR(INTL_CONFIGURATION), true);
    debug_outln_info(F("ws: guest GET ..."));

	if (wificonfig_loop) {  // scan for wlan ssids
		page_content += FPSTR(WEB_CONFIG_SCRIPT);
	}

	webserver_guest_create_body_get_part1(page_content, wificonfig_loop, deviceStatus);
	web_page_flush_chunk(page_content, &server);
	webserver_guest_create_body_get_part2(page_content, wificonfig_loop);
	web_page_flush_chunk(page_content, &server);
	end_html_page_guest(page_content);
}

void SensorWebServer::setWifiInfo(struct_wifiInfo* info, uint8_t count) {
    wifiInfo = info;
    wifiInfoCount = count;
}

#ifdef ALTRUIST_INSIGHT
void SensorWebServer::_webserver_scan_urbans() {
	debug_outln_info(F("ws: scan_urbans ..."));
	String json = "[";
	int n = MDNS.queryService("altruist", "tcp");
	bool first = true;
	for (int i = 0; i < n; i++) {
		String device_type = "";
		if (MDNS.hasTxt(i, DEVICE_MODEL_MDNS_PROPERTY)) {
			device_type = MDNS.txt(i, DEVICE_MODEL_MDNS_PROPERTY);
		}
		if (device_type == DEVICE_MODEL_URBAN) {
			if (!first) json += ",";
			json += "{\"ip\":\"" + MDNS.address(i).toString() + "\",\"hostname\":\"" + MDNS.hostname(i) + "\"}";
			first = false;
		}
	}
	json += "]";
	server.send(200, FPSTR(TXT_CONTENT_TYPE_JSON), json);
}

void SensorWebServer::_send_urban_pairing_form_html() {
	MDNS.begin(cfg::local_hostname);
	MDNS.addService("altruist", "tcp", 80);
	MDNS.addServiceTxt("altruist", "tcp", DEVICE_MODEL_MDNS_PROPERTY, DEVICE_MODEL);
	delay(250);
	yield();

	int nrOfServices = MDNS.queryService("altruist", "tcp");
	debug_outln_info(F("mDNS scan found services: "), String(nrOfServices));

	RESERVE_STRING(page_content, XLARGE_STR);
	start_html_page(page_content, FPSTR(INTL_CONFIGURATION), true);

	page_content += F(
		"<div style='margin:20px auto;max-width:480px;padding:20px;'>"
		"<h3 style='margin-bottom:15px;text-align:center;'>" INTL_SELECT_URBAN_TITLE "</h3>"
		"<p style='color:#666;font-size:14px;margin-bottom:15px;text-align:center;'>"
		INTL_SELECT_URBAN_DESC "</p>"
		"<form class='guest-wizard-form' method='POST' action='/select_urban'>");

	int urban_count = 0;
	for (int i = 0; i < nrOfServices; i++) {
		String device_type = "";
		if (MDNS.hasTxt(i, DEVICE_MODEL_MDNS_PROPERTY)) {
			device_type = MDNS.txt(i, DEVICE_MODEL_MDNS_PROPERTY);
		}
		if (device_type == DEVICE_MODEL_URBAN) {
			String ip = MDNS.address(i).toString();
			String hostname = MDNS.hostname(i);
			debug_outln_info(F("Found Urban: "), hostname + " (" + ip + ")");

			page_content += F("<div style='margin:8px 0;padding:12px;border:1px solid #ddd;border-radius:6px;background:#fafafa;'>"
				"<label style='cursor:pointer;display:flex;align-items:center;gap:10px;font-size:15px;'>"
				"<input type='radio' name='chosen_altruist_urban' value='");
			page_content += ip;
			page_content += "'";
			if (urban_count == 0) {
				page_content += F(" checked");
			}
			page_content += F("> <strong>");
			page_content += hostname;
			page_content += F("</strong>&nbsp;(");
			page_content += ip;
			page_content += F(")</label></div>");
			urban_count++;
		}
	}

	if (urban_count == 0) {
		page_content += F("<div style='padding:15px;background:#fff3cd;border:1px solid #ffc107;border-radius:6px;margin:10px 0;'>"
			"<p style='margin:0;color:#856404;'>" INTL_NO_URBANS_FOUND "</p></div>");
	}

	page_content += F("<div style='margin:8px 0;padding:12px;border:1px solid #ddd;border-radius:6px;background:#fafafa;'>"
		"<label style='cursor:pointer;display:flex;align-items:center;gap:10px;font-size:15px;'>"
		"<input type='radio' name='chosen_altruist_urban' value='__custom__'");
	if (urban_count == 0) {
		page_content += F(" checked");
	}
	page_content += F("> " INTL_USE_CUSTOM_IP "</label>"
		"<input type='text' name='custom_ip' placeholder='e.g. 192.168.1.100' "
		"style='margin-top:8px;padding:8px 12px;width:90%;max-width:220px;border:1px solid #ccc;border-radius:4px;font-size:14px;'/>"
		"</div>");

	page_content += F(
		"<button type='submit' class='submit-btn' style='margin-top:20px;width:100%;padding:14px;'>");
	page_content += FPSTR(INTL_SAVE_AND_RESTART);
	page_content += F("</button></form></div>");
	page_content += FPSTR(WEB_GUEST_WIZARD_SUBMIT_JS);

	end_html_page_guest(page_content);
}

void SensorWebServer::_webserver_select_urban() {
	debug_outln_info(F("ws: select_urban ..."));
	if (server.method() != HTTP_POST) {
		sendHttpRedirectGuest();
		return;
	}
#ifdef ALTRUIST_INSIGHT
	insightGuestClearFinishPending();
#endif

	// Step 1: checkbox form from guest WiFi success (no chosen_altruist_urban field).
	if (!server.hasArg(F("chosen_altruist_urban"))) {
		const bool wantUrban = server.hasArg(F("pair_with_urban")) && server.arg(F("pair_with_urban")) == F("1");
		if (!wantUrban) {
			debug_outln_info(F("Insight standalone at setup (checkbox unchecked)"));
			cfg::standalone = true;
			cfg::use_custom_urban = false;
			cfg::chosen_altruist_urban[0] = '\0';
			cfg::custom_altruist_urban[0] = '\0';
			cfgApplyStandaloneModeEnabled();
		} else {
			_send_urban_pairing_form_html();
			return;
		}

		RESERVE_STRING(page_content, LARGE_STR);
		start_html_page(page_content, F(INTL_SETUP_COMPLETE), true);
		String setup_ip = WiFi.localIP().toString();
		page_content += F("<div style='text-align:center;padding:40px;'>"
			"<h2 style='color:#4CAF50;'>" INTL_SETTINGS_SAVED "</h2>"
			"<p>" INTL_GUEST_IP_ADDRESS " <strong><span class='ip-address'>");
		page_content += setup_ip;
		page_content += F("</span></strong> <button class='copy-btn' onclick='copyText()'></button></p>"
			"<p>" INTL_GUEST_OPEN_IP_HINT "</p>"
			"</div>");
		page_content += FPSTR(WEB_COPY_IP_JS);
		end_html_page_guest(page_content);

		if (writeConfig()) {
			set_restart_reason(RESTART_REASON_CONFIG);
			sensor_restart();
		}
		return;
	}

	// Step 2: Urban IP / custom IP chosen.
	String chosen = server.arg("chosen_altruist_urban");

	if (chosen == "__skip__") {
		debug_outln_info(F("Urban selection skipped by user"));
		cfg::standalone = true;
		cfg::use_custom_urban = false;
		cfg::chosen_altruist_urban[0] = '\0';
		cfg::custom_altruist_urban[0] = '\0';
		cfgApplyStandaloneModeEnabled();
	} else if (chosen == "__custom__") {
		String custom_ip = server.arg("custom_ip");
		if (custom_ip.length() > 0) {
			strncpy(cfg::custom_altruist_urban, custom_ip.c_str(), LEN_CHOSEN_ALTRUIS_ADDRESS - 1);
			cfg::custom_altruist_urban[LEN_CHOSEN_ALTRUIS_ADDRESS - 1] = '\0';
			cfg::use_custom_urban = true;
			cfg::standalone = false;
			cfgOnStandaloneModeDisabled();
			debug_outln_info(F("Custom Urban IP set: "), custom_ip);
		} else {
			cfg::standalone = true;
			cfg::use_custom_urban = false;
			cfg::chosen_altruist_urban[0] = '\0';
			cfg::custom_altruist_urban[0] = '\0';
			debug_outln_info(F("Custom Urban IP empty; standalone mode"));
			cfgApplyStandaloneModeEnabled();
		}
	} else if (chosen.length() > 0) {
		strncpy(cfg::chosen_altruist_urban, chosen.c_str(), LEN_CHOSEN_ALTRUIS_ADDRESS - 1);
		cfg::chosen_altruist_urban[LEN_CHOSEN_ALTRUIS_ADDRESS - 1] = '\0';
		cfg::use_custom_urban = false;
		cfg::standalone = false;
		cfgOnStandaloneModeDisabled();
		debug_outln_info(F("Chosen Urban IP: "), chosen);
	}

	RESERVE_STRING(page_content, LARGE_STR);
	start_html_page(page_content, F(INTL_SETUP_COMPLETE), true);
	String setup_ip = WiFi.localIP().toString();
	page_content += F("<div style='text-align:center;padding:40px;'>"
		"<h2 style='color:#4CAF50;'>" INTL_SETTINGS_SAVED "</h2>"
		"<p>" INTL_GUEST_IP_ADDRESS " <strong><span class='ip-address'>");
	page_content += setup_ip;
	page_content += F("</span></strong> <button class='copy-btn' onclick='copyText()'></button></p>"
		"<p>" INTL_GUEST_OPEN_IP_HINT "</p>"
		"</div>");
	page_content += FPSTR(WEB_COPY_IP_JS);
	end_html_page_guest(page_content);

	if (xSemaphoreTake(mutex, pdMS_TO_TICKS(500))) {
		clearUrbanPairingTelemetry(sensors_data);
		xSemaphoreGive(mutex);
	}
	displayManager.clearUrbanCache();

	if (writeConfig()) {
		set_restart_reason(RESTART_REASON_CONFIG);
		sensor_restart();
	}
}
#endif

void SensorWebServer::_webserver_ota() {
	if (WiFi.status() != WL_CONNECTED) {
		sendHttpRedirectGuest();
		return;
	}
	if (!webserver_request_auth()) {
		return;
	}

	RESERVE_STRING(page_content, LARGE_STR);
	start_html_page(page_content, FPSTR(INTL_OTA_UPDATE), false, "settings");

	if (server.method() == HTTP_POST) {
		append_app_page_body_start(page_content, F(INTL_PAGE_OTA_INTRO));
		if (server.hasArg("action") && server.arg("action") == F("switch_lang")) {
			String new_lang = server.arg("current_lang");
			new_lang.toUpperCase();
			if (new_lang == String(CURRENT_LANG)) {
				page_content += F("<div class='ui-notice ui-notice--warn'><strong>");
				page_content += FPSTR(INTL_OTA_LANG_SAME);
				page_content += F("</strong></div>");
			} else {
				strncpy(cfg::current_lang, new_lang.c_str(), sizeof(cfg::current_lang) - 1);
				cfg::current_lang[sizeof(cfg::current_lang) - 1] = '\0';
				writeConfig();
				deviceStatus.ota_update_requested = true;
				page_content += F("<div class='ui-notice ui-notice--ok'><strong>");
				page_content += FPSTR(INTL_OTA_LANG_REQUESTED);
				page_content += F("</strong></div>");
			}
		} else {
			deviceStatus.ota_update_requested = true;
			page_content += F("<div class='ui-notice ui-notice--ok'><strong>");
			page_content += FPSTR(INTL_OTA_CHECK_REQUESTED);
			page_content += F("</strong></div>");
		}
	} else {
		append_app_page_body_start(page_content, F(INTL_PAGE_OTA_INTRO));
		page_content += F("<div class='data-sheet'>");
		add_data_section_start(page_content, FPSTR(INTL_OTA_UPDATE));
		add_data_row_from_value(page_content, FPSTR(INTL_OTA_CURRENT_VERSION), String(SOFTWARE_VERSION_STR));
		add_data_row_from_value(page_content, "Firmware channel", ALTRUIST_BUILD_CHANNEL);
		add_data_row_from_value(page_content, FPSTR(INTL_LAST_OTA),
			delayToString(millis() - deviceStatus.last_update_attempt));
		add_data_section_end(page_content);
		page_content += F("</div>");

		page_content += F("<div class='page-form'>");

		page_content += F("<section class='config-section'><h2 class='config-section__title'>");
		page_content += FPSTR(INTL_OTA_CHECK_UPDATE);
		page_content += F("</h2><div class='config-section__body'>"
			"<form method='POST' action='/ota'>");
		page_content += form_submit(FPSTR(INTL_OTA_CHECK_UPDATE));
		page_content += F("</form></div></section>");

		page_content += F("<section class='config-section'><h2 class='config-section__title'>");
		page_content += FPSTR(INTL_OTA_SWITCH_LANG);
		page_content += F("</h2><div class='config-section__body'>"
			"<p class='form-hint'><strong>");
		page_content += FPSTR(INTL_OTA_CURRENT_LANG);
		page_content += F(":</strong> ");
		page_content += String(CURRENT_LANG);
		page_content += F("</p>"
			"<form method='POST' action='/ota'>"
			"<input type='hidden' name='action' value='switch_lang'>");
		page_content += form_select_lang();
		page_content += F("<p class='form-hint'>");
		page_content += FPSTR(INTL_OTA_SWITCH_LANG_NOTE);
		page_content += F("</p>");
		page_content += form_submit(FPSTR(INTL_OTA_SWITCH_LANG));
		page_content += F("</form></div></section>");

		page_content += F("</div>");
	}

	append_app_page_body_end(page_content);
	end_html_page_app(page_content);
}

void SensorWebServer::_webserver_config() {
    if (WiFi.status() != WL_CONNECTED) {
		sendHttpRedirectGuest();
	} else {
		if (!webserver_request_auth())
		{ return; }

		debug_outln_info(F("ws: config page ..."));

		server.sendHeader(F("Cache-Control"), F("no-cache, no-store, must-revalidate"));
		server.sendHeader(F("Pragma"), F("no-cache"));
		server.sendHeader(F("Expires"), F("0"));

		RESERVE_STRING(page_content, XLARGE_STR);
		page_content.reserve(4096);

		start_html_page(page_content, FPSTR(INTL_CONFIGURATION), false, "settings", true);
		if (wificonfig_loop) {  // scan for wlan ssids
			page_content += FPSTR(WEB_CONFIG_SCRIPT);
			web_page_flush_chunk(page_content, &server);
		}

		if (server.method() == HTTP_GET) {
			webserver_config_send_body_get(server, page_content, wificonfig_loop, sensors_data);
		} else {
#ifdef ALTRUIST_INSIGHT
			const bool prev_use_custom_urban = cfg::use_custom_urban;
			const String prev_custom_urban_ip = String(cfg::custom_altruist_urban);
			const String prev_chosen_urban_ip = String(cfg::chosen_altruist_urban);
			const bool prev_standalone = cfg::standalone;
#endif
			webserver_config_send_body_post(server);
			rwsOnConfigOwnerUpdated(robonomics_address);
#ifdef ALTRUIST_INSIGHT
			if (!prev_standalone && cfg::standalone) {
				cfgApplyStandaloneModeEnabled();
			} else if (prev_standalone && !cfg::standalone) {
				cfgOnStandaloneModeDisabled();
			}
			if (prev_use_custom_urban != cfg::use_custom_urban ||
			    prev_custom_urban_ip != String(cfg::custom_altruist_urban) ||
			    prev_chosen_urban_ip != String(cfg::chosen_altruist_urban)) {
				if (xSemaphoreTake(mutex, pdMS_TO_TICKS(500))) {
					clearUrbanPairingTelemetry(sensors_data);
					xSemaphoreGive(mutex);
				}
				displayManager.clearUrbanCache();
			}
#endif
			page_content += FPSTR(INTL_SENSOR_IS_REBOOTING);
			web_page_flush_chunk(page_content, &server);
		}
		end_html_page_app(page_content);

		if (server.method() == HTTP_POST) {

			if (writeConfig()) {
				set_restart_reason(RESTART_REASON_CONFIG);
				sensor_restart();
			}
		}
	}
}

void SensorWebServer::_webserver_hub_local() {
	if (WiFi.status() != WL_CONNECTED) {
		sendHttpRedirectGuest();
		return;
	}
	if (!webserver_request_auth()) {
		return;
	}

	server.sendHeader(F("Cache-Control"), F("no-cache, no-store, must-revalidate"));
	server.sendHeader(F("Pragma"), F("no-cache"));
	server.sendHeader(F("Expires"), F("0"));

	RESERVE_STRING(page_content, XLARGE_STR);
	const String title = buildLocalAccessLabel();
	start_html_page(page_content, title, false, "local", true);

	if (server.method() == HTTP_POST) {
#ifdef ALTRUIST_INSIGHT
		const bool prev_use_custom_urban = cfg::use_custom_urban;
		const String prev_custom_urban_ip = String(cfg::custom_altruist_urban);
		const String prev_chosen_urban_ip = String(cfg::chosen_altruist_urban);
		const bool prev_standalone = cfg::standalone;
#endif
		webserver_config_send_body_post(server);
		rwsOnConfigOwnerUpdated(robonomics_address);
#ifdef ALTRUIST_INSIGHT
		if (!prev_standalone && cfg::standalone) {
			cfgApplyStandaloneModeEnabled();
		} else if (prev_standalone && !cfg::standalone) {
			cfgOnStandaloneModeDisabled();
		}
		if (prev_use_custom_urban != cfg::use_custom_urban ||
		    prev_custom_urban_ip != String(cfg::custom_altruist_urban) ||
		    prev_chosen_urban_ip != String(cfg::chosen_altruist_urban)) {
			if (xSemaphoreTake(mutex, pdMS_TO_TICKS(500))) {
				clearUrbanPairingTelemetry(sensors_data);
				xSemaphoreGive(mutex);
			}
			displayManager.clearUrbanCache();
		}
#endif
		page_content += FPSTR(INTL_SENSOR_IS_REBOOTING);
		web_page_flush_chunk(page_content, &server);
		end_html_page_app(page_content);
		if (writeConfig()) {
			set_restart_reason(RESTART_REASON_CONFIG);
			sensor_restart();
		}
		return;
	}

	DynamicJsonDocument values_snapshot(sensors_data.capacity());
	bool readings_busy = false;
	if (!xSemaphoreTake(mutex, pdMS_TO_TICKS(500))) {
		readings_busy = true;
	} else {
		values_snapshot.set(sensors_data.as<JsonVariantConst>());
		xSemaphoreGive(mutex);
		if (values_snapshot.overflowed()) {
			readings_busy = true;
		}
	}

	webserver_hub_local(page_content, values_snapshot, deviceStatus, server, wificonfig_loop, readings_busy);
	end_html_page_app(page_content);
}

void SensorWebServer::_webserver_hub_social() {
	if (WiFi.status() != WL_CONNECTED) {
		sendHttpRedirectGuest();
		return;
	}
	if (!webserver_request_auth()) {
		return;
	}

	server.sendHeader(F("Cache-Control"), F("no-cache, no-store, must-revalidate"));
	server.sendHeader(F("Pragma"), F("no-cache"));
	server.sendHeader(F("Expires"), F("0"));

	RESERVE_STRING(page_content, XLARGE_STR);
	start_html_page(page_content, FPSTR(INTL_DASH_GROUP_SOCIAL_TITLE), false, "social", true);

	const String self_ss58 = String(robonomics.getSs58Address());
	setRobonomicsAddress(self_ss58);
	rwsSyncGroupModeFromOwner(self_ss58);

	RwsGroupApplyResult group_save = RwsGroupApply_None;
	if (server.method() == HTTP_POST) {
		if (server.hasArg(F("save_group"))) {
			group_save = webserver_group_post(server, self_ss58);
		} else {
#ifdef ALTRUIST_INSIGHT
			const bool prev_use_custom_urban = cfg::use_custom_urban;
			const String prev_custom_urban_ip = String(cfg::custom_altruist_urban);
			const String prev_chosen_urban_ip = String(cfg::chosen_altruist_urban);
			const bool prev_standalone = cfg::standalone;
#endif
			webserver_config_send_body_post(server);
			rwsOnConfigOwnerUpdated(robonomics_address);
#ifdef ALTRUIST_INSIGHT
			if (!prev_standalone && cfg::standalone) {
				cfgApplyStandaloneModeEnabled();
			} else if (prev_standalone && !cfg::standalone) {
				cfgOnStandaloneModeDisabled();
			}
			if (prev_use_custom_urban != cfg::use_custom_urban ||
			    prev_custom_urban_ip != String(cfg::custom_altruist_urban) ||
			    prev_chosen_urban_ip != String(cfg::chosen_altruist_urban)) {
				if (xSemaphoreTake(mutex, pdMS_TO_TICKS(500))) {
					clearUrbanPairingTelemetry(sensors_data);
					xSemaphoreGive(mutex);
				}
				displayManager.clearUrbanCache();
			}
#endif
			page_content += FPSTR(INTL_SENSOR_IS_REBOOTING);
			web_page_flush_chunk(page_content, &server);
			end_html_page_app(page_content);
			if (writeConfig()) {
				set_restart_reason(RESTART_REASON_CONFIG);
				sensor_restart();
			}
			return;
		}
	}

	DynamicJsonDocument values_snapshot(sensors_data.capacity());
	if (!xSemaphoreTake(mutex, pdMS_TO_TICKS(500))) {
		values_snapshot.clear();
	} else {
		values_snapshot.set(sensors_data.as<JsonVariantConst>());
		xSemaphoreGive(mutex);
	}

	webserver_hub_social(page_content, self_ss58, &robonomics, server, values_snapshot, group_save, wificonfig_loop);
	end_html_page_app(page_content);
}

void SensorWebServer::_webserver_hub_custom() {
	if (WiFi.status() != WL_CONNECTED) {
		sendHttpRedirectGuest();
		return;
	}
	if (!webserver_request_auth()) {
		return;
	}

	server.sendHeader(F("Cache-Control"), F("no-cache, no-store, must-revalidate"));
	server.sendHeader(F("Pragma"), F("no-cache"));
	server.sendHeader(F("Expires"), F("0"));

	RESERVE_STRING(page_content, XLARGE_STR);
	start_html_page(page_content, FPSTR(INTL_DASH_GROUP_CUSTOM_TITLE), false, "custom", true);

	if (server.method() == HTTP_POST) {
		webserver_config_send_body_post(server);
		page_content += FPSTR(INTL_SENSOR_IS_REBOOTING);
		web_page_flush_chunk(page_content, &server);
		end_html_page_app(page_content);
		if (writeConfig()) {
			set_restart_reason(RESTART_REASON_CONFIG);
			sensor_restart();
		}
		return;
	}

	webserver_hub_custom(page_content, server);
	end_html_page_app(page_content);
}

void SensorWebServer::_webserver_hub_advanced() {
	if (WiFi.status() != WL_CONNECTED) {
		sendHttpRedirectGuest();
		return;
	}
	if (!webserver_request_auth()) {
		return;
	}

	server.sendHeader(F("Cache-Control"), F("no-cache, no-store, must-revalidate"));
	server.sendHeader(F("Pragma"), F("no-cache"));
	server.sendHeader(F("Expires"), F("0"));

	RESERVE_STRING(page_content, XLARGE_STR);
	start_html_page(page_content, FPSTR(INTL_NAV_ADVANCED), false, "advanced", true);

	if (server.method() == HTTP_POST) {
		webserver_config_send_body_post(server);
		page_content += FPSTR(INTL_SENSOR_IS_REBOOTING);
		web_page_flush_chunk(page_content, &server);
		end_html_page_app(page_content);
		if (writeConfig()) {
			set_restart_reason(RESTART_REASON_CONFIG);
			sensor_restart();
		}
		return;
	}

	webserver_hub_advanced(page_content, server, wificonfig_loop);
	end_html_page_app(page_content);
}

void SensorWebServer::_webserver_hub_warnings_redirect() {
	if (WiFi.status() != WL_CONNECTED) {
		sendHttpRedirectGuest();
		return;
	}
	if (!webserver_request_auth()) {
		return;
	}
	server.sendHeader(F("Location"), F("/advanced"));
	server.send(302, FPSTR(TXT_CONTENT_TYPE_TEXT_HTML), emptyString);
}

bool SensorWebServer::webserver_request_auth() {
	if (cfg::www_basicauth_enabled && ! wificonfig_loop) {
		debug_outln_info(F("validate request auth..."));
		if (!server.authenticate(www_username.c_str(), www_password.c_str())) {
			server.requestAuthentication(BASIC_AUTH, "Sensor Login", F("Authentication failed"));
			return false;
		}
	}
	return true;
}

void SensorWebServer::sendHttpRedirectGuest() {
	// Never hard-code 192.168.4.1 in STA-only mode: after a home-WiFi drop the device is not that AP,
	// and browsers following this redirect appear "stuck" / webserver dead while HTTP is still up.
	const IPAddress ap_ip = WiFi.softAPIP();
	const IPAddress sta_ip = WiFi.localIP();
	String loc = F("http://");
	if (wificonfig_loop && ap_ip[0] != 0) {
		loc += ap_ip.toString();
	} else if (sta_ip[0] != 0) {
		loc += sta_ip.toString();
	} else {
		server.send(503, FPSTR(TXT_CONTENT_TYPE_TEXT_PLAIN),
		             F("WiFi not ready yet; try again in a few seconds."));
		return;
	}
	loc += F("/guest");
	server.sendHeader(F("Location"), loc);
	server.send(302, FPSTR(TXT_CONTENT_TYPE_TEXT_HTML), emptyString);
}

void SensorWebServer::sendHttpRedirectConnected(String &address) {
	String redirect = F("http://");
	redirect += address;
	debug_outln_info(F("Redirecting to: "), redirect);
	server.sendHeader(F("Location"), redirect);
	server.sendHeader(F("Cache-Control"), F("no-cache"));
	server.sendHeader(F("Connection"), F("close"));
	server.send(303, FPSTR(TXT_CONTENT_TYPE_TEXT_HTML), "<html><body>Redirecting...</body></html>");

}

void SensorWebServer::stream_html_page_head(const String& title, bool guest_page, const char* app_page, bool app_config_layout) {
	RESERVE_STRING(s, LARGE_STR);
	s = FPSTR(WEB_PAGE_HEADER);
	s.replace("{t}", title.length() ? title : String(F(INTL_DASH_TITLE)));
	server.setContentLength(CONTENT_LENGTH_UNKNOWN);
	server.send(200, FPSTR(TXT_CONTENT_TYPE_TEXT_HTML), s);
	yield();

	if (app_page) {
		if (app_config_layout) {
			s = FPSTR(WEB_PAGE_APP_CONFIG_HEADER_HEAD);
		} else {
			s = FPSTR(WEB_PAGE_APP_HEADER_HEAD);
		}
		s.replace(F("{page}"), app_page);
		server.sendContent(s);
		yield();

		s = FPSTR(WEB_PAGE_APP_TOPBAR_BODY);
		s.replace(F("{device}"), esp_chipid);
		s.replace(F("{addr}"), robonomics_address);
		server.sendContent(s);
		yield();

		s = FPSTR(WEB_PAGE_APP_LAYOUT_OPEN);
		server.sendContent(s);
		yield();

		RESERVE_STRING(sidebar, LARGE_STR);
		append_app_sidebar(sidebar);
		server.sendContent(sidebar);
		yield();

		if (app_config_layout) {
			s = FPSTR(WEB_PAGE_APP_CONFIG_MAIN_OPEN);
		} else {
			s = FPSTR(WEB_PAGE_APP_MAIN_OPEN);
		}
		s.replace(F("{t}"), title.length() ? title : String(F(INTL_DASH_TITLE)));
		s.replace(F("{home}"), buildLocalAccessLabel());
		server.sendContent(s);
		yield();
		return;
	}

	if (title.indexOf(FPSTR(INTL_CONFIGURATION)) != -1) {
		server.sendContent_P(WEB_PAGE_HEADER_CONFIG_HEAD);
	} else {
		server.sendContent_P(WEB_PAGE_HEADER_HEAD);
	}
	yield();

	if (guest_page) {
		s = FPSTR(WEB_PAGE_GUEST_HEADER_BODY);
	} else if (title.indexOf(FPSTR(INTL_DEBUG_LEVEL)) != -1) {
		s = FPSTR(WEB_PAGE_DEBUG_HEADER_BODY);
	} else if (title.indexOf(FPSTR(INTL_CONFIGURATION)) != -1) {
		s = FPSTR(WEB_PAGE_CONFIG_HEADER_BODY);
	} else if (title.indexOf(FPSTR(INTL_OTA_UPDATE)) != -1 || title.indexOf(FPSTR(INTL_GROUP_MENU)) != -1) {
		s = FPSTR(WEB_PAGE_DATA_HEADER_BODY);
	} else {
		s = FPSTR(WEB_PAGE_HEADER_BODY);
	}
	s.replace("{addr}", robonomics_address);
	if (!guest_page) {
		s.replace("{t}", title);
		if (title != " ") {
			s.replace("{n}", F("&raquo;"));
		} else {
			s.replace("{n}", emptyString);
		}
	}
	s.replace("{id}", esp_chipid);
	s.replace("{mac}", WiFi.macAddress());
	server.sendContent(s);
	yield();
}

void SensorWebServer::start_html_page(String& page_content, const String& title, bool guest_page, const char* app_page, bool app_config_layout) {
	(void)page_content;
	stream_html_page_head(title, guest_page, app_page, app_config_layout);
}

void SensorWebServer::end_html_page(String& page_content) {
	if (page_content.length()) {
		server.sendContent(page_content);
		page_content = emptyString;
	}
	server.sendContent_P(WEB_PAGE_FOOTER);
	web_page_finish_chunked(&server);
}

void SensorWebServer::end_html_page_guest(String& page_content) {
	if (page_content.length()) {
		server.sendContent(page_content);
		page_content = emptyString;
	}
	server.sendContent_P(WEB_PAGE_GUEST_FOOTER);
	web_page_finish_chunked(&server);
}

void SensorWebServer::end_html_page_root(String& page_content) {
	if (page_content.length()) {
		server.sendContent(page_content);
		page_content = emptyString;
	}
	server.sendContent_P(WEB_PAGE_ROOT_FOOTER);
	web_page_finish_chunked(&server);
}

void SensorWebServer::end_html_page_app(String& page_content) {
	if (page_content.length()) {
		server.sendContent(page_content);
		page_content = emptyString;
	}
	RESERVE_STRING(footer, XLARGE_STR);
	footer = FPSTR(WEB_PAGE_APP_FOOTER);
	footer.replace(F("{local}"), buildLocalAccessLabel());
	server.sendContent(footer);
	web_page_finish_chunked(&server);
}
