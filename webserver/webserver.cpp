#include "webserver.h"
#include "pages/pages.h"
#include "../utils.h"
#include "html-content.h"
#include "../config_manager/config_helpers.h"


void SensorWebServer::setup() {
    www_username = cfg::www_username;
    www_password = cfg::www_password;
    uint64_t chipid_num;
	chipid_num = ESP.getEfuseMac();
    esp_chipid = (uint16_t)(chipid_num >> 32), HEX;
	esp_chipid += String((uint32_t)chipid_num, HEX);

	server.on("/guest", std::bind(&SensorWebServer::_webserver_guest, this)); // x
	server.on("/", std::bind(&SensorWebServer::_webserver_root, this)); // x
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
	server.on(F(STATIC_PREFIX), std::bind(&SensorWebServer::_webserver_static, this)); // x
	server.onNotFound(std::bind(&SensorWebServer::_webserver_not_found, this)); // x

	debug_outln_info(F("Starting Webserver... "), WiFi.localIP().toString());
	server.begin();
}

void SensorWebServer::_webserver_status() {
	if (WiFi.status() != WL_CONNECTED) {
		sendHttpRedirect();
		return;
	}

	RESERVE_STRING(page_content, XLARGE_STR);
	start_html_page(page_content, FPSTR(INTL_DEVICE_STATUS));

	debug_outln_info(F("ws: status ..."));
	server.sendContent(page_content);
    webserver_status_part1(page_content, deviceStatus);
    server.sendContent(page_content);
    page_content = FPSTR(EMPTY_ROW);
    webserver_status_part2(page_content, deviceStatus);
    server.sendContent(page_content);

	page_content += FPSTR(TABLE_TAG_CLOSE_BR);
	end_html_page(page_content);
}

void SensorWebServer::_webserver_data_json() {
	String json_content;
    webserver_data_json(sensors_data, esp_chipid, json_content);
    server.send(200, FPSTR(TXT_CONTENT_TYPE_JSON), json_content);
}

void SensorWebServer::_webserver_not_found() {
	debug_outln_info(F("ws: not found ..."));
	if (WiFi.status() != WL_CONNECTED) {
		if ((server.uri().indexOf(F("success.html")) != -1) || (server.uri().indexOf(F("detect.html")) != -1)) {
			server.send(200, FPSTR(TXT_CONTENT_TYPE_TEXT_HTML), FPSTR(WEB_IOS_REDIRECT));
		} else {
			sendHttpRedirect();
		}
	} else {
		server.send(404, FPSTR(TXT_CONTENT_TYPE_TEXT_PLAIN), F("Not found."));
	}
}

void SensorWebServer::_webserver_static() {
	server.sendHeader(F("Cache-Control"), F("max-age=2592000, public"));

	if (server.arg(String('r')) == F("logo")) {
		server.send_P(200, TXT_CONTENT_TYPE_IMAGE_PNG,
			LUFTDATEN_INFO_LOGO_PNG, LUFTDATEN_INFO_LOGO_PNG_SIZE);
	}
	else if (server.arg(String('r')) == F("css")) {
		server.send_P(200, TXT_CONTENT_TYPE_TEXT_CSS,
			WEB_PAGE_STATIC_CSS, sizeof(WEB_PAGE_STATIC_CSS)-1);
	} else {
		_webserver_not_found();
	}
}

void SensorWebServer::_webserver_favicon() {
	server.sendHeader(F("Cache-Control"), F("max-age=2592000, public"));

	server.send_P(200, TXT_CONTENT_TYPE_IMAGE_PNG,
		LUFTDATEN_INFO_LOGO_PNG, LUFTDATEN_INFO_LOGO_PNG_SIZE);
}

void SensorWebServer::_webserver_restart() {
    if (!webserver_request_auth())
	{ return; }

	RESERVE_STRING(page_content, LARGE_STR);
	start_html_page(page_content, FPSTR(INTL_DELETE_CONFIG));
    bool is_HTTP_GET = server.method() == HTTP_GET;
    webserver_removeConfig(page_content, is_HTTP_GET);
    end_html_page(page_content);
    if (!is_HTTP_GET) {
        esp_restart();
    }
}

void SensorWebServer::_webserver_removeConfig() {
    if (!webserver_request_auth())
	{ return; }

	RESERVE_STRING(page_content, LARGE_STR);
	start_html_page(page_content, FPSTR(INTL_DELETE_CONFIG));
    bool is_HTTP_GET = server.method() == HTTP_GET;
    webserver_removeConfig(page_content, is_HTTP_GET);
    end_html_page(page_content);
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
	start_html_page(page_content, FPSTR(INTL_DEBUG_LEVEL));
    webserver_debug_level(server, page_content);
    end_html_page(page_content);;
}

