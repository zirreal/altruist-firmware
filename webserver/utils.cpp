#include "utils.h"
#include "html-content.h"
#include "../utils.h"
#include "../intl.h"
#include <ArduinoJson.h>

namespace {

String data_row_unit_html(const String& unit) {
	if (unit.length() == 0) {
		return emptyString;
	}
	RESERVE_STRING(s, SMALL_STR);
	s = F("<span class='data-line__unit'>");
	s += unit;
	s += F("</span>");
	return s;
}

void append_data_block_open(String& page_content, const String& label, const char* block_modifier) {
	page_content += F("<div class='data-block");
	if (block_modifier && block_modifier[0] != '\0') {
		page_content += ' ';
		page_content += block_modifier;
	}
	page_content += F("'><h3 class='data-block__title'>");
	page_content += label;
	page_content += F("</h3><div class='data-block__rows'>");
}

} // namespace

void web_page_flush_chunk(String& page_content, WebServer* server) {
	if (!server || page_content.length() == 0) {
		return;
	}
	server->sendContent(page_content);
	page_content = emptyString;
	markMainLoopAlive();
	yield();
}

void web_send_content_progmem(WebServer* server, const char* data, size_t len) {
	if (!server || !data || len == 0) {
		return;
	}
	constexpr size_t kChunk = 512;
	char buf[kChunk];
	for (size_t i = 0; i < len; ) {
		const size_t n = (len - i > kChunk) ? kChunk : (len - i);
		memcpy_P(buf, data + i, n);
		server->sendContent(buf, n);
		i += n;
		markMainLoopAlive();
		yield();
	}
}

void web_page_finish_chunked(WebServer* server) {
	if (!server) {
		return;
	}
	server->sendContent(emptyString);
}

void add_data_row_from_value(String& page_content, const __FlashStringHelper* param, const String& value, const char* unit) {
	RESERVE_STRING(s, MED_STR);
	s = F("<div class='data-line'><span class='data-line__name'>{p}</span><span class='data-line__reading'><span class='data-line__val'>{v}</span>");
	s.replace("{p}", param);
	s.replace("{v}", value);
	page_content += s;
	page_content += data_row_unit_html(String(unit));
	page_content += F("</span></div>");
}

void add_data_row_from_value(String& page_content, const __FlashStringHelper* param, const __FlashStringHelper* value, const char* unit) {
	RESERVE_STRING(s, MED_STR);
	s = F("<div class='data-line'><span class='data-line__name'>{p}</span><span class='data-line__reading'><span class='data-line__val'>{v}</span>");
	s.replace("{p}", param);
	s.replace("{v}", value);
	page_content += s;
	page_content += data_row_unit_html(String(unit));
	page_content += F("</span></div>");
}

void add_data_row_from_value(String& page_content, const String& param, const String& value, const char* unit) {
	RESERVE_STRING(s, MED_STR);
	s = F("<div class='data-line'><span class='data-line__name'>{p}</span><span class='data-line__reading'><span class='data-line__val'>{v}</span>");
	s.replace("{p}", param);
	s.replace("{v}", value);
	page_content += s;
	page_content += data_row_unit_html(String(unit));
	page_content += F("</span></div>");
}

void add_data_section_start(String& page_content, const __FlashStringHelper* label, const char* block_modifier) {
	append_data_block_open(page_content, String(label), block_modifier);
}

void add_data_section_start(String& page_content, const String& label, const char* block_modifier) {
	append_data_block_open(page_content, label, block_modifier);
}

void add_data_section_end(String& page_content) {
	page_content += F("</div></div>");
}

void add_reading_metrics_grid_start(String& page_content) {
	page_content += F("<div class='reading-grid'>");
}

void add_reading_metrics_grid_end(String& page_content) {
	page_content += F("</div>");
}

void add_reading_metric_card(String& page_content, const __FlashStringHelper* label, const String& value, const char* unit) {
	page_content += F("<div class='reading-card'><span class='reading-card__label'>");
	page_content += label;
	page_content += F("</span><span class='reading-card__value'>");
	page_content += value;
	page_content += F("</span>");
	if (unit != nullptr && unit[0] != '\0') {
		page_content += F("<span class='reading-card__unit'>");
		page_content += unit;
		page_content += F("</span>");
	}
	page_content += F("</div>");
}

