#ifdef ALTRUIST_INSIDE

#include "http_altruist_sensor.h"
#include "../utils.h"
#include "../intl.h"
#include "../config_manager/config_helpers.h"
#include "sensor_names.h"
#include <ESPmDNS.h>

#define HTTP_ALTRUIST_SENSOR_MIN_TIMEOUT 300000UL  // 5 minutes
// Maximum discovery attempts when Urban is not initially present.
// We will try to find Urban up to this many times, spaced 5 minutes apart.
static const uint8_t URBAN_MAX_DISCOVERY_ATTEMPTS = 3;
static const unsigned long URBAN_REDISCOVER_INTERVAL_MS = 5UL * 60UL * 1000UL; // 5 minutes

HTTPAltruistSensor::HTTPAltruistSensor(unsigned long sending_timeout)
    : Sensor(sending_timeout) {
    if (sending_timeout > HTTP_ALTRUIST_SENSOR_MIN_TIMEOUT) {
    timeout = sending_timeout;
    } else {
    timeout = HTTP_ALTRUIST_SENSOR_MIN_TIMEOUT;
    }
    sensor_name = HTTP_ALTRUIST_SENSOR_NAME;
}

bool HTTPAltruistSensor::_discoverSensors() {
    // Bookkeeping for discovery attempts when we don't yet know an Urban IP.
    // We only increment the attempts counter if neither a discovered address
    // nor a configured chosen address is available.
    if (sensor_addresses.empty() && strlen(cfg::chosen_altruist_urban) == 0) {
        if (discovery_attempts < URBAN_MAX_DISCOVERY_ATTEMPTS) {
            discovery_attempts++;
        }
    } else {
        // If we already have some address info, reset attempts so future
        // "no Urban" phases can start their own limited sequence.
        discovery_attempts = 0;
    }
    last_discovery_attempt_time = millis();

    sensor_addresses.clear();
    debug_outln_info(F("HTTPAltruistSensor: discovering Urban devices via mDNS"));
    int nrOfServices = MDNS.queryService("altruist", "tcp");
   
    if (nrOfServices == 0) {
        debug_outln_info(F("No services were found."));
        return false;
    } 
    debug_outln_info(F("Number of services found: "), nrOfServices);

    bool found_chosen = false;

    for (int i = 0; i < nrOfServices; i++) {
        String ip_str = MDNS.address(i).toString();
        String device_type = DEVICE_MODEL_URBAN;
        if (MDNS.hasTxt(i, DEVICE_MODEL_MDNS_PROPERTY)) {
            device_type = MDNS.txt(i, DEVICE_MODEL_MDNS_PROPERTY);
        }

        debug_outln_info(F("---------------"));
        debug_outln_info(F("Hostname: "), MDNS.hostname(i));
        debug_outln_info(F("IP address: "), ip_str);
        debug_outln_info(F("Port: "), MDNS.port(i));
        debug_outln_info(F("Device type: "), device_type);
        debug_outln_info(F("---------------"));

        if (device_type == DEVICE_MODEL_URBAN) {
            sensor_addresses.push_back(ip_str);

            if (ip_str == cfg::chosen_altruist_urban) {
                found_chosen = true;
            }
        }
    }
    if (cfg::use_custom_urban) {
        chosen_address = String(cfg::custom_altruist_urban);
        debug_outln_info(F("Use custom altruist urban address "), chosen_address);
    } else {
        if (!found_chosen && !sensor_addresses.empty()) {
            config_set_string_by_key("chosen_altruist_urban", sensor_addresses[0].c_str());
            writeConfig();
            debug_outln_info(F("Chosen altruist sensor not found, using: "), cfg::chosen_altruist_urban);
        }
        chosen_address = String(cfg::chosen_altruist_urban);
    }
    debug_outln_info(F("Http Altruis Sensor started with fetch interval (sec): "), String(timeout/1000));
    last_fetch_time = millis() - timeout;
    return true;
}

bool HTTPAltruistSensor::begin() {
    debug_outln_info(F("Begin HTTPAltruistSensor"));
    if(mdns_init()!= ESP_OK){
        debug_outln_info(F("mDNS failed to start"));
        return false;
    }
    debug_outln_info(F("mDNS init finished"));

    bool ok = _discoverSensors();
    if (ok) {
        // Reset success / failure counters on fresh start
        last_success_time    = 0;
        consecutive_failures = 0;
    }
    return true;
}

