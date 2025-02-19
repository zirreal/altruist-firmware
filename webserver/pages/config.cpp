#include "pages.h"
#include "../../config_manager/config_helpers.h"
#include "../../utils.h"
#include "../html-content.h"
#include "../utils.h"

void webserver_config_send_body_post(WebServer &server, String& page_content) {
	String masked_pwd;

	for (unsigned e = 0; e < sizeof(configShape)/sizeof(configShape[0]); ++e) {
		ConfigShapeEntry c;
		memcpy_P(&c, &configShape[e], sizeof(ConfigShapeEntry));
		const String s_param(c.cfg_key());
		if (!server.hasArg(s_param)) {
			continue;
		}
		const String server_arg(server.arg(s_param));

		switch (c.cfg_type) {
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
			if (server_arg.length()) {
				server_arg.toCharArray(c.cfg_val.as_str, LEN_CFG_PASSWORD);
			}
			break;
		}
	}

	page_content += FPSTR(INTL_SENSOR_IS_REBOOTING);

	server.sendContent(page_content);
	page_content = emptyString;
}

/*****************************************************************
 * Webserver config: show config page                            *
 *****************************************************************/

void webserver_config_send_body_get(WebServer &server, String& page_content, bool wificonfig_loop) {
	auto add_form_checkbox = [&page_content](const ConfigShapeId cfgid, const String& info) {
		page_content += form_checkbox(cfgid, info, true);
	};

	auto add_form_checkbox_sensor = [&add_form_checkbox](const ConfigShapeId cfgid, __const __FlashStringHelper* info) {
		add_form_checkbox(cfgid, add_sensor_type(info));
	};


	// изменения в верстке
	debug_outln_info(F("begin webserver_config_body_get ..."));
	page_content += F("<form method='POST' action='/config' style='width:100%;'>\n"
  "<div class='tabs'>"
	"<div class='tab' onclick='showPanel(1)'>" INTL_WIFI_SETTINGS "</div>"
	"<div class='tab' onclick='showPanel(2)'>");
	page_content += FPSTR(INTL_MORE_SETTINGS);
	page_content += F("</div>"
		"<div class='tab' onclick='showPanel(3)'>APIs"
		"</div></div>"
		"<div class='panel' id='panel1'>");

	if (wificonfig_loop) {  // scan for wlan ssids
		page_content += F("<div id='wifilist'>" INTL_WIFI_NETWORKS "</div><br/>");
	}
	page_content += "<div class='panel-container'>";
	add_form_input(page_content, Config_wlanssid, FPSTR(INTL_FS_WIFI_NAME), LEN_WLANSSID-1);
	add_form_input(page_content, Config_wlanpwd, FPSTR(INTL_PASSWORD), LEN_CFG_PASSWORD-1);
	page_content += form_checkbox(Config_wlannopwd, FPSTR(INTL_NO_WLAN_PWD), false);
	page_content += F("<hr/>\n<br/><b>");

	page_content += FPSTR(INTL_AB_HIER_NUR_ANDERN);
	page_content += FPSTR(WEB_B_BR);
	page_content += FPSTR(BR_TAG);

	// Paginate page after ~ 1500 Bytes
	server.sendContent(page_content);
	page_content = emptyString;
	page_content += "</div>";
	page_content += "<div class='panel-container'>";
	add_form_checkbox(Config_www_basicauth_enabled, FPSTR(INTL_BASICAUTH));
	add_form_input(page_content, Config_www_username, FPSTR(INTL_USER), LEN_WWW_USERNAME-1);
	add_form_input(page_content, Config_www_password, FPSTR(INTL_PASSWORD), LEN_CFG_PASSWORD-1);
	page_content += FPSTR(BR_TAG);

	// Paginate page after ~ 1500 Bytes
	server.sendContent(page_content);

	page_content += "</div>";
	if (! wificonfig_loop) {
		page_content += "<div class='panel-container'>";
		page_content = FPSTR(INTL_FS_WIFI_DESCRIPTION);
		page_content += FPSTR(BR_TAG);

		add_form_input(page_content, Config_fs_ssid, FPSTR(INTL_FS_WIFI_NAME), LEN_FS_SSID-1);
		add_form_input(page_content, Config_fs_pwd, FPSTR(INTL_PASSWORD), LEN_CFG_PASSWORD-1);

		// Paginate page after ~ 1500 Bytes
		server.sendContent(page_content);
	}
	page_content += "</div>";

	page_content = tmpl(FPSTR(WEB_DIV_PANEL), String(2));

	// Paginate page after ~ 1500 Bytes
	server.sendContent(page_content);
	page_content = emptyString;

	page_content += "<div class='panel-container'>";
	page_content = FPSTR(WEB_BR_LF_B);
	page_content += F(INTL_FIRMWARE "</b>&nbsp;");
	add_form_checkbox(Config_auto_update, FPSTR(INTL_AUTO_UPDATE));
	//add_form_checkbox(Config_use_beta, FPSTR(INTL_USE_BETA));

	page_content += form_select_lang();

	page_content += form_select_reg();

	page_content += F("<script>"
	    "var $ = function(e) { return document.getElementById(e); };"
	    "function updateOTAOptions() { "
		"$('current_lang').disabled = $('use_beta').disabled = !$('auto_update').checked; "
		"}; updateOTAOptions(); $('auto_update').onchange = updateOTAOptions;"
		"</script>");

	// скрипт для табов
		page_content += F("<script>"
			"function showPanel(panelIndex) {"
				"var panels = document.querySelectorAll('.panel');"
				"var tabs = document.querySelectorAll('.tab');"
					"panels.forEach(function(panel) { panel.classList.remove('active'); });"
					"tabs.forEach(function(tab) { tab.style.background = '#f4f4f4'; });"
					"panels[panelIndex - 1].classList.add('active');"
					"tabs[panelIndex - 1].style.background = '#ddd';"
			"}"
				"showPanel(1);"
			"</script>");

	page_content += "</div>";
	page_content += "<div class='panel-container'>";
	add_form_input(page_content, Config_debug, FPSTR(INTL_DEBUG_LEVEL), 1);
	add_form_input(page_content, Config_sending_intervall_ms, FPSTR(INTL_MEASUREMENT_INTERVAL), 5);
	add_form_input(page_content, Config_time_for_wifi_config, FPSTR(INTL_DURATION_ROUTER_MODE), 5);
	page_content += "</div>";

	page_content += "<div class='panel-container'>";
	page_content += FPSTR(WEB_GPS);
	page_content += FPSTR(WEB_B_BR);	
	page_content += FPSTR(BR_TAG);

	add_form_input(page_content, Config_coords_gps, FPSTR(INTL_COORDS), LEN_GPS_COORDS-1);
	page_content += "</div>";

	// Paginate page after ~ 1500 Bytes
	server.sendContent(page_content);
	page_content = tmpl(FPSTR(WEB_DIV_PANEL), String(3));

	page_content += "<div class='panel-container'>";
	page_content += tmpl(FPSTR(INTL_SEND_TO), F("APIs"));
	page_content += FPSTR(BR_TAG);
	page_content += form_checkbox(Config_send2robonomics, FPSTR(WEB_ROBONOMICS), false);
	page_content += FPSTR(WEB_BRACE_BRE);
	add_form_input(page_content, Config_rws_owner, FPSTR(INTL_RWS_OWNER), LEN_RWS_OWNER-1);
	add_form_input(page_content, Config_datalog_sending_intervall_ms, FPSTR(INTL_DATALOG_SENDING_INTERVAL), 5);
	add_form_input(page_content, Config_robonomics_public_node, FPSTR(INTL_ROBONOMICS_PUBLIC_NODE), LEN_ROBONOMICS_PUBLIC_NODE-1);
	
	add_form_checkbox(Config_send2csv, FPSTR(WEB_CSV));

	page_content += FPSTR(BR_TAG);
	page_content += "</div>";
	page_content += "<div class='panel-container'>";
	page_content += form_checkbox(Config_send2custom, FPSTR(INTL_SEND_TO_OWN_API), false);
	page_content += FPSTR(WEB_NBSP_NBSP_BRACE);
	page_content += form_checkbox(Config_ssl_custom, FPSTR(WEB_HTTPS), false);
	page_content += FPSTR(WEB_BRACE_BR);

	server.sendContent(page_content);
	add_form_input(page_content, Config_host_custom, FPSTR(INTL_SERVER), LEN_HOST_CUSTOM-1);
	add_form_input(page_content, Config_url_custom, FPSTR(INTL_PATH), LEN_URL_CUSTOM-1);
	add_form_input(page_content, Config_port_custom, FPSTR(INTL_PORT), MAX_PORT_DIGITS);
	add_form_input(page_content, Config_user_custom, FPSTR(INTL_USER), LEN_USER_CUSTOM-1);
	add_form_input(page_content, Config_pwd_custom, FPSTR(INTL_PASSWORD), LEN_CFG_PASSWORD-1);

	page_content += FPSTR(BR_TAG);
	page_content += "</div>";
	server.sendContent(page_content);
	
	page_content += "<div class='panel-container'>";
	page_content = form_checkbox(Config_send2influx, tmpl(FPSTR(INTL_SEND_TO), F("InfluxDB")), false);

	page_content += FPSTR(WEB_NBSP_NBSP_BRACE);
	page_content += form_checkbox(Config_ssl_influx, FPSTR(WEB_HTTPS), false);
	page_content += FPSTR(WEB_BRACE_BR);
	add_form_input(page_content, Config_host_influx, FPSTR(INTL_SERVER), LEN_HOST_INFLUX-1);
	add_form_input(page_content, Config_url_influx, FPSTR(INTL_PATH), LEN_URL_INFLUX-1);
	add_form_input(page_content, Config_port_influx, FPSTR(INTL_PORT), MAX_PORT_DIGITS);
	add_form_input(page_content, Config_user_influx, FPSTR(INTL_USER), LEN_USER_INFLUX-1);
	add_form_input(page_content, Config_pwd_influx, FPSTR(INTL_PASSWORD), LEN_CFG_PASSWORD-1);
	add_form_input(page_content, Config_measurement_name_influx, FPSTR(INTL_MEASUREMENT), LEN_MEASUREMENT_NAME_INFLUX-1);
	page_content += "</div>";
	page_content += F("</div></div>");
	page_content += form_submit(FPSTR(INTL_SAVE_AND_RESTART));
	page_content += FPSTR(BR_TAG);
	page_content += FPSTR(WEB_BR_FORM);
	if (wificonfig_loop) {  // scan for wlan ssids
		page_content += F("<script>window.setTimeout(load_wifi_list,1000);</script>");
	}

	server.sendContent(page_content);
	page_content = emptyString;
}