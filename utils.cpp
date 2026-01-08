/************************************************************************
 *                                                                      *
 *    airRohr firmware                                                  *
 *    Copyright (C) 2016-2020  Code for Stuttgart a.o.                  *
 *    Copyright (C) 2019-2020  Dirk Mueller                             *
 *                                                                      *
 * This program is free software: you can redistribute it and/or modify *
 * it under the terms of the GNU General Public License as published by *
 * the Free Software Foundation, either version 3 of the License, or    *
 * (at your option) any later version.                                  *
 *                                                                      *
 * This program is distributed in the hope that it will be useful,      *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of       *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        *
 * GNU General Public License for more details.                         *
 *                                                                      *
 * You should have received a copy of the GNU General Public License    *
 * along with this program. If not, see <http://www.gnu.org/licenses/>. *
 *                                                                      *
 ************************************************************************
 */

#include <WString.h>

#include "./intl.h"
#include "./utils.h"
#include "./defines.h"
//#include "./ext_def.h"
#include <SPIFFS.h>
#include <Preferences.h>
#include <esp_system.h>
#include <time.h>

#ifdef ESP32
#if defined(CONFIG_IDF_TARGET_ESP32C6)
#include "driver/temperature_sensor.h"
static temperature_sensor_handle_t temp_sensor_handle = NULL;
static bool temp_sensor_initialized = false;
#endif
#endif

String get_chipid() {
	static String cached_chipid = "";  // Static variable to cache the ID
	
	if (cached_chipid.length() == 0) {  // Only get it once
		WiFiMode_t previousMode = WiFi.getMode();  // Save current mode
		WiFi.mode(WIFI_STA);  // Initialize WiFi to get MAC
		String mac = WiFi.macAddress();
		mac.replace(":", "");  // "AA:BB:CC:DD:EE:FF" -> "AABBCCDDEEFF"
		cached_chipid = mac;
		
		// Restore previous mode only if it wasn't OFF
		if (previousMode != WIFI_OFF) {
			WiFi.mode(previousMode);
		}
		// If it was OFF, leave it in STA mode for the wifi manager to handle
	}
	
	return cached_chipid;
}

String tmpl(const __FlashStringHelper* patt, const String& value) {
	String s = patt;
	s.replace("{v}", value);
	return s;
}

const char* get_reset_reason_text() {
    esp_reset_reason_t reason = esp_reset_reason();

    switch (reason) {
        case ESP_RST_POWERON:
            return "Power-on reset";
        case ESP_RST_EXT:
            return "External reset (e.g., reset button)";
        case ESP_RST_SW:
            return "Software reset (esp_restart())";
        case ESP_RST_PANIC:
            return "Panic reset (e.g., unhandled exception)";
        case ESP_RST_INT_WDT:
            return "Interrupt watchdog timeout";
        case ESP_RST_TASK_WDT:
            return "Task watchdog timeout";
        case ESP_RST_WDT:
            return "Other watchdog reset";
        case ESP_RST_DEEPSLEEP:
            return "Wake from deep sleep";
        case ESP_RST_BROWNOUT:
            return "Brownout reset (voltage too low)";
        case ESP_RST_SDIO:
            return "Reset over SDIO";
        default:
            return "Unknown";
    }
}

String delayToString(unsigned time_ms) {

	char buf[64];
	String s;

	if (time_ms > 2 * 1000 * 60 * 60 * 24) {
		sprintf_P(buf, PSTR("%d days, "), time_ms / (1000 * 60 * 60 * 24));
		s += buf;
		time_ms %= 1000 * 60 * 60 * 24;
	}

	if (time_ms > 2 * 1000 * 60 * 60) {
		sprintf_P(buf, PSTR("%d hours, "), time_ms / (1000 * 60 * 60));
		s += buf;
		time_ms %= 1000 * 60 * 60;
	}

	if (time_ms > 2 * 1000 * 60) {
		sprintf_P(buf, PSTR("%d min, "), time_ms / (1000 * 60));
		s += buf;
		time_ms %= 1000 * 60;
	}

	if (time_ms > 2 * 1000) {
		sprintf_P(buf, PSTR("%ds, "), time_ms / 1000);
		s += buf;
	}

	if (s.length() > 2) {
		s = s.substring(0, s.length() - 2);
	}

	return s;
}