void add_reading_metric_card(String& page_content, const String& label, const String& value, const char* unit) {
	page_content += F("<div class='reading-card'><span class='reading-card__label'>");
	page_content += label;
	page_content += F("</span><span class='reading-card__value'>");
	page_content += value;
	page_content += F("</span>");
	if (unit != nullptr && unit[0] != '\0') {
		page_content += F("<span class='reading-card__unit'>");
		page_content += unit;
		page_content += F("</span>");
	}
	page_content += F("</div>");
}

void add_data_block_intro(String& page_content, const __FlashStringHelper* intro) {
	page_content += F("<p class='data-block__intro'>");
	page_content += intro;
	page_content += F("</p>");
}

void add_data_api_status_row(String& page_content, const String& api_name, const String& status,
	const String& sends, const String& last_send) {
	page_content += F("<div class='data-api'><span class='data-api__name'>");
	page_content += api_name;
	page_content += F("</span><div class='data-api__details'>");
	if (status == "OK") {
		page_content += F("<span class='data-api__badge data-api__badge--ok'>OK</span>");
	} else {
		page_content += F("<span class='data-api__badge data-api__badge--err'>Error</span>");
	}
	page_content += F("<span class='data-api__detail'><span class='data-api__detail-lbl'>");
	page_content += FPSTR(INTL_API_SENDS_SHORT);
	page_content += F("</span>");
	page_content += sends;
	page_content += F("</span><span class='data-api__detail'><span class='data-api__detail-lbl'>");
	page_content += FPSTR(INTL_API_LAST_SHORT);
	page_content += F("</span>");
	page_content += last_send;
	page_content += F("</span></div></div>");
}

void add_table_row_from_value(String& page_content, const __FlashStringHelper* param, const String& value, const char* unit) {
	RESERVE_STRING(s, MED_STR);
	s = F("<tr><td>{p}</td><td class='r'>{v}&nbsp;{u}</td></tr>");
	s.replace("{p}", param);
	s.replace("{v}", value);
	s.replace("{u}", String(unit));
	page_content += s;
}

void add_table_row_from_value(String& page_content, const __FlashStringHelper* param, const __FlashStringHelper* value, const char* unit) {
	RESERVE_STRING(s, MED_STR);
	s = F("<tr><td>{p}</td><td class='r'>{v}&nbsp;{u}</td></tr>");
	s.replace("{p}", param);
	s.replace("{v}", value);
	s.replace("{u}", String(unit));
	page_content += s;
}

void add_table_row_from_value(String& page_content, const String& param, const String& value, const char* unit) {
    RESERVE_STRING(s, MED_STR);
	s = F("<tr><td>{p}</td><td class='r'>{v}&nbsp;{u}</td></tr>");
	s.replace("{p}", param);
	s.replace("{v}", value);
	s.replace("{u}", String(unit));
	page_content += s;
}

int32_t calcWiFiSignalQuality(int32_t rssi) {
	// Treat 0 or positive values as 0%
	if (rssi >= 0 || rssi < -100) {
		rssi = -100;
	}
	if (rssi > -50) {
		rssi = -50;
	}
	return (rssi + 100) * 2;
}

String wlan_ssid_to_table_row(const String& ssid, const String& encryption, int32_t rssi) {
	String s = F(	"<tr>"
					"<td>"
					"<a href='#wlanpwd' onclick='setSSID(this)' class='wifi'>{n}</a>&nbsp;{e}"
					"</td>"
					"<td style='vertical-align:middle;'>"
					"{v}%"
					"</td>"
					"</tr>");
	s.replace("{n}", ssid);
	s.replace("{e}", encryption);
	s.replace("{v}", String(calcWiFiSignalQuality(rssi)));
	return s;
}