void HTTPAltruistSensor::_fetch(JsonDocument &data) {
    debug_outln_info(F("fetch HTTP Altruist"));
    HTTPClient http;
    JsonArray addresses = data["service_data"].createNestedArray("altruist_addresses");
    for (const auto& ip_address : sensor_addresses) {
        bool already_exists = false;
        for (JsonVariant v : addresses) {
            if (v.as<String>() == ip_address) {
                already_exists = true;
                break;
            }
        }
        if (!already_exists) {
            addresses.add(ip_address);
        }
    }

    // If we don't yet know which Urban IP to use, try to (re)discover it
    // or fall back to the last configured IP before attempting an HTTP
    // request. This avoids calling HTTP with an empty host and makes sure we give Urban multiple chances to appear.
    if (chosen_address.length() == 0) {
        // 1) If a chosen Urban IP is already stored in config (from a previous successful run), use it directly even if mDNS hasn't found it yet.
        if (strlen(cfg::chosen_altruist_urban) != 0) {
            chosen_address = String(cfg::chosen_altruist_urban);
            debug_outln_info(F("HTTPAltruistSensor: using configured chosen_altruist_urban IP "),
                             chosen_address);
        } else {
            // 2) No configured IP at all: drive mDNS rediscovery up to a limited number of attempts, spaced in time.
            if (discovery_attempts < URBAN_MAX_DISCOVERY_ATTEMPTS) {
                bool first_attempt     = (discovery_attempts == 0);
                bool interval_elapsed = (last_discovery_attempt_time == 0) ||
                                        (msSince(last_discovery_attempt_time) >= URBAN_REDISCOVER_INTERVAL_MS);

                if (first_attempt || interval_elapsed) {
                    debug_outln_info(F("HTTPAltruistSensor: proactive rediscovery from _fetch, attempt "),
                                     String(discovery_attempts + 1));
                    if (_discoverSensors()) {
                        // After rediscovery, chosen_address may now be populated
                        consecutive_failures = 0;
                    }
                }
            }
        }
    }

    // If we still don't have a target Urban address, skip this cycle.
    if (chosen_address.length() == 0) {
        return;
    }

    _fetch_one_sensor(data, http, chosen_address);
    // sensor_name = HTTP_ALTRUIST_SENSOR_NAME;
}