void sensor_restart() {

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored  "-Wdeprecated-declarations"

		SPIFFS.end();

#pragma GCC diagnostic pop

		serialSDS.end();
		// digitalWrite(PM_RESTART, LOW);
		// debug_outln_info(F("Restart."));
		delay(5000);
		ESP.restart();
		// should not be reached
		while(true) { yield(); }
}


float readCorrectionOffset(const char* correction) {
	char* pEnd = nullptr;
	// Avoiding atof() here as this adds a lot (~ 9kb) of code size
	float r = float(strtol(correction, &pEnd, 10));
	if (pEnd && pEnd[0] == '.' && pEnd[1] >= '0' && pEnd[1] <= '9') {
		r += (correction[0] == '-' ? -1.0f : 1.0f) * ((pEnd[1] - '0') / 10.0f);
	}
	return r;
}

/*****************************************************************
 * Debug output                                                  *
 *****************************************************************/

LoggingSerial Debug;

LoggingSerial::LoggingSerial()
    : HardwareSerial(0)
{
	m_buffer = xQueueCreate(LARGE_STR, sizeof(uint8_t));
}

size_t LoggingSerial::write(uint8_t c)
{
	xQueueSendToBack(m_buffer, ( void * ) &c, ( TickType_t ) 1);
	return HardwareSerial::write(c);
}

size_t LoggingSerial::write(const uint8_t *buffer, size_t size)
{
	for(int i = 0; i < size; i++) {
		xQueueSendToBack(m_buffer, ( void * ) &buffer[i], ( TickType_t ) 1);
	}
	return HardwareSerial::write(buffer, size);
}

String LoggingSerial::popLines()
{
	String r;
	uint8_t c;
	while (xQueueReceive(m_buffer, &(c ), (TickType_t) 1 )) {
		r += (char) c;

		if (c == '\n' && r.length() > 10)
			break;
	}
	return r;
}

#define debug_level_check(level) { if (level > cfg::debug) return; }

void debug_out(const String& text, unsigned int level) {
	debug_level_check(level); Debug.print(text); Serial.print(text);
}

void debug_out(const __FlashStringHelper* text, unsigned int level) {
	debug_level_check(level); Debug.print(text); Serial.print(text);
}

void debug_outln(const String& text, unsigned int level) {
	debug_level_check(level); Debug.println(text); Serial.println(text);
}

void debug_outln_info(const String& text) {
	String dated_text = "[" + String(millis()) + "] " + text; 
	debug_level_check(DEBUG_MIN_INFO); Debug.println(text); Serial.println(dated_text);
}

void debug_outln_verbose(const String& text) {
	debug_level_check(DEBUG_MED_INFO); Debug.println(text); Serial.println(text);
}

void debug_outln_error(const __FlashStringHelper* text) {
	debug_level_check(DEBUG_ERROR); Debug.println(text); Serial.println(text);
}

void debug_outln_info(const __FlashStringHelper* text) {
	String dated_text = "[" + String(millis()) + "] " + text; 
	debug_level_check(DEBUG_MIN_INFO); Debug.println(text); Serial.println(dated_text);
}

void debug_outln_verbose(const __FlashStringHelper* text) {
	debug_level_check(DEBUG_MED_INFO); Debug.println(text); Serial.println(text);
}

void debug_outln_info(const __FlashStringHelper* text, const String& option) {
	String dated_text = "[" + String(millis()) + "] " + text; 
	debug_level_check(DEBUG_MIN_INFO);
	Debug.print(text);
	Debug.println(option);
	Serial.print(dated_text);
	Serial.println(option);
}

void debug_outln_info(const __FlashStringHelper* text, float value) {
	debug_outln_info(text, String(value));
}

void debug_outln_verbose(const __FlashStringHelper* text, const String& option) {
	debug_level_check(DEBUG_MED_INFO);
	Debug.print(text);
	Debug.println(option);
	Serial.print(text);
	Serial.println(option);
}