String add_sensor_type(const String& sensor_text) {
	RESERVE_STRING(s, SMALL_STR);
	s = sensor_text;
	s.replace("{pm}", FPSTR(INTL_PARTICULATE_MATTER));
	s.replace("{t}", FPSTR(INTL_TEMPERATURE));
	s.replace("{h}", FPSTR(INTL_HUMIDITY));
	s.replace("{p}", FPSTR(INTL_PRESSURE));
	s.replace("{l_a}", FPSTR(INTL_LEQ_A));
	return s;
}

String form_checkbox(const ConfigShapeId cfgid, const String& info, const bool linebreak, bool enabled) {
	RESERVE_STRING(s, MED_STR);
	if (enabled) {
		s = F("<div class='form-group'><label for='{n}'>"
		"<input type='checkbox' name='{n}' value='1' id='{n}' {c}/>"
		"<input type='hidden' name='{n}' value='0'/>"
		"{i}</label></div><br/>");
	} else {
		s = F("<div class='form-group'><label for='{n}'>"
		"<input type='checkbox' name='{n}' value='1' id='{n}' {c} disabled/>"
		"<input type='hidden' name='{n}' value='0'/>"
		"{i}</label></div><br/>");
	}

	if (*configShape[cfgid].cfg_val.as_bool) {
		s.replace("{c}", F(" checked='checked'"));
	} else {
		s.replace("{c}", emptyString);
	};
	s.replace("{i}", info);
	s.replace("{n}", String(configShape[cfgid].cfg_key()));
	if (! linebreak) {
		s.replace("<br/>", emptyString);
	}
	return s;
}

String form_submit(const String& value) {
	String s = F(	"<button type='submit' class='submit-btn'>{v}</button>");
	s.replace("{v}", value);
	return s;
}

String form_select_lang() {
	String s_select = F(" selected='selected'");
	//INTL_LANGUAGE
	String s = F("<div class='form-group'>"
		"<label for='current_lang'>" INTL_LANGUAGE "</label>"
		"<select id='current_lang' name='current_lang'>"
				"<option value='EN'>English (EN)</option>"
				"<option value='RU'>Русский (RU)</option>"
		"</select>"
		"</div>");

	s.replace("'" + String(cfg::current_lang) + "'>", "'" + String(cfg::current_lang) + "'" + s_select + ">");
	return s;
}

String form_select_altruist(JsonDocument& data) {
	String s_select = F(" selected='selected'");
	String s = F("<div class='form-group'>"
				"<label for='chosen_altruist_urban'>Altruist Urban</label>"
				"<div style='display:flex;gap:8px;align-items:center;'>"
				"<select id='chosen_altruist_urban' name='chosen_altruist_urban' style='flex:1;'>");

	JsonArray addresses = data["service_data"]["altruist_addresses"];
	for (JsonVariant v : addresses) {
		const String ip = v.as<String>();

		s += F("<option value='");
		s += ip;
		s += "'";

		if (ip == cfg::chosen_altruist_urban) {
			s += s_select;
		}

		s += ">";
		s += ip;
		s += "</option>";
	}

	s += F("</select>"
		"<button type='button' id='btn_scan_urbans' "
		"style='padding:6px 14px;border:1px solid #ccc;border-radius:4px;background:#f8f8f8;cursor:pointer;white-space:nowrap;'>"
		"&#x1F50D; " INTL_SCAN_BTN "</button></div>"
		"<span id='scan_status' style='font-size:12px;color:#666;'></span>"
		"</div>");
	return s;
}

