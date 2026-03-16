#include "robonomics_http_api.h"
#include "../defines.h"
#include "../utils.h"
#include "helpers/message_formatter.h"
#include "../config_manager/config_helpers.h"
#include <WiFi.h>

void RobonomicsHTTPAPI::setup() {
	api_name = "Robonomics Map";
    _client = new WiFiClient();
    esp_chipid = get_chipid();
    donated_by = cfg::donated_by;
    rws_owner = cfg::rws_owner;
	current_reg = cfg::current_reg;
	timeout = getConfigUintValue("sending_intervall_ms");
	debug_outln_info(F("Robonomics HTTP API is ready with sending interval (sec): "), String(timeout/1000));
}

void RobonomicsHTTPAPI::_send(JsonDocument &data) {
    int num_of_host;
	String data_to_send;
	if (WiFi.status() != WL_CONNECTED) {
		debug_outln_error(F("[Map] Skipping send: WiFi is disconnected"));
		is_ok = false;
		return;
	}
	formatDataToSend(data_to_send, data);
    debug_outln_verbose(F("[Map] Payload: "), data_to_send);
	is_ok = false;
    num_of_host = chooseRobonomicsServer(false);
    if (num_of_host == 255) {
        debug_outln_verbose(F("[Map] No regional server found, trying global..."));
        num_of_host = chooseRobonomicsServer(true);
    }
    if (num_of_host != 255) {
        POSTRequest(data_to_send, HOST_ROBONOMICS[num_of_host][0]);
    } else {
        debug_outln_error(F("[Map] FAILED: No server available (all hosts unreachable or returned errors)"));
    }
}

void RobonomicsHTTPAPI::formatDataToSend(String &data_to_send, JsonDocument &data) {
	double last_value_GPS_lat = 0.0;
	double last_value_GPS_lon = 0.0;
	String datalog_data;
	int parsed = sscanf(cfg::coords_gps, "%lf,%lf", &last_value_GPS_lat, &last_value_GPS_lon);
	if (parsed != 2 || (last_value_GPS_lat == 0.0 && last_value_GPS_lon == 0.0)) {
		debug_outln_error(F("[Map] WARNING: GPS coordinates missing or invalid, raw value: "));
		debug_outln_verbose(F("[Map] coords_gps = "), String(cfg::coords_gps));
	} else {
		debug_outln_verbose(F("[Map] GPS lat="), String(last_value_GPS_lat, 6));
		debug_outln_verbose(F("[Map] GPS lon="), String(last_value_GPS_lon, 6));
	}
	formatRobonomicsString(data, datalog_data);
	if (datalog_data.length() == 0) {
		debug_outln_error(F("[Map] WARNING: sensor data string is empty (all sharing disabled or no sensor data?)"));
	}
    String signature;
	addTimeAndSign(datalog_data, signature, robonomics);
	if (signature.length() == 0) {
		debug_outln_error(F("[Map] WARNING: signature is empty (time not synced or signing failed)"));
	}
    data_to_send = F("{\"robonomics_address\": \"");
    data_to_send += robonomics->getSs58Address();
    data_to_send += "\", \"donated_by\": \"";
    data_to_send += donated_by;
    data_to_send += "\", \"owner\": \"";
    data_to_send += rws_owner;
    data_to_send += "\", \"signature\": \"";
    data_to_send += signature;
    data_to_send += "\", \"GPS_lat\": \"";
    data_to_send += String(last_value_GPS_lat, 6);
    data_to_send += "\", \"GPS_lon\": \"";
    data_to_send += String(last_value_GPS_lon, 6);
    data_to_send += "\", \"sensordatavalues\": \"";
    data_to_send += datalog_data;
    data_to_send += "\"}";
}

