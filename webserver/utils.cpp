#include "utils.h"
#include "html-content.h"
#include "../utils.h"
#include "../intl.h"
#include <ArduinoJson.h>
#include "../config_manager/config_helpers.h"


void add_table_row_from_value(String& page_content, String& sensor, String& param, const String& value, const String& unit) {
	RESERVE_STRING(s, MED_STR);
	s = F("<tr><td>{s}</td><td>{p}</td><td class='r'>{v}&nbsp;{u}</td></tr>");
	s.replace("{s}", sensor);
	s.replace("{p}", param);
	s.replace("{v}", value);
	s.replace("{u}", unit);
	page_content += s;
}

void add_table_row_from_value(String& page_content, const __FlashStringHelper* param, const String& value, const char* unit) {
	RESERVE_STRING(s, MED_STR);
	s = F("<tr><td>{p}</td><td class='r'>{v}&nbsp;{u}</td></tr>");
	s.replace("{p}", param);
	s.replace("{v}", value);
	s.replace("{u}", String(unit));
	page_content += s;
}

void add_table_row_from_value(String& page_content, const String& param, const String& value, const char* unit = nullptr) {
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

String form_checkbox(const ConfigShapeId cfgid, const String& info, const bool linebreak) {
	RESERVE_STRING(s, MED_STR);
	s = F("<label for='{n}'>"
	"<input type='checkbox' name='{n}' value='1' id='{n}' {c}/>"
	"<input type='hidden' name='{n}' value='0'/>"
	"{i}</label><br/>");
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
	String s = F(	"<tr>"
					"<td>&nbsp;</td>"
					"<td>"
					"<input type='submit' name='submit' value='{v}' />"
					"</td>"
					"</tr>");
	s.replace("{v}", value);
	return s;
}

String form_select_lang() {
	String s_select = F(" selected='selected'");
	String s = F(	"<tr>"
					"<td>" INTL_LANGUAGE ":&nbsp;</td>"
					"<td>"
					"<select id='current_lang' name='current_lang'>"
					"<option value='EN'>English (EN)</option>"
					"<option value='RU'>Русский (RU)</option>"
					"</select>"
					"</td>"
					"</tr>");

	s.replace("'" + String(cfg::current_lang) + "'>", "'" + String(cfg::current_lang) + "'" + s_select + ">");
	return s;
}

String form_select_reg() {
	String s_select = F(" selected='selected'");
	String s = F(	"<tr>"
					"<td>" INTL_REGION ":&nbsp;</td>"
					"<td>"
					"<select id='current_reg' name='current_reg'>"
					"<option value='" INTL_REGION_GLOBAL "'>" INTL_REGION_GLOBAL "</option>"
					"<option value='" INTL_REGION_EU "'>" INTL_REGION_EU "</option>"
					"<option value='" INTL_REGION_AS "'>" INTL_REGION_AS "</option>"
					"<option value='" INTL_REGION_AF "'>" INTL_REGION_AF "</option>"
					"<option value='" INTL_REGION_AU "'>" INTL_REGION_AU "</option>"
					"<option value='" INTL_REGION_NA "'>" INTL_REGION_NA "</option>"
					"<option value='" INTL_REGION_SA "'>" INTL_REGION_SA "</option>"
					"</select>"
					"</td>"
					"</tr>");

	s.replace("'" + String(cfg::current_reg) + "'>", "'" + String(cfg::current_reg) + "'" + s_select + ">");
	return s;
}

void add_form_input(String& page_content, const ConfigShapeId cfgid, const __FlashStringHelper* info, const int length) {
	RESERVE_STRING(s, MED_STR);
	s = F("<tr>"
			"<td title='[&lt;= {l}]'>{i}:&nbsp;</td>"
			"<td style='width:{l}em'>"
			"<input type='{t}' name='{n}' id='{n}' placeholder='{i}' value='{v}' maxlength='{l}'/>"
			"</td></tr>");
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