String form_select_timezone() {
	String s_select = F(" selected='selected'");
	String s = F(
    "<div class='form-group'>"
    "<label for='timezone'>Timezone:</label>"
    "<select id='timezone' name='timezone'>"
        "<option value='<-12>12'>UTC-12</option>"
        "<option value='<-11>11'>UTC-11</option>"
        "<option value='<-10>10'>UTC-10</option>"
        "<option value='<-9>9'>UTC-9</option>"
        "<option value='<-8>8'>UTC-8</option>"
        "<option value='<-7>7'>UTC-7</option>"
        "<option value='<-6>6'>UTC-6</option>"
        "<option value='<-5>5'>UTC-5</option>"
        "<option value='<-4>4'>UTC-4</option>"
        "<option value='<-3>3'>UTC-3</option>"
        "<option value='<-2>2'>UTC-2</option>"
        "<option value='<-1>1'>UTC-1</option>"
        "<option value='<+00>0'>UTC+0</option>"
        "<option value='<+01>-1'>UTC+1</option>"
        "<option value='<+02>-2'>UTC+2</option>"
        "<option value='<+03>-3'>UTC+3</option>"
        "<option value='<+04>-4'>UTC+4</option>"
        "<option value='<+05>-5'>UTC+5</option>"
        "<option value='<+06>-6'>UTC+6</option>"
        "<option value='<+07>-7'>UTC+7</option>"
        "<option value='<+08>-8'>UTC+8</option>"
        "<option value='<+09>-9'>UTC+9</option>"
        "<option value='<+10>-10'>UTC+10</option>"
        "<option value='<+11>-11'>UTC+11</option>"
        "<option value='<+12>-12'>UTC+12</option>"
    "</select>"
    "</div>");

	s.replace("'" + String(cfg::timezone) + "'>", "'" + String(cfg::timezone) + "'" + s_select + ">");
	return s;
}

String form_select_reg() {
	String s_select = F(" selected='selected'");
	String s = F(	"<div class='form-group'>"
					"<label for='current_reg'>" INTL_REGION ":&nbsp</label>"
					"<select id='current_reg' name='current_reg'>"
					"<option value='" REGION_GLOBAL "'>" INTL_REGION_GLOBAL "</option>"
					"<option value='" REGION_EU "'>" INTL_REGION_EU "</option>"
					"<option value='" REGION_AS "'>" INTL_REGION_AS "</option>"
					"<option value='" REGION_AF "'>" INTL_REGION_AF "</option>"
					"<option value='" REGION_AU "'>" INTL_REGION_AU "</option>"
					"<option value='" REGION_NA "'>" INTL_REGION_NA "</option>"
					"<option value='" REGION_SA "'>" INTL_REGION_SA "</option>"
					"</select>"
					"</div>");

	s.replace("'" + String(cfg::current_reg) + "'>", "'" + String(cfg::current_reg) + "'" + s_select + ">");
	return s;
}

void add_form_input(String& page_content, const ConfigShapeId cfgid, const __FlashStringHelper* info, const int length, bool enabled) {
	RESERVE_STRING(s, MED_STR);
	if (enabled) {
		s = F("<div class='form-group'>"
				"<label for='{n}'>{i}</label>"
				"<input type='{t}' name='{n}' id='{n}' placeholder='{i}' value='{v}' maxlength='{l}'/>"
				"</div>");
	} else {
		s = F("<div class='form-group'>"
			"<label for='{n}'>{i}</label>"
			"<input type='{t}' name='{n}' id='{n}' placeholder='{i}' value='{v}' maxlength='{l}' disabled/>"
			"</div>");
	}
	String t_value;
	ConfigShapeEntry c;
	memcpy_P(&c, &configShape[cfgid], sizeof(ConfigShapeEntry));
	switch (c.cfg_type) {
	case Config_Type_UInt:
		t_value = String(*c.cfg_val.as_uint);
		s.replace("{t}", F("number"));
		break;
	case Config_Type_Time:
		t_value = String((*c.cfg_val.as_uint) / 1000);
		s.replace("{t}", F("number"));
		break;
	default:
		if (c.cfg_type == Config_Type_Password) {
			s.replace("{t}", F("password"));
			info = FPSTR(INTL_PASSWORD);
		} else {
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

void add_form_input(String& page_content, const ConfigShapeId cfgid, const __FlashStringHelper* info, const int length) {
    add_form_input(page_content, cfgid, info, length, true);
}

void append_app_page_body_start(String& page_content, const __FlashStringHelper* lead) {
	page_content += F("<div class='app-page-body'>");
	if (lead != nullptr) {
		page_content += F("<p class='app-page-lead'>");
		page_content += lead;
		page_content += F("</p>");
	}
}

void append_app_page_body_end(String& page_content) {
	page_content += F("</div>");
}