void HTTPAltruistSensor::_fetch_one_sensor(JsonDocument &data, HTTPClient& http, const String &ip_address) {
    debug_outln_info(F("fetch HTTP Altruist "), ip_address);
    String sensor_url = SENSOR_URL_PREFIX + ip_address + JSON_DATA_PATH;
    http.begin(sensor_url);
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
        debug_outln_info(F("Success request to Altruis Urban"));

        // Ensure we have a dedicated Urban block in the global sensors_data:
        // "altruist_urban": { IP_address, SDS_P1, SDS_P2, ... }
        // It is pre-created at boot in setup(), but if for some reason it's missing, create it here without clearing the rest of sensors_data.
        JsonObject urbanRoot = data[ATRUIST_URBAN_SENSOR];
        if (urbanRoot.isNull()) {
            debug_outln_info(F("HTTPAltruistSensor: altruist_urban missing in sensors_data, creating on the fly"));
            urbanRoot = data.createNestedObject(ATRUIST_URBAN_SENSOR);
            if (urbanRoot.isNull()) {
                debug_outln_info(F("HTTPAltruistSensor: FAILED to create altruist_urban (JSON memory issue)"));
                serializeJson(data, Serial);
                http.end();
                return;
            }
        }

        // Store IP address inside the Urban block in the same "value/int_name/units" shape
        {
            JsonObject ipObj = urbanRoot["IP_address"];
            if (ipObj.isNull()) {
                ipObj = urbanRoot.createNestedObject("IP_address");
            }
            ipObj[F("value")]    = ip_address;
            ipObj[F("intl_name")] = INTL_IP_ADDRESS;
            ipObj[F("units")]     = "";
        }

        // Parse Urban's /data.json payload and map sensordatavalues into altruist_urban
        String payload = http.getString();
        DynamicJsonDocument doc(2048);
        DeserializationError err = deserializeJson(doc, payload);
        if (err) {
            debug_outln_info(F("JSON parse error: "), err.c_str());
            http.end();
            return;
        }

        JsonArray values = doc["sensordatavalues"];
        debug_outln_info(F("HTTPAltruistSensor: sensordatavalues count "),
                         String(values.size()));

        for (JsonObject v : values) {
            String type  = v["value_type"];
            float  value = v["value"].as<float>();

            // Пример соответствия intl_name и units
            String units;
            String intl_name;
            if (type == "SDS_P1") {
                intl_name = "PM10";
                units = F("µg/m³");
            } else if (type == "SDS_P2") {
                intl_name = "PM2.5";
                units = F("µg/m³");
            } else if (type == "BME280_temperature") {
                intl_name = INTL_TEMPERATURE;
                units = F("°C");
            } else if (type == "BME280_humidity") {
                intl_name = INTL_HUMIDITY;
                units = F("%");
            } else if (type == "BME280_pressure") {
                intl_name = INTL_PRESSURE;
                units = F("Pa");
            } else if (type == "PCBA_noiseMax") {
                intl_name = INTL_NOISE_MAX;
                units = F("db");
            } else if (type == "PCBA_noiseAvg") {
                intl_name = INTL_NOISE_MEAN;
                units = F("db");
            } else {
                // Unknown/extra types are still stored so they are visible in JSON
                intl_name = type;
                units = "";
            }

            JsonObject measObj = urbanRoot[type];
            if (measObj.isNull()) {
                measObj = urbanRoot.createNestedObject(type);
            }
            measObj[F("value")]     = value;
            measObj[F("intl_name")] = intl_name;
            measObj[F("units")]     = units;
        }
        // Capture Urban device's Robonomics address from data.json, or fallback to HTML extraction
        bool has_urban_addr = false;
        if (doc.containsKey("service_data") && doc["service_data"].containsKey("robonomics_address")) {
            String urban_addr = doc["service_data"]["robonomics_address"].as<String>();
            if (urban_addr.length() > 0) {
                JsonObject service = data["service_data"].isNull() ? data.createNestedObject("service_data") : data["service_data"].as<JsonObject>();
                service["urban_robonomics_address"] = urban_addr;
                has_urban_addr = true;
            }
        }
        if (!has_urban_addr) {
            HTTPClient http2;
            String root_url = SENSOR_URL_PREFIX + ip_address + String("/");
            http2.begin(root_url);
            int httpCode2 = http2.GET();
            if (httpCode2 == HTTP_CODE_OK) {
                String html = http2.getString();
                // Fallback heuristic: first base58-like sequence starting with '4'
                const char *base58 = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
                for (size_t i = 0; i < html.length(); ++i) {
                    if (html[i] == '4') {
                        size_t j = i;
                        while (j < html.length() && strchr(base58, html[j])) {
                            ++j;
                        }
                        size_t len = j - i;
                        if (len >= 47 && len <= 50) { // accept typical SS58 length range
                            String urban_addr = html.substring(i, j);
                            JsonObject service = data["service_data"].isNull() ? data.createNestedObject("service_data") : data["service_data"].as<JsonObject>();
                            service["urban_robonomics_address"] = urban_addr;
                            break;
                        }
                    }
                }
            }
            http2.end();
        }
        serializeJson(data, Serial);

        // Mark successful communication with Urban
        last_success_time    = millis();
        consecutive_failures = 0;
    } else {
        debug_outln_info(F("Request to Altruist Urban failed, code: "), httpCode);
        consecutive_failures++;
        bool never_succeeded = (last_success_time == 0);
        bool have_any_address = !sensor_addresses.empty() || strlen(cfg::chosen_altruist_urban) != 0;

        if (never_succeeded && !have_any_address) {
            // Limited discovery sequence while Urban is "possibly not present".
            if (discovery_attempts < URBAN_MAX_DISCOVERY_ATTEMPTS) {
                unsigned long now = millis();
                bool first_attempt = (discovery_attempts == 0);
                bool interval_elapsed = msSince(last_discovery_attempt_time) >= URBAN_REDISCOVER_INTERVAL_MS;

                if (first_attempt || interval_elapsed) {
                    debug_outln_info(F("HTTPAltruistSensor: scheduled rediscovery attempt "), String(discovery_attempts + 1));
                    if (_discoverSensors()) {
                        // After rediscovery, reset failure counter; success time will be updated
                        // once we actually fetch data successfully.
                        consecutive_failures = 0;
                    }
                }
            } else {
                debug_outln_info(F("HTTPAltruistSensor: reached max discovery attempts, Urban assumed absent"));
            }
        } else {
            // We had at least one successful communication before (or know an address);
            // occasionally re-discover in case of IP / network changes.
            bool long_since_success = (last_success_time != 0 && msSince(last_success_time) > URBAN_REDISCOVER_INTERVAL_MS);

            if (long_since_success) {
                debug_outln_info(F("HTTPAltruistSensor: attempting rediscovery after prolonged failures"));
                if (_discoverSensors()) {
                    consecutive_failures = 0;
                }
            }
        }
    }
}

#endif