void SensorWebServer::_webserver_values() {
    if (WiFi.status() != WL_CONNECTED) {
		sendHttpRedirect();
		return;
	}
    if (!webserver_request_auth())
		{ return; }
    RESERVE_STRING(page_content, XLARGE_STR);
    start_html_page(page_content, FPSTR(INTL_CURRENT_DATA));
    webserver_values(sensors_data, page_content);
    end_html_page(page_content);
}

void SensorWebServer::_webserver_guest() {
    server.sendHeader(F("Cache-Control"), F("no-cache, no-store, must-revalidate"));
	server.sendHeader(F("Pragma"), F("no-cache"));
	server.sendHeader(F("Expires"), F("0"));
	// Enable Pagination (Chunked Transfer)
	server.setContentLength(CONTENT_LENGTH_UNKNOWN);

	RESERVE_STRING(page_content, XLARGE_STR);

	start_html_page(page_content, FPSTR(INTL_CONFIGURATION));
    debug_outln_info(F("ws: guest ..."));

	if (wificonfig_loop) {  // scan for wlan ssids
		page_content += FPSTR(WEB_CONFIG_SCRIPT);
	}

	if (server.method() == HTTP_GET) {
		webserver_guest_create_body_get_part1(page_content, wificonfig_loop);
        // Paginate page after ~ 1500 Bytes
        server.sendContent(page_content);
	    page_content = emptyString;
        webserver_guest_create_body_get_part2(page_content, wificonfig_loop);
        server.sendContent(page_content);
	    page_content = emptyString;
	} else {
		webserver_config_send_body_post(server, page_content);
	}

	if (server.method() == HTTP_POST) {
		if (writeConfig()) {
			sensor_restart();
		}
	}
    // end_html_page(page_content);
}

void SensorWebServer::setWifiInfo(struct_wifiInfo* info, uint8_t count) {
    wifiInfo = info;
    wifiInfoCount = count;
}

void SensorWebServer::_webserver_config() {
    if (WiFi.status() != WL_CONNECTED) {
		sendHttpRedirect();
	} else {
		if (!webserver_request_auth())
		{ return; }

		debug_outln_info(F("ws: config page ..."));

		server.sendHeader(F("Cache-Control"), F("no-cache, no-store, must-revalidate"));
		server.sendHeader(F("Pragma"), F("no-cache"));
		server.sendHeader(F("Expires"), F("0"));
		// Enable Pagination (Chunked Transfer)
		server.setContentLength(CONTENT_LENGTH_UNKNOWN);

		RESERVE_STRING(page_content, XLARGE_STR);

		start_html_page(page_content, FPSTR(INTL_CONFIGURATION));
		if (wificonfig_loop) {  // scan for wlan ssids
			page_content += FPSTR(WEB_CONFIG_SCRIPT);
		}

		if (server.method() == HTTP_GET) {
			webserver_config_send_body_get(server, page_content, wificonfig_loop);
		} else {
			webserver_config_send_body_post(server, page_content);
		}
		end_html_page(page_content);

		if (server.method() == HTTP_POST) {
			// display_debug(F("Writing config"), emptyString);
			if (writeConfig()) {
				// display_debug(F("Writing config"), F("and restarting"));
				sensor_restart();
			}
		}
	}
}

void SensorWebServer::_webserver_root() {
    if (WiFi.status() != WL_CONNECTED) {
		sendHttpRedirect();
		return;
	}
    if (!webserver_request_auth())
		{ return; }

    RESERVE_STRING(page_content, XLARGE_STR);
    start_html_page(page_content, emptyString);
    debug_outln_info(F("ws: root ..."));
    webserver_root(page_content);
    end_html_page(page_content);
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

void SensorWebServer::sendHttpRedirect() {
	server.sendHeader(F("Location"), F("http://192.168.4.1/guest"));
	server.send(302, FPSTR(TXT_CONTENT_TYPE_TEXT_HTML), emptyString);
}

void SensorWebServer::start_html_page(String& page_content, const String& title) {
	RESERVE_STRING(s, LARGE_STR);
	s = FPSTR(WEB_PAGE_HEADER);
	s.replace("{t}", title);
	server.setContentLength(CONTENT_LENGTH_UNKNOWN);
	server.send(200, FPSTR(TXT_CONTENT_TYPE_TEXT_HTML), s);

	server.sendContent_P(WEB_PAGE_HEADER_HEAD);

	s = FPSTR(WEB_PAGE_HEADER_BODY);
	s.replace("{addr}", robonomics_address);
	s.replace("{t}", title);
	if (title != " ") {
		s.replace("{n}", F("&raquo;"));
	} else {
		s.replace("{n}", emptyString);
	}
	s.replace("{id}", esp_chipid);
	s.replace("{mac}", WiFi.macAddress());
	page_content += s;
}

void SensorWebServer::end_html_page(String& page_content) {
	if (page_content.length()) {
		server.sendContent(page_content);
	}
	server.sendContent_P(WEB_PAGE_FOOTER);
}