void debug_outln_info_bool(const __FlashStringHelper* text, const bool option) {
	debug_level_check(DEBUG_MIN_INFO);
	Debug.print(text);
	Debug.println(String(option));
	Serial.print(text);
	Serial.println(String(option));
}

#undef debug_level_check

/*****************************************************************
 * helper to see if a given string is numeric                    *
 *****************************************************************/
bool isNumeric(const String& str) {
	size_t stringLength = str.length();

	if (stringLength == 0) {
		return false;
	}

	bool seenDecimal = false;

	for (size_t i = 0; i < stringLength; ++i) {
		if (i == 0 && str.charAt(0) == '-') {
			continue;
		}

		if (isDigit(str.charAt(i))) {
			continue;
		}

		if (str.charAt(i) == '.') {
			if (seenDecimal) {
				return false;
			}
			seenDecimal = true;
			continue;
		}
		return false;
	}
	return true;
}

// Global metrics instance
metrics_t system_metrics;

// Initialize metrics (load boot counter from NVS, increment it)
void initMetrics() {
	Preferences preferences;
	preferences.begin("metrics", false);
	system_metrics.boot_counter = preferences.getULong("boot_cnt", 0);
	system_metrics.boot_counter++; // Increment on boot
	preferences.putULong("boot_cnt", system_metrics.boot_counter);
	preferences.end();
	
	system_metrics.uptime_sec = 0;
	system_metrics.tx_counter = 0;
	system_metrics.last_telemetry_timestamp = 0;
	system_metrics.err_wifi_reconnects = 0;
	system_metrics.err_sensor = 0;
	system_metrics.err_sd_write = 0;
	system_metrics.has_error = false;
}

// Update metrics (uptime, temperature, etc.)
void updateMetrics() {
	system_metrics.uptime_sec = millis() / 1000;
	system_metrics.esp_temperature = getESPTemperature();
	
	// Check if we have errors
	system_metrics.has_error = (system_metrics.err_wifi_reconnects > 0 || 
	                           system_metrics.err_sensor > 0 || 
	                           system_metrics.err_sd_write > 0);
}

// Initialize ESP32-C6 temperature sensor (call once at startup)
void initESPTemperatureSensor() {
	#ifdef ESP32
		#if defined(CONFIG_IDF_TARGET_ESP32C6)
			if (temp_sensor_initialized) {
				return; // Already initialized
			}
			
			temperature_sensor_config_t temp_sensor_config = {
				.range_min = -10,
				.range_max = 80,
			};
			
			esp_err_t ret = temperature_sensor_install(&temp_sensor_config, &temp_sensor_handle);
			if (ret != ESP_OK) {
				Serial.printf("[URBAN][Temp] Failed to install temperature sensor: %s\n", esp_err_to_name(ret));
				return;
			}
			
			ret = temperature_sensor_enable(temp_sensor_handle);
			if (ret != ESP_OK) {
				Serial.printf("[URBAN][Temp] Failed to enable temperature sensor: %s\n", esp_err_to_name(ret));
				temperature_sensor_uninstall(temp_sensor_handle);
				temp_sensor_handle = NULL;
				return;
			}
			
			temp_sensor_initialized = true;
			Serial.println(F("[URBAN][Temp] ESP32-C6 temperature sensor initialized"));
		#endif
	#endif
}

// Get ESP temperature (ESP32-C6 internal temperature sensor)
float getESPTemperature() {
	#ifdef ESP32
		#if defined(CONFIG_IDF_TARGET_ESP32C6)
			if (!temp_sensor_initialized || temp_sensor_handle == NULL) {
				return 0.0; // Sensor not initialized
			}
			
			float temperature = 0.0;
			esp_err_t ret = temperature_sensor_get_celsius(temp_sensor_handle, &temperature);
			if (ret != ESP_OK) {
				// If reading fails, return last known value or 0
				return 0.0;
			}
			return temperature;
		#else
			// Other ESP32 variants - no built-in temperature sensor
			return 0.0;
		#endif
	#else
		// Fallback: return 0 if not available
		return 0.0;
	#endif
}

// Increment error counters
void incrementWiFiReconnectError() {
	system_metrics.err_wifi_reconnects++;
}

void incrementSensorError() {
	system_metrics.err_sensor++;
}

void incrementSDWriteError() {
	system_metrics.err_sd_write++;
}

void incrementTXCounter() {
	system_metrics.tx_counter++;
	system_metrics.last_telemetry_timestamp = time(nullptr);
}

// Log metrics to UART in the specified format
void logMetrics() {
	updateMetrics();
	
	// Determine status
	const char* status = system_metrics.has_error ? "ERROR" : "ALIVE";
	const char* status_symbol = system_metrics.has_error ? "✗" : "✓";
	
	// Get WiFi state
	const char* wifi_state = "DISCONNECTED";
	const char* wifi_symbol = "✗";
	int rssi = 0;
	if (WiFi.status() == WL_CONNECTED) {
		wifi_state = "OK";
		wifi_symbol = "✓";
		rssi = WiFi.RSSI();
	}
	
	// Format uptime
	unsigned long hours = system_metrics.uptime_sec / 3600;
	unsigned long minutes = (system_metrics.uptime_sec % 3600) / 60;
	unsigned long seconds = system_metrics.uptime_sec % 60;
	
	// Add device identification
	#if defined(ALTRUIST_INSIDE)
	Serial.print(F("\r\n=== [INSIGHT] METRICS ===\r\n"));
	#elif defined(ALTRUIST_URBAN)
	Serial.print(F("\r\n=== [URBAN] METRICS ===\r\n"));
	#endif
	
	// Status line
	Serial.print(F("Status: "));
	Serial.print(status_symbol);
	Serial.print(F(" "));
	Serial.print(status);
	if (system_metrics.has_error) {
		Serial.print(F(" ("));
		if (system_metrics.err_wifi_reconnects > 0) Serial.print(F("WiFi "));
		if (system_metrics.err_sensor > 0) Serial.print(F("Sensor "));
		if (system_metrics.err_sd_write > 0) Serial.print(F("SD "));
		Serial.print(F(")"));
	}
	Serial.print(F("\r\n"));
	
	// Uptime line
	Serial.print(F("Uptime: "));
	if (hours > 0) {
		Serial.print(hours);
		Serial.print(F("h "));
	}
	if (minutes > 0 || hours > 0) {
		Serial.print(minutes);
		Serial.print(F("m "));
	}
	Serial.print(seconds);
	Serial.print(F("s ("));
	Serial.print(system_metrics.uptime_sec);
	Serial.print(F("s total)\r\n"));
	
	// Boot counter
	Serial.print(F("Boot: "));
	Serial.print(system_metrics.boot_counter);
	Serial.print(F("\r\n"));
	
	// WiFi line
	Serial.print(F("WiFi: "));
	Serial.print(wifi_symbol);
	Serial.print(F(" "));
	Serial.print(wifi_state);
	if (rssi != 0) {
		Serial.print(F(" (RSSI: "));
		Serial.print(rssi);
		Serial.print(F(" dBm)"));
	}
	Serial.print(F("\r\n"));
	
	// TX counter
	Serial.print(F("TX: "));
	Serial.print(system_metrics.tx_counter);
	if (system_metrics.last_telemetry_timestamp > 0) {
		time_t now = time(nullptr);
		unsigned long sec_since_tx = (now > system_metrics.last_telemetry_timestamp) ? 
		                            (now - system_metrics.last_telemetry_timestamp) : 0;
		Serial.print(F(" (last: "));
		Serial.print(sec_since_tx);
		Serial.print(F("s ago)"));
	}
	Serial.print(F("\r\n"));
	
	// Error counters
	Serial.print(F("Errors: WiFi="));
	Serial.print(system_metrics.err_wifi_reconnects);
	Serial.print(F(" Sensor="));
	Serial.print(system_metrics.err_sensor);
	Serial.print(F(" SD="));
	Serial.print(system_metrics.err_sd_write);
	Serial.print(F("\r\n"));
	
	// ESP temperature
	Serial.print(F("ESP Temp: "));
	Serial.print(system_metrics.esp_temperature, 1);
	Serial.print(F("°C\r\n"));
	
	Serial.print(F("==========================\r\n"));
}