void RobonomicsHTTPAPI::POSTRequest(const String& data, const char* host) {
	HTTPClient _http;
	String SOFTWARE_VERSION(SOFTWARE_VERSION_STR);
    int result = 0;
	if (WiFi.status() != WL_CONNECTED) {
		debug_outln_error(F("[Map] POST skipped: WiFi disconnected"));
		return;
	}
    String s_Host(FPSTR(host));
	String s_url(FPSTR(URL_ROBONOMICS));
	debug_outln_verbose(F("[Map] POST to "), s_Host + ":" + String(PORT_ROBONOMICS) + s_url);
    _http.setTimeout(20 * 1000);
	_http.setUserAgent(SOFTWARE_VERSION + '/' + esp_chipid);
    _http.setReuse(false);
    if (_http.begin(*_client, s_Host, PORT_ROBONOMICS, s_url)) {
        _http.addHeader(F("Content-Type"), "application/json");
		_http.addHeader(F("X-Sensor"), String(F(SENSOR_BASENAME)) + esp_chipid);
        result = _http.POST(data);
        if (result >= HTTP_CODE_OK && result <= HTTP_CODE_ALREADY_REPORTED) {
			debug_outln_info(F("[Map] OK, POST succeeded -> "), s_Host);
			is_ok = true;
		} else if (result >= HTTP_CODE_BAD_REQUEST) {
			debug_outln_error(F("[Map] FAILED: server returned HTTP error"));
			debug_outln_verbose(F("[Map] HTTP code: "), String(result));
			debug_outln_verbose(F("[Map] Response body: "), _http.getString());
		} else {
			debug_outln_error(F("[Map] FAILED: HTTP error (connection/timeout)"));
			debug_outln_verbose(F("[Map] Error code: "), String(result));
			debug_outln_verbose(F("[Map] Details: "), HTTPClient::errorToString(result));
		}
        _http.end();
    } else {
		debug_outln_error(F("[Map] FAILED: could not begin HTTP connection"));
		debug_outln_verbose(F("[Map] Host: "), s_Host);
	}
}

int RobonomicsHTTPAPI::chooseRobonomicsServer(bool onlyGlobal) {
	HTTPClient _http;
	int num_of_robonomics_host = 255;
	int min_sensors = 255;
	int result = 0;
	String s_url = FPSTR(URL_ROBONOMICS);
	int numRobonomicsHosts = sizeof(HOST_ROBONOMICS) / sizeof(HOST_ROBONOMICS[0]);
	debug_outln_verbose(F("[Map] Selecting server: "), String(numRobonomicsHosts) + " hosts, region=" + current_reg.c_str() + (onlyGlobal ? " (global only)" : ""));

	for (int i = 0; i < numRobonomicsHosts; i++) {
		if (WiFi.status() != WL_CONNECTED) {
			debug_outln_error(F("[Map] Stop server selection: WiFi disconnected"));
			break;
		}
		if (onlyGlobal) {
			if (strcmp(HOST_ROBONOMICS[i][1], INTL_REGION_GLOBAL) != 0) {
				continue;
			}
		} else if (strcmp(current_reg.c_str(), HOST_ROBONOMICS[i][1]) != 0) {
			continue;
		}
		String s_Host = FPSTR(HOST_ROBONOMICS[i][0]);
		debug_outln_verbose(F("[Map] Trying GET "), s_Host + ":" + String(PORT_ROBONOMICS));

		if (_http.begin(*_client, s_Host, PORT_ROBONOMICS, s_url)) {
			const char * headerKeys[] = {"sensors-count", "on-server"} ;
			const size_t numberOfHeaders = 2;
			_http.collectHeaders(headerKeys, numberOfHeaders);
			_http.addHeader("Sensor-id", robonomics->getSs58Address());

			result = _http.GET();

			if (result >= HTTP_CODE_OK && result <= HTTP_CODE_ALREADY_REPORTED) {
				String header = _http.header("sensors-count");
				String on_server = _http.header("on-server");
				int num = atoi(header.c_str());
				debug_outln_verbose(F("[Map] OK from "), s_Host + " sensors=" + header + " on_server=" + on_server);
				if (on_server == "True") {
					num_of_robonomics_host = i;
					_http.end();
					break;
				}
				if (num < min_sensors) {
					min_sensors = num;
					num_of_robonomics_host = i;
				}
			} else if (result >= HTTP_CODE_BAD_REQUEST) {
				debug_outln_verbose(F("[Map] Server error from "), s_Host + " HTTP " + String(result));
				debug_outln_verbose(F("[Map] Response: "), _http.getString());
			} else {
				debug_outln_verbose(F("[Map] Connection error to "), s_Host + " code=" + String(result) + " " + HTTPClient::errorToString(result));
			}
			_http.end();

		} else {
			debug_outln_error(F("[Map] Cannot connect to host"));
			debug_outln_verbose(F("[Map] Host: "), s_Host);
		}
	}
	if (num_of_robonomics_host < numRobonomicsHosts) {
		debug_outln_verbose(F("[Map] Selected server: "), HOST_ROBONOMICS[num_of_robonomics_host][0]);
	} else {
		debug_outln_error(F("[Map] No suitable server found among all hosts"));
	}
	
	return num_of_robonomics_host;
}