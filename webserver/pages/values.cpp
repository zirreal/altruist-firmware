#include "pages.h"
#include <WiFi.h>
#include "../utils.h"
#include "../../utils.h"
#include "../../intl.h"
#include "../html-content.h"
#include <ArduinoJson.h>

/*****************************************************************
 * Webserver root: show latest values                            *
 *****************************************************************/
void webserver_values(JsonDocument &data, String &page_content) {
	debug_outln_info(F("ws: values ..."));
    uint8_t signal_strength;

	// page_content = "<b>";
	// page_content += FPSTR(WEB_B_BR_BR);

	page_content = F("<table cellspacing='0' cellpadding='5' class='v'>\n"
			"<thead><tr><th>" INTL_SENSOR "</th><th> " INTL_PARAMETER "</th><th>" INTL_VALUE "</th></tr></thead>");

	for (JsonPair sensor : data.as<JsonObject>())  {
        String sensor_name = sensor.key().c_str();
        JsonObject sensorData = sensor.value().as<JsonObject>();

        if (sensor_name == "service_data") {
            signal_strength = sensorData["signal_strength"].as<uint8_t>();
            continue;
        }

        for (JsonPair measurement : sensorData) {
            String type = measurement.key().c_str();
            JsonObject measurementData = measurement.value().as<JsonObject>();
            String value = measurementData["value"].as<String>();
            String intl_param = measurementData["intl_name"].as<String>();
            String units = measurementData["units"].as<String>();

			add_table_row_from_value(page_content, sensor_name, intl_param, value, units.c_str());
        }
		page_content += FPSTR(EMPTY_ROW);
    }

	const int signal_quality = calcWiFiSignalQuality(signal_strength);
	// if (!count_sends) {
	// 	page_content += F("<b style='color:red'>");
	// 	add_warning_first_cycle(page_content);
	// 	page_content += FPSTR(WEB_B_BR_BR);
	// } else {
	// 	add_age_last_values(page_content);
	// }

	// server.sendContent(page_content);
	// page_content = emptyString;

	add_table_row_from_value(page_content, F("WiFi"), FPSTR(INTL_SIGNAL_STRENGTH), String(signal_strength), "dBm");
	add_table_row_from_value(page_content, F("WiFi"), FPSTR(INTL_SIGNAL_QUALITY), String(signal_quality), "%");

	page_content += FPSTR(TABLE_TAG_CLOSE_BR);
	page_content += FPSTR(BR_TAG);
}