/************************************************************************
 *                                                                      *
 *  This source code needs to be compiled for the board                 *
 *  NodeMCU 1.0 (ESP-12E Module)                                        *
 *                                                                      *
 ************************************************************************
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
 * OK LAB Particulate Matter Sensor                                     *
 *      - nodemcu-LoLin board                                           *
 *      - Nova SDS0111                                                  *
 *  http://inovafitness.com/en/Laser-PM2-5-Sensor-SDS011-35.html        *
 *                                                                      *
 * Wiring Instruction see included Readme.md                            *
 *                                                                      *
 ************************************************************************
 *                                                                      *
 * Alternative                                                          *
 *      - nodemcu-LoLin board                                           *
 *                                                                      *
 * Wiring Instruction:                                                  *
 *      Pin 2 of dust sensor PM2.5 -> Digital 6 (PWM)                   *
 *      Pin 3 of dust sensor       -> +5V                               *
 *      Pin 4 of dust sensor PM1   -> Digital 3 (PMW)                   *
 *                                                                      *
 *                                                                      *
 ************************************************************************
 *                                                                      *
 * Please check Readme.md for other sensors and hardware                *
 *                                                                      *
 ************************************************************************
 *
 * latest mit lib 2.6.1
 * DATA:    [====      ]  40.7% (used 33316 bytes from 81920 bytes)
 * PROGRAM: [=====     ]  49.3% (used 514788 bytes from 1044464 bytes)

 * latest mit lib 2.5.2
 * DATA:    [====      ]  39.4% (used 32304 bytes from 81920 bytes)
 * PROGRAM: [=====     ]  48.3% (used 504812 bytes from 1044464 bytes)
 *
 ************************************************************************/
#include <WString.h>
#include <pgmspace.h>

/*****************************************************************
 * Includes                                                      *
 *****************************************************************/

#if defined(ESP8266)
#include <FS.h>                     // must be first
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <SoftwareSerial.h>
#include <Hash.h>
#include <ctime>
#include <coredecls.h>
#include <sntp.h>
#include <Arduino.h>
#endif

#if defined(ESP32)
#include <FS.h>
#include <HTTPClient.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <HardwareSerial.h>
#include "sha/sha_parallel_engine.h"
#include <WebServer.h>
#include <ESPmDNS.h>
#include <MD5Builder.h>
#include <Update.h>
#endif

// includes common to ESP8266 and ESP32 (especially external libraries)
#include "./oledfont.h"				// avoids including the default Arial font, needs to be included before SSD1306.h
#include <SH1106.h>
#include <SSD1306.h>
#include <LiquidCrystal_I2C.h>
#define ARDUINOJSON_ENABLE_ARDUINO_STREAM 0
#define ARDUINOJSON_ENABLE_ARDUINO_PRINT 0
#define ARDUINOJSON_DECODE_UNICODE 0
#include <ArduinoJson.h>
#include <DNSServer.h>
#include "./DHT.h"
#include <Adafruit_HTU21DF.h>
#include <Adafruit_BMP085.h>
#include <Adafruit_SHT31.h>
#include <StreamString.h>
#include <DallasTemperature.h>
#include <TinyGPS++.h>
#include "./sps30_i2c.h"
#include "./dnms_i2c.h"
#include "./dbmeter_regs.h"

#include "./intl.h"

#include "./utils.h"
#include "defines.h"
#include "ext_def.h"
#include "robonomics_servers.h"
#include "webserver/html-content.h"
#include "SparkFunCCS811.h"
#include "CG_RadSens.h"
#include <Robonomics.h>
#include <Ed25519.h>
#include "sensors/sensor_factory.h"
#include "apis/apis.h"
#include "config_manager/config_helpers.h"
#include "wifi_manager.h"
#include "webserver/webserver.h"
//#include "radSens1v2.h"

String SOFTWARE_VERSION(SOFTWARE_VERSION_STR);

bool first_loop = true;

LoggerConfig loggerConfigs[LoggerCount];

long int sample_count = 0;
bool htu21d_init_failed = false;
bool dbmeter_init_failed = false;
bool bmp_init_failed = false;
bool bmx280_init_failed = false;
bool sht3x_init_failed = false;
bool dnms_init_failed = false;
bool gps_init_failed = false;
bool airrohr_selftest_failed = false;

StaticJsonDocument<1024> sensors_data;
device_status_t deviceStatus;

SensorWebServer webserver(sensors_data, deviceStatus);

/*****************************************************************
 * Variables for Robonomics                                      *
 *****************************************************************/
int num_of_robonomics_API = 0;
String last_datalog_data = "";
Robonomics robonomics;

/*****************************************************************
 * Variables for Noise Measurement DNMS                          *
 *****************************************************************/
float last_value_dnms_laeq = -1.0;
float last_value_dnms_la_min = -1.0;
float last_value_dnms_la_max = -1.0;

/*****************************************************************
 * Display definitions                                           *
 *****************************************************************/
// SSD1306*  oled_ssd1306 = nullptr;
// SH1106* oled_sh1106 = nullptr;
// LiquidCrystal_I2C* lcd_1602 = nullptr;
// LiquidCrystal_I2C* lcd_2004 = nullptr;

/*****************************************************************
 * SDS011 declarations                                           *
 *****************************************************************/
#if defined(ESP8266)
SoftwareSerial serialSDS;
// SoftwareSerial* serialGPS;
#endif
#if defined(ESP32)
#define serialSDS (Serial1)
// #include <HardwareSerial.h>

// // Define Serial2 on UART2
// HardwareSerial Serial2(2);
// #define serialGPS (&(Serial2))
#endif

/*****************************************************************
 * DHT declaration                                               *
 *****************************************************************/
DHT dht(ONEWIRE_PIN, DHT_TYPE);

/*****************************************************************
 * HTU21D declaration                                            *
 *****************************************************************/
Adafruit_HTU21DF htu21d;

/*****************************************************************
 * BMP declaration                                               *
 *****************************************************************/
Adafruit_BMP085 bmp;


/*****************************************************************
 * SHT3x declaration                                             *
 *****************************************************************/
Adafruit_SHT31 sht3x;

/*****************************************************************
 * DS18B20 declaration                                            *
 *****************************************************************/
OneWire oneWire;
DallasTemperature ds18b20(&oneWire);

/*****************************************************************
 * GPS declaration                                               *
 *****************************************************************/
TinyGPSPlus gps;

/*****************************************************************
 * CCS811 declaration                                               *
 *****************************************************************/

#define CCS811_ADDR 0x5A //Default I2C Address
#define CCS811_27_ADDR 0x5B //Alternate I2C Address

CCS811 ccs811(CCS811_ADDR);
CCS811 ccs811_27(CCS811_27_ADDR);

/*****************************************************************
 * Radiation sensor declaration                                            *
 *****************************************************************/

CG_RadSens radSens(RS_DEFAULT_I2C_ADDRESS); 

/*****************************************************************
 * Variable Definitions for PPD24NS                              *
 * P1 for PM10 & P2 for PM25                                     *
 *****************************************************************/

boolean trigP1 = false;
boolean trigP2 = false;
unsigned long trigOnP1;
unsigned long trigOnP2;

unsigned long lowpulseoccupancyP1 = 0;
unsigned long lowpulseoccupancyP2 = 0;

bool send_now = false;
bool send_datalog_now = false;
unsigned long starttime;
unsigned long last_datalog_time;
unsigned long time_point_device_start_ms;
unsigned long starttime_SDS;
unsigned long starttime_DB;
unsigned long starttime_GPS;
unsigned long starttime_NPM;
unsigned long last_NPM;
unsigned long act_micro;
unsigned long act_milli;
unsigned long last_micro = 0;
unsigned long min_micro = 1000000000;
unsigned long max_micro = 0;

bool is_SDS_running = true;
enum {
	SDS_REPLY_HDR = 10,
	SDS_REPLY_BODY = 8
} SDS_waiting_for;
bool is_PMS_running = true;
bool is_HPM_running = true;
bool is_NPM_running = true;

unsigned long sending_time = 0;
unsigned long last_update_attempt;
int last_update_returncode;
int last_sendData_returncode;

float last_value_BMP_T = -128.0;
float last_value_BMP_P = -1.0;
float last_value_BMX280_T = -128.0;
float last_value_BMX280_P = -1.0;
float last_value_BME280_H = -1.0;
float last_value_DHT_T = -128.0;
float last_value_DHT_H = -1.0;
float last_value_DS18B20_T = -1.0;
float last_value_HTU21D_T = -128.0;
float last_value_HTU21D_H = -1.0;
float last_value_SHT3X_T = -128.0;
float last_value_SHT3X_H = -1.0;

uint8_t last_value_DBMETER = 0;
uint8_t last_value_DBMETER_max = 0;
uint32_t last_value_DBMETER_sum = 0;
uint8_t last_value_DBMETER_count = 0;
float last_value_DBMETER_mean = 0;

uint32_t sds_pm10_sum = 0;
uint32_t sds_pm25_sum = 0;
uint32_t sds_val_count = 0;
uint32_t sds_pm10_max = 0;
uint32_t sds_pm10_min = 20000;
uint32_t sds_pm25_max = 0;
uint32_t sds_pm25_min = 20000;

int pms_pm1_sum = 0;
int pms_pm10_sum = 0;
int pms_pm25_sum = 0;
int pms_val_count = 0;
int pms_pm1_max = 0;
int pms_pm1_min = 20000;
int pms_pm10_max = 0;
int pms_pm10_min = 20000;
int pms_pm25_max = 0;
int pms_pm25_min = 20000;

int hpm_pm10_sum = 0;
int hpm_pm25_sum = 0;
int hpm_val_count = 0;
int hpm_pm10_max = 0;
int hpm_pm10_min = 20000;
int hpm_pm25_max = 0;
int hpm_pm25_min = 20000;

uint32_t npm_pm1_sum = 0;
uint32_t npm_pm10_sum = 0;
uint32_t npm_pm25_sum = 0;
uint32_t npm_pm1_sum_pcs = 0;
uint32_t npm_pm10_sum_pcs = 0;
uint32_t npm_pm25_sum_pcs = 0;
uint16_t npm_val_count = 0;
uint16_t npm_pm1_max = 0;
uint16_t npm_pm1_min = 20000;
uint16_t npm_pm10_max = 0;
uint16_t npm_pm10_min = 20000;
uint16_t npm_pm25_max = 0;
uint16_t npm_pm25_min = 20000;
uint16_t npm_pm1_max_pcs = 0;
uint16_t npm_pm1_min_pcs = 60000;
uint16_t npm_pm10_max_pcs = 0;
uint16_t npm_pm10_min_pcs = 60000;
uint16_t npm_pm25_max_pcs = 0;
uint16_t npm_pm25_min_pcs = 60000;
bool newCmdNPM = true;

float last_value_SPS30_P0 = -1.0;
float last_value_SPS30_P1 = -1.0;
float last_value_SPS30_P2 = -1.0;
float last_value_SPS30_P4 = -1.0;
float last_value_SPS30_N05 = -1.0;
float last_value_SPS30_N1 = -1.0;
float last_value_SPS30_N25 = -1.0;
float last_value_SPS30_N4 = -1.0;
float last_value_SPS30_N10 = -1.0;
float last_value_SPS30_TS = -1.0;
float value_SPS30_P0 = 0.0;
float value_SPS30_P1 = 0.0;
float value_SPS30_P2 = 0.0;
float value_SPS30_P4 = 0.0;
float value_SPS30_N05 = 0.0;
float value_SPS30_N1 = 0.0;
float value_SPS30_N25 = 0.0;
float value_SPS30_N4 = 0.0;
float value_SPS30_N10 = 0.0;
float value_SPS30_TS = 0.0;


uint16_t SPS30_measurement_count = 0;
unsigned long SPS30_read_counter = 0;
unsigned long SPS30_read_error_counter = 0;
unsigned long SPS30_read_timer = 0;
bool sps30_init_failed = false;

bool ccs811_init_failed = false;
bool gc_init_failed = false;

float last_value_PPD_P1 = -1.0;
float last_value_PPD_P2 = -1.0;
float last_value_SDS_P1 = -1.0;
float last_value_SDS_P2 = -1.0;
float last_value_CCS811_CO2 = -1.0;
float last_value_CCS811_TVOC = -1.0;
float last_value_gc = -1.0;
float last_value_PMS_P0 = -1.0;
float last_value_PMS_P1 = -1.0;
float last_value_PMS_P2 = -1.0;
float last_value_HPM_P1 = -1.0;
float last_value_HPM_P2 = -1.0;
float last_value_NPM_P0 = -1.0;
float last_value_NPM_P1 = -1.0;
float last_value_NPM_P2 = -1.0;
float last_value_NPM_N0 = -1.0;
float last_value_NPM_N1 = -1.0;
float last_value_NPM_N2 = -1.0;
float last_value_GPS_alt = -1000.0;
double last_value_GPS_lat = -200.0;
double last_value_GPS_lon = -200.0;
String last_value_GPS_timestamp;
String last_data_string;
int last_signal_strength;
int last_disconnect_reason;

String esp_chipid;
String esp_mac_id;
String last_value_SDS_version;

unsigned long SDS_error_count;
unsigned long WiFi_error_count;


uint8_t sntp_time_set;

unsigned long count_sends = 0;
unsigned long last_display_millis = 0;
uint8_t next_display_count = 0;

const char data_first_part[] PROGMEM = "{\"software_version\": \"" SOFTWARE_VERSION_STR "\", \"sensordatavalues\":[";
const char JSON_SENSOR_DATA_VALUES[] PROGMEM = "sensordatavalues";

/*****************************************************************
 * display values                                                *
 *****************************************************************/
// static void display_debug(const String& text1, const String& text2) {
// 	debug_outln_info(F("output debug text to displays..."));
// 	if (oled_ssd1306) {
// 		oled_ssd1306->clear();
// 		oled_ssd1306->displayOn();
// 		oled_ssd1306->setTextAlignment(TEXT_ALIGN_LEFT);
// 		oled_ssd1306->drawString(0, 12, text1);
// 		oled_ssd1306->drawString(0, 24, text2);
// 		oled_ssd1306->display();
// 	}
// 	if (oled_sh1106) {
// 		oled_sh1106->clear();
// 		oled_sh1106->displayOn();
// 		oled_sh1106->setTextAlignment(TEXT_ALIGN_LEFT);
// 		oled_sh1106->drawString(0, 12, text1);
// 		oled_sh1106->drawString(0, 24, text2);
// 		oled_sh1106->display();
// 	}
// 	if (lcd_1602) {
// 		lcd_1602->clear();
// 		lcd_1602->setCursor(0, 0);
// 		lcd_1602->print(text1);
// 		lcd_1602->setCursor(0, 1);
// 		lcd_1602->print(text2);
// 	}
// 	if (lcd_2004) {
// 		lcd_2004->clear();
// 		lcd_2004->setCursor(0, 0);
// 		lcd_2004->print(text1);
// 		lcd_2004->setCursor(0, 1);
// 		lcd_2004->print(text2);
// 	}
// }

/*****************************************************************
 * init CCS811 sensor                                            *
 *****************************************************************/

static void initSensorCCS811() {
	Wire.begin();
	if (cfg::ccs811_read) {
		debug_outln_info(F("Trying CCS811 on 0x5A"));
		if (ccs811.begin() == false) {
			ccs811_init_failed = true;
			debug_outln_error(F("CCS811 error starting measurement"));
			return;
		}
	}
	if (cfg::ccs811_27_read) {
		debug_outln_info(F("Trying CCS811 on 0x5B"));
		if (ccs811_27.begin() == false) {
			ccs811_init_failed = true;
			debug_outln_error(F("CCS811 error starting measurement"));
			return;
		}
	}
}

/*****************************************************************
 * init DB Meter sensor                                            *
 *****************************************************************/

static void initDBMeter() {
	// Read version register
	uint8_t version = dbmeter_readreg(DBM_REG_VERSION);
	if (version != 255) {
		debug_outln_info(F("DB Meter version = "), String(version));
	} else {
		debug_outln_info(F("Check DB Meter wiring..."));
		dbmeter_init_failed = true;
	}
}

/*****************************************************************
 * disable unneeded NMEA sentences, TinyGPS++ needs GGA, RMC     *
 *****************************************************************/
static void disable_unneeded_nmea() {
// 	serialGPS->println(F("$PUBX,40,GLL,0,0,0,0*5C"));       // Geographic position, latitude / longitude
// //	serialGPS->println(F("$PUBX,40,GGA,0,0,0,0*5A"));       // Global Positioning System Fix Data
// 	serialGPS->println(F("$PUBX,40,GSA,0,0,0,0*4E"));       // GPS DOP and active satellites
// //	serialGPS->println(F("$PUBX,40,RMC,0,0,0,0*47"));       // Recommended minimum specific GPS/Transit data
// 	serialGPS->println(F("$PUBX,40,GSV,0,0,0,0*59"));       // GNSS satellites in view
// 	serialGPS->println(F("$PUBX,40,VTG,0,0,0,0*5E"));       // Track made good and ground speed
}


/*****************************************************************
 * send data to influxdb                                         *
 *****************************************************************/
static void create_influxdb_string_from_data(String& data_4_influxdb, const String& data) {
	debug_outln_verbose(F("Parse JSON for influx DB: "), data);
	DynamicJsonDocument json2data(JSON_BUFFER_SIZE);
	DeserializationError err = deserializeJson(json2data, data);
	if (!err) {
		data_4_influxdb += cfg::measurement_name_influx;
		data_4_influxdb += F(",node=" SENSOR_BASENAME);
		data_4_influxdb += esp_chipid + " ";
		for (JsonObject measurement : json2data[FPSTR(JSON_SENSOR_DATA_VALUES)].as<JsonArray>()) {
			data_4_influxdb += measurement["value_type"].as<char*>();
			data_4_influxdb += "=";

			if (isNumeric(measurement["value"])) {
				//send numerics without quotes
				data_4_influxdb += measurement["value"].as<char*>();
			} else {
				//quote string values
				data_4_influxdb += "\"";
				data_4_influxdb += measurement["value"].as<char*>();
				data_4_influxdb += "\"";
			}
			data_4_influxdb += ",";
		}
		if ((unsigned)(data_4_influxdb.lastIndexOf(',') + 1) == data_4_influxdb.length()) {
			data_4_influxdb.remove(data_4_influxdb.length() - 1);
		}

		data_4_influxdb += '\n';
	} else {
		debug_outln_error(FPSTR(DBG_TXT_DATA_READ_FAILED));
	}
}

/*****************************************************************
 * send data as csv to serial out                                *
 *****************************************************************/
static void send_csv(const String& data) {
	DynamicJsonDocument json2data(JSON_BUFFER_SIZE);
	DeserializationError err = deserializeJson(json2data, data);
	debug_outln_info(F("CSV Output: "), data);
	if (!err) {
		String headline = F("Timestamp_ms;");
		String valueline(act_milli);
		valueline += ';';
		for (JsonObject measurement : json2data[FPSTR(JSON_SENSOR_DATA_VALUES)].as<JsonArray>()) {
			headline += measurement["value_type"].as<char*>();
			headline += ';';
			valueline += measurement["value"].as<char*>();
			valueline += ';';
		}
		static bool first_csv_line = true;
		if (first_csv_line) {
			if (headline.length() > 0) {
				headline.remove(headline.length() - 1);
			}
			Debug.println(headline);
			first_csv_line = false;
		}
		if (valueline.length() > 0) {
			valueline.remove(valueline.length() - 1);
		}
		Debug.println(valueline);
	} else {
		debug_outln_error(FPSTR(DBG_TXT_DATA_READ_FAILED));
	}
}

/*****************************************************************
 * read DHT22 sensor values                                      *
 *****************************************************************/
static void fetchSensorDHT(String& s) {
	debug_outln_verbose(FPSTR(DBG_TXT_START_READING), FPSTR(SENSORS_DHT22));

	// Check if valid number if non NaN (not a number) will be send.
	last_value_DHT_T = -128;
	last_value_DHT_H = -1;

	int count = 0;
	const int MAX_ATTEMPTS = 5;
	while ((count++ < MAX_ATTEMPTS)) {
		auto t = dht.readTemperature();
		auto h = dht.readHumidity();
		if (isnan(t) || isnan(h)) {
			delay(100);
			t = dht.readTemperature(false);
			h = dht.readHumidity();
		}
		if (isnan(t) || isnan(h)) {
			debug_outln_error(F("DHT11/DHT22 read failed"));
		} else {
			last_value_DHT_T = t + readCorrectionOffset(cfg::temp_correction);
			last_value_DHT_H = h;
			add_Value2Json(s, F("temperature"), FPSTR(DBG_TXT_TEMPERATURE), last_value_DHT_T);
			add_Value2Json(s, F("humidity"), FPSTR(DBG_TXT_HUMIDITY), last_value_DHT_H);
			break;
		}
	}
	debug_outln_info(FPSTR(DBG_TXT_SEP));

	debug_outln_verbose(FPSTR(DBG_TXT_END_READING), FPSTR(SENSORS_DHT22));
}

/*****************************************************************
 * read HTU21D sensor values                                     *
 *****************************************************************/
static void fetchSensorHTU21D(String& s) {
	debug_outln_verbose(FPSTR(DBG_TXT_START_READING), FPSTR(SENSORS_HTU21D));

	const auto t = htu21d.readTemperature();
	const auto h = htu21d.readHumidity();
	if (isnan(t) || isnan(h)) {
		last_value_HTU21D_T = -128.0;
		last_value_HTU21D_H = -1.0;
		debug_outln_error(F("HTU21D read failed"));
	} else {
		last_value_HTU21D_T = t;
		last_value_HTU21D_H = h;
		add_Value2Json(s, F("HTU21D_temperature"), FPSTR(DBG_TXT_TEMPERATURE), last_value_HTU21D_T);
		add_Value2Json(s, F("HTU21D_humidity"), FPSTR(DBG_TXT_HUMIDITY), last_value_HTU21D_H);
	}
	debug_outln_info(FPSTR(DBG_TXT_SEP));

	debug_outln_verbose(FPSTR(DBG_TXT_END_READING), FPSTR(SENSORS_HTU21D));
}

/*****************************************************************
 * read DB meter sensor values                                     *
 *****************************************************************/
static void fetchSensorDBMeter(String& s) {
	debug_outln_verbose(FPSTR(DBG_TXT_START_READING), FPSTR(SENSORS_DBMETER));
	if (is_SDS_running && cfg::sds_read) {
		debug_outln_verbose(F("Don't measure noise: SDS is running"));
	} else {
		Wire.setClock(10000);
		uint8_t db = dbmeter_readreg(DBM_REG_DECIBEL);
		if (db == 255) {
			last_value_DBMETER = 0;
			last_value_DBMETER_max = 0;
			last_value_DBMETER_mean = 0;
			last_value_DBMETER_count = 0;
			last_value_DBMETER_sum = 0;
			debug_outln_error(F("DB Meter read failed"));
		} else {
			last_value_DBMETER = db;
			if (last_value_DBMETER > last_value_DBMETER_max) {
				last_value_DBMETER_max = last_value_DBMETER;
			}
			last_value_DBMETER_sum += last_value_DBMETER;
			last_value_DBMETER_count++;
			last_value_DBMETER_mean = (float)last_value_DBMETER_sum / (float)last_value_DBMETER_count;
		}
		Wire.setClock(100000);
	}
	if (send_now) {
		debug_outln_info(F("Noise sum: "), last_value_DBMETER_sum);
		debug_outln_info(F("Noise count: "), last_value_DBMETER_count);
		debug_outln_info(F("Noise max: "), last_value_DBMETER_max);
		debug_outln_info(F("Noise mean: "), last_value_DBMETER_mean);
		debug_outln_info(FPSTR(DBG_TXT_SEP));
		add_Value2Json(s, F("PCBA_noiseMax"), FPSTR(DBG_TXT_DECIBEL), last_value_DBMETER_max);
		add_Value2Json(s, F("PCBA_noiseAvg"), FPSTR(DBG_TXT_DECIBEL), last_value_DBMETER_mean);
		last_value_DBMETER_max = 0;
		last_value_DBMETER_mean = 0;
		last_value_DBMETER_count = 0;
		last_value_DBMETER_sum = 0;
	}
	debug_outln_verbose(FPSTR(DBG_TXT_END_READING), FPSTR(SENSORS_DBMETER));
}

uint8_t dbmeter_readreg (uint8_t regaddr) {
	Wire.beginTransmission(DBM_ADDR);
	Wire.write(regaddr);
	Wire.endTransmission();
	Wire.requestFrom(DBM_ADDR, 1);
	delay(10);
	return Wire.read();
}

/*****************************************************************
 * read BMP180 sensor values                                     *
 *****************************************************************/
static void fetchSensorBMP(String& s) {
	debug_outln_verbose(FPSTR(DBG_TXT_START_READING), FPSTR(SENSORS_BMP180));

	const auto p = bmp.readPressure();
	const auto t = bmp.readTemperature();
	if (isnan(p) || isnan(t)) {
		last_value_BMP_T = -128.0;
		last_value_BMP_P = -1.0;
		debug_outln_error(F("BMP180 read failed"));
	} else {
		last_value_BMP_T = t;
		last_value_BMP_P = p;
		add_Value2Json(s, F("BMP_pressure"), FPSTR(DBG_TXT_PRESSURE), last_value_BMP_P);
		add_Value2Json(s, F("BMP_temperature"), FPSTR(DBG_TXT_TEMPERATURE), last_value_BMP_T);
	}
	debug_outln_info(FPSTR(DBG_TXT_SEP));
	debug_outln_verbose(FPSTR(DBG_TXT_END_READING), FPSTR(SENSORS_BMP180));
}

/*****************************************************************
 * read SHT3x sensor values                                      *
 *****************************************************************/
static void fetchSensorSHT3x(String& s) {
	debug_outln_verbose(FPSTR(DBG_TXT_START_READING), FPSTR(SENSORS_SHT3X));

	const auto t = sht3x.readTemperature();
	const auto h = sht3x.readHumidity();
	if (isnan(h) || isnan(t)) {
		last_value_SHT3X_T = -128.0;
		last_value_SHT3X_H = -1.0;
		debug_outln_error(F("SHT3X read failed"));
	} else {
		last_value_SHT3X_T = t;
		last_value_SHT3X_H = h;
		add_Value2Json(s, F("SHT3X_temperature"), FPSTR(DBG_TXT_TEMPERATURE), last_value_SHT3X_T);
		add_Value2Json(s, F("SHT3X_humidity"), FPSTR(DBG_TXT_HUMIDITY), last_value_SHT3X_H);
	}
	debug_outln_info(FPSTR(DBG_TXT_SEP));
	debug_outln_verbose(FPSTR(DBG_TXT_END_READING), FPSTR(SENSORS_SHT3X));
}

/*****************************************************************
 * read DS18B20 sensor values                                    *
 *****************************************************************/
static void fetchSensorDS18B20(String& s) {
	float t;
	debug_outln_verbose(FPSTR(DBG_TXT_START_READING), FPSTR(SENSORS_DS18B20));

	//it's very unlikely (-127: impossible) to get these temperatures in reality. Most times this means that the sensor is currently faulty
	//try 5 times to read the sensor, otherwise fail
	const int MAX_ATTEMPTS = 5;
	int count = 0;
	do {
		ds18b20.requestTemperatures();
		//for now, we want to read only the first sensor
		t = ds18b20.getTempCByIndex(0);
		++count;
		debug_outln_info(F("DS18B20 trying...."));
	} while (count < MAX_ATTEMPTS && (isnan(t) || t >= 85.0f || t <= (-127.0f)));

	if (count == MAX_ATTEMPTS) {
		last_value_DS18B20_T = -128.0;
		debug_outln_error(F("DS18B20 read failed"));
	} else {
		last_value_DS18B20_T = t + readCorrectionOffset(cfg::temp_correction);
		add_Value2Json(s, F("DS18B20_temperature"), FPSTR(DBG_TXT_TEMPERATURE), last_value_DS18B20_T);
	}
	debug_outln_info(FPSTR(DBG_TXT_SEP));
	debug_outln_verbose(FPSTR(DBG_TXT_END_READING), FPSTR(SENSORS_DS18B20));
}

/*****************************************************************
 * read CCS811 sensor values                                     *
 *****************************************************************/
static void fetchSensorCCS811(String& s) {
	if (cfg::ccs811_read) {
		if (ccs811.dataAvailable()){
			ccs811.readAlgorithmResults();
			last_value_CCS811_CO2 = ccs811.getCO2();
			last_value_CCS811_TVOC = ccs811.getTVOC();
			debug_outln_verbose(F("CCS811 CO2 : "), String(last_value_CCS811_CO2));
			debug_outln_verbose(F("CCS811 TVOC : "), String(last_value_CCS811_TVOC));
		}
	}
	if (cfg::ccs811_27_read) {
		if (ccs811_27.dataAvailable()){
			ccs811_27.readAlgorithmResults();
			last_value_CCS811_CO2 = ccs811_27.getCO2();
			last_value_CCS811_TVOC = ccs811_27.getTVOC();
			debug_outln_verbose(F("CCS811 CO2 : "), String(last_value_CCS811_CO2));
			debug_outln_verbose(F("CCS811 TVOC : "), String(last_value_CCS811_TVOC));
		}
	}
	if (send_now) {
		add_Value2Json(s, F("CCS_CO2"), F("CO2:  "), last_value_CCS811_CO2);
		add_Value2Json(s, F("CCS_TVOC"), F("TVOC: "), last_value_CCS811_TVOC);
		debug_outln_info(FPSTR(DBG_TXT_SEP));
	}
}

/*****************************************************************
 * read Cajoe Geiger Counter sensor values                                     *
 *****************************************************************/

static void init_GS() {
	debug_outln_info(F("Trying RadSens on "), RS_DEFAULT_I2C_ADDRESS);
	if (radSens.init() == false) {
		gc_init_failed = true;
		debug_outln_error(F("RadSens error starting measurement"));
		return;
	}
}

static void fetchSensorGC(String& s) {
		last_value_gc = radSens.getRadIntensyDynamic();
		add_Value2Json(s, F("GC"), String(last_value_gc));
		debug_outln_info(F("GC "), last_value_gc);
	}


/*****************************************************************
 * read PPD42NS sensor values                                    *
 *****************************************************************/
static __noinline void fetchSensorPPD(String& s) {
	debug_outln_verbose(FPSTR(DBG_TXT_START_READING), FPSTR(SENSORS_PPD42NS));

	if (msSince(starttime) <= SAMPLETIME_MS) {

		// Read pins connected to ppd42ns
		boolean valP1 = digitalRead(PPD_PIN_PM1);
		boolean valP2 = digitalRead(PPD_PIN_PM2);

		if (valP1 == LOW && trigP1 == false) {
			trigP1 = true;
			trigOnP1 = act_micro;
		}

		if (valP1 == HIGH && trigP1 == true) {
			lowpulseoccupancyP1 += act_micro - trigOnP1;
			trigP1 = false;
		}

		if (valP2 == LOW && trigP2 == false) {
			trigP2 = true;
			trigOnP2 = act_micro;
		}

		if (valP2 == HIGH && trigP2 == true) {
			unsigned long durationP2 = act_micro - trigOnP2;
			lowpulseoccupancyP2 += durationP2;
			trigP2 = false;
		}

	}
	// Checking if it is time to sample
	if (send_now) {
		auto calcConcentration = [](const float ratio) {
			/* spec sheet curve*/
			return (1.1f * ratio * ratio * ratio - 3.8f * ratio * ratio + 520.0f * ratio + 0.62f);
		};

		last_value_PPD_P1 = -1;
		last_value_PPD_P2 = -1;
		float ratio = lowpulseoccupancyP1 / (SAMPLETIME_MS * 10.0f);
		float concentration = calcConcentration(ratio);

		// json for push to api / P1
		last_value_PPD_P1 = concentration;
		add_Value2Json(s, F("durP1"), F("LPO P10    : "), lowpulseoccupancyP1);
		add_Value2Json(s, F("ratioP1"), F("Ratio PM10%: "), ratio);
		add_Value2Json(s, F("P1"), F("PM10 Count : "), last_value_PPD_P1);

		ratio = lowpulseoccupancyP2 / (SAMPLETIME_MS * 10.0f);
		concentration = calcConcentration(ratio);

		// json for push to api / P2
		last_value_PPD_P2 = concentration;
		add_Value2Json(s, F("durP2"), F("LPO PM25   : "), lowpulseoccupancyP2);
		add_Value2Json(s, F("ratioP2"), F("Ratio PM25%: "), ratio);
		add_Value2Json(s, F("P2"), F("PM25 Count : "), last_value_PPD_P2);

		debug_outln_info(FPSTR(DBG_TXT_SEP));
	}

	debug_outln_verbose(FPSTR(DBG_TXT_END_READING), FPSTR(SENSORS_PPD42NS));
}

/*****************************************************************
   read SPS30 PM sensor values
 *****************************************************************/
static void fetchSensorSPS30(String& s) {
	debug_outln_verbose(FPSTR(DBG_TXT_START_READING), FPSTR(SENSORS_SPS30));

	last_value_SPS30_P0 = value_SPS30_P0 / SPS30_measurement_count;
	last_value_SPS30_P2 = value_SPS30_P2 / SPS30_measurement_count;
	last_value_SPS30_P4 = value_SPS30_P4 / SPS30_measurement_count;
	last_value_SPS30_P1 = value_SPS30_P1 / SPS30_measurement_count;
	last_value_SPS30_N05 = value_SPS30_N05 / SPS30_measurement_count;
	last_value_SPS30_N1 = value_SPS30_N1 / SPS30_measurement_count;
	last_value_SPS30_N25 = value_SPS30_N25 / SPS30_measurement_count;
	last_value_SPS30_N4 = value_SPS30_N4 / SPS30_measurement_count;
	last_value_SPS30_N10 = value_SPS30_N10 / SPS30_measurement_count;
	last_value_SPS30_TS = value_SPS30_TS / SPS30_measurement_count;

	add_Value2Json(s, F("SPS30_P0"), F("PM1.0: "), last_value_SPS30_P0);
	add_Value2Json(s, F("SPS30_P2"), F("PM2.5: "), last_value_SPS30_P2);
	add_Value2Json(s, F("SPS30_P4"), F("PM4.0: "), last_value_SPS30_P4);
	add_Value2Json(s, F("SPS30_P1"), F("PM 10: "), last_value_SPS30_P1);
	add_Value2Json(s, F("SPS30_N05"), F("NC0.5: "), last_value_SPS30_N05);
	add_Value2Json(s, F("SPS30_N1"), F("NC1.0: "), last_value_SPS30_N1);
	add_Value2Json(s, F("SPS30_N25"), F("NC2.5: "), last_value_SPS30_N25);
	add_Value2Json(s, F("SPS30_N4"), F("NC4.0: "), last_value_SPS30_N4);
	add_Value2Json(s, F("SPS30_N10"), F("NC10:  "), last_value_SPS30_N10);
	add_Value2Json(s, F("SPS30_TS"), F("TPS:   "), last_value_SPS30_TS);

	debug_outln_info(F("SPS30 read counter: "), String(SPS30_read_counter));
	debug_outln_info(F("SPS30 read error counter: "), String(SPS30_read_error_counter));

	SPS30_measurement_count = 0;
	SPS30_read_counter = 0;
	SPS30_read_error_counter = 0;
	value_SPS30_P0 = value_SPS30_P1 = value_SPS30_P2 = value_SPS30_P4 = 0.0;
	value_SPS30_N05 = value_SPS30_N1 = value_SPS30_N25 = value_SPS30_N10 = value_SPS30_N4 = 0.0;
	value_SPS30_TS = 0.0;

	debug_outln_info(FPSTR(DBG_TXT_SEP));
	debug_outln_verbose(FPSTR(DBG_TXT_END_READING), FPSTR(SENSORS_SPS30));
}

/*****************************************************************
   read DNMS values
 *****************************************************************/
static void fetchSensorDNMS(String& s) {
	static bool dnms_error = false;
	debug_outln_verbose(FPSTR(DBG_TXT_START_READING), FPSTR(SENSORS_DNMS));
	last_value_dnms_laeq = -1.0;
	last_value_dnms_la_min = -1.0;
	last_value_dnms_la_max = -1.0;

	if (dnms_calculate_leq() != 0) {
		dnms_error = true;
	}

	uint16_t data_ready = 0;
	dnms_error = true;

	for (unsigned i = 0; i < 20; i++) {
		delay(2);
		int16_t ret_dnms = dnms_read_data_ready(&data_ready);
		if ((ret_dnms == 0) && (data_ready != 0)) {
			dnms_error = false;
			break;
		}
	}
	if (!dnms_error) {
		struct dnms_measurements dnms_values;
		if (dnms_read_leq(&dnms_values) == 0) {
			float dnms_corr_value = readCorrectionOffset(cfg::dnms_correction);
			last_value_dnms_laeq = dnms_values.leq_a + dnms_corr_value;
			last_value_dnms_la_min = dnms_values.leq_a_min + dnms_corr_value;
			last_value_dnms_la_max = dnms_values.leq_a_max + dnms_corr_value;
		} else {
			dnms_error = true;
		}
	}
	if (dnms_error) {
		dnms_reset(); // try to reset dnms
		debug_outln_error(F("DNMS read failed"));
	} else {
		add_Value2Json(s, F("DNMS_noise_LAeq"), F("LAeq: "), last_value_dnms_laeq);
		add_Value2Json(s, F("DNMS_noise_LA_min"), F("LA_MIN: "), last_value_dnms_la_min);
		add_Value2Json(s, F("DNMS_noise_LA_max"), F("LA_MAX: "), last_value_dnms_la_max);
	}
	debug_outln_info(FPSTR(DBG_TXT_SEP));
	debug_outln_verbose(FPSTR(DBG_TXT_END_READING), FPSTR(SENSORS_DNMS));
}

/*****************************************************************
 * read GPS sensor values                                        *
 *****************************************************************/
static __noinline void fetchSensorGPS(String& s) {
	debug_outln_verbose(FPSTR(DBG_TXT_START_READING), "GPS");

	if (gps.location.isUpdated()) {
		if (gps.location.isValid()) {
			last_value_GPS_lat = gps.location.lat();
			last_value_GPS_lon = gps.location.lng();
		} else {
			last_value_GPS_lat = 59.934;
			last_value_GPS_lon = 30.335;
			debug_outln_verbose(F("Lat/Lng INVALID"));
		}
		if (gps.altitude.isValid()) {
			last_value_GPS_alt = gps.altitude.meters();
		} else {
			last_value_GPS_alt = 200;
			debug_outln_verbose(F("Altitude INVALID"));
		}
		if (!gps.date.isValid()) {
			debug_outln_verbose(F("Date INVALID"));
		}
		if (!gps.time.isValid()) {
			debug_outln_verbose(F("Time: INVALID"));
		}
		if (gps.date.isValid() && gps.time.isValid()) {
			char gps_datetime[37];
			snprintf_P(gps_datetime, sizeof(gps_datetime), PSTR("%04d-%02d-%02dT%02d:%02d:%02d.%03d"),
				gps.date.year(), gps.date.month(), gps.date.day(), gps.time.hour(), gps.time.minute(), gps.time.second(), gps.time.centisecond());
			last_value_GPS_timestamp = gps_datetime;
		} else {
			//define a default value
			last_value_GPS_timestamp = F("1970-01-01T00:00:00.000");
		}
	} else {
		// last_value_GPS_lat = atof(cfg::lat_gps);
		// last_value_GPS_lon = atof(cfg::lon_gps);
		sscanf(cfg::coords_gps, "%lf,%lf", &last_value_GPS_lat, &last_value_GPS_lon);
	}
	if (send_now) {

		debug_outln_info(F("Lat: "), String(last_value_GPS_lat, 6));
		debug_outln_info(F("Lng: "), String(last_value_GPS_lon, 6));
		debug_outln_info(F("DateTime: "), last_value_GPS_timestamp);

		add_Value2Json(s, F("GPS_lat"), String(last_value_GPS_lat, 6));
		add_Value2Json(s, F("GPS_lon"), String(last_value_GPS_lon, 6));

		add_Value2Json(s, F("GPS_height"), F("Altitude: "), last_value_GPS_alt);
		add_Value2Json(s, F("GPS_timestamp"), last_value_GPS_timestamp);
		debug_outln_info(FPSTR(DBG_TXT_SEP));
	}

	if ( count_sends > 0 && gps.charsProcessed() < 10) {
		//debug_outln_error(F("No GPS data received: check wiring"));
		gps_init_failed = true;
	}

	debug_outln_verbose(FPSTR(DBG_TXT_END_READING), "GPS");
}

/*****************************************************************
 * OTAUpdate                                                     *
 *****************************************************************/

static bool fwDownloadStream(WiFiClient& client, const String& url, Stream* ostream) {

	HTTPClient http;
	int bytes_written = -1;

	// work with 128kbit/s downlinks
	http.setTimeout(60 * 1000);
	String agent(SOFTWARE_VERSION);
	agent += ' ';
	agent += esp_chipid;
	agent += "/";
	agent += esp_mac_id;
	agent += ' ';
	// agent += SDS_version_date();
	// agent += ' ';
	agent += String(cfg::current_lang);
	agent += ' ';
	agent += String(CURRENT_LANG);
	agent += ' ';
	if (cfg::use_beta) {
		agent += F("BETA");
	}

	http.setUserAgent(agent);
	http.setReuse(false);

	debug_outln_info(F("HTTP GET: "), String(FPSTR(FW_DOWNLOAD_HOST)) + ':' + String(FW_DOWNLOAD_PORT) + url);

	if (http.begin(client, FPSTR(FW_DOWNLOAD_HOST), FW_DOWNLOAD_PORT, url)) {
		int r = http.GET();
		debug_outln_info(F("GET r: "), String(r));
		last_update_returncode = r;
		if (r == HTTP_CODE_OK) {
			bytes_written = http.writeToStream(ostream);
		}
		http.end();
	}

	if (bytes_written > 0)
		return true;

	return false;
}

#if defined(ESP32)

bool downloadAndUpdate(const char* url, const String& expectedMD5) {
    WiFiClient client;
    HTTPClient http;
    http.begin(client, FPSTR(FW_DOWNLOAD_HOST), FW_DOWNLOAD_PORT, url);
    int httpCode = http.GET();
    if (httpCode != HTTP_CODE_OK) {
        debug_outln_info(F("Failed to download file, http code: "), httpCode);
        http.end();
        return false;
    }

    int contentLength = http.getSize();
    if (contentLength <= 0) {
        debug_outln_error(F("Content-Length not defined or invalid"));
        http.end();
        return false;
    }

    bool canBegin = Update.begin(contentLength);
    if (!canBegin) {
        debug_outln_error(F("Not enough space to begin OTA"));
        http.end();
        return false;
    }

    debug_outln_info(F("Begin OTA. This may take some time..."));

    WiFiClient *stream = http.getStreamPtr();
    size_t written = Update.writeStream(*stream);

    if (written == contentLength) {
        debug_outln_info(F("Written successfully: "), written);
    } else {
		debug_outln_info(F("Content length: "), contentLength);
        debug_outln_info(F("Written only: "), written);
    }

    if (Update.end()) {
        if (Update.isFinished()) {
            debug_outln_info(F("Update successfully completed."));
            String md5String = Update.md5String();
            if (md5String.equalsIgnoreCase(expectedMD5)) {
                debug_outln_info(F("MD5 verified successfully."));
                http.end();
                return true;
            } else {
                debug_outln_error(F("MD5 verification failed."));
            }
        } else {
            debug_outln_error(F("Update not finished? Something went wrong!"));
        }
    } else {
        debug_outln_error(F("Error Occurred during update"));
    }

    http.end();
    return false;
}

#endif

static bool fwDownloadStreamFile(WiFiClient& client, const String& url, const String& fname) {

	String fname_new(fname);
	fname_new += F(".new");
	bool downloadSuccess = false;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored  "-Wdeprecated-declarations"
	debug_outln_info(F("Start open file: "), fname_new);
	File fwFile = SPIFFS.open(fname_new, "w");
	if (fwFile) {
		downloadSuccess = fwDownloadStream(client, url, &fwFile);
		fwFile.close();
		if (downloadSuccess) {
			SPIFFS.remove(fname);
			SPIFFS.rename(fname_new, fname);
			debug_outln_info(F("Success downloading: "), url);
		} else {
			debug_outln_info(F("Download wasn't success"));
		}
	}

	if (downloadSuccess)
		return true;

	SPIFFS.remove(fname_new);
#pragma GCC diagnostic pop
	return false;
}

static void twoStageOTAUpdate() {
	if (!cfg::auto_update) return;

	debug_outln_info(F("twoStageOTAUpdate"));
	String lang_variant(cfg::current_lang);
	if (lang_variant.length() != 2) {
		lang_variant = CURRENT_LANG;
	}
	lang_variant.toLowerCase();
#if defined(ESP32)
	String fetch_name(F("/latest32c3_"));
#endif
#if defined(ESP8266)
	String fetch_name(F("/latest_"));
#endif
	fetch_name += lang_variant;
	fetch_name += F(".bin");

	WiFiClient client;
	String fetch_md5_name(fetch_name);
	fetch_md5_name += F(".md5");
	debug_outln_info(F("download md5 begin"));

	StreamString newFwmd5;
	if (!fwDownloadStream(client, fetch_md5_name, &newFwmd5)){
		debug_outln_info(F("download md5 fail"));
		return;}
	debug_outln_info(F("download md5 end"));

	newFwmd5.trim();
	if (newFwmd5 == ESP.getSketchMD5()) {
		debug_outln_info(F("No newer version available."));
		return;
	}

	debug_outln_info(F("Update md5: "), newFwmd5);
	debug_outln_info(F("Sketch md5: "), ESP.getSketchMD5());

#if defined(ESP32)
	if (downloadAndUpdate(fetch_name.c_str(), newFwmd5)) {
        sensor_restart();
    }
#endif

#if defined(ESP8266)
	// We're entering update phase, kill off everything else
	WiFiUDP::stopAll();
	WiFiClient::stopAllExcept(&client);
	delay(100);

	String firmware_name(F("/firmware.bin"));
	String firmware_md5(F("/firmware.bin.md5"));
	String loader_name(F("/loader.bin"));
	if (!fwDownloadStreamFile(client, fetch_name, firmware_name))
		return;
	if (!fwDownloadStreamFile(client, fetch_md5_name, firmware_md5))
		return;
	if (!fwDownloadStreamFile(client, FPSTR(FW_2ND_LOADER_URL), loader_name))
		return;

	// SPIFFS is deprecated, we know
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored  "-Wdeprecated-declarations"
	File fwFile = SPIFFS.open(firmware_name, "r");
	if (!fwFile) {
		SPIFFS.remove(firmware_name);
		SPIFFS.remove(firmware_md5);
		debug_outln_error(F("Failed reopening fw file.."));
		return;
	}
	size_t fwSize = fwFile.size();
	MD5Builder md5;
	md5.begin();
	md5.addStream(fwFile, fwSize);
	md5.calculate();
	fwFile.close();
	String md5String = md5.toString();

	// Firmware is always at least 128 kB and padded to 16 bytes
	if (fwSize < (1<<17) || (fwSize % 16 != 0) || newFwmd5 != md5String) {
		debug_outln_info(F("FW download failed validation.. deleting"));
		SPIFFS.remove(firmware_name);
		SPIFFS.remove(firmware_md5);
		return;
	}

	StreamString loaderMD5;
	if (!fwDownloadStream(client, String(FPSTR(FW_2ND_LOADER_URL)) + F(".md5"), &loaderMD5))
		return;

	loaderMD5.trim();

	debug_outln_info(F("launching 2nd stage"));
	if (!launchUpdateLoader(loaderMD5)) {
		debug_outln_error(FPSTR(DBG_TXT_UPDATE_FAILED));
		// display_debug(FPSTR(DBG_TXT_UPDATE), FPSTR(DBG_TXT_UPDATE_FAILED));
		SPIFFS.remove(firmware_name);
		SPIFFS.remove(firmware_md5);
		return;
	}
#pragma GCC diagnostic pop

	sensor_restart();
#endif
}

/*****************************************************************
   Init SPS30 PM Sensor
 *****************************************************************/
static void initSPS30() {
	char serial[SPS_MAX_SERIAL_LEN];
	debug_out(F("Trying SPS30 sensor on 0x69H "), DEBUG_MIN_INFO);
	sps30_reset();
	delay(200);
	if ( sps30_get_serial(serial) != 0 ) {
		debug_outln_info(FPSTR(DBG_TXT_NOT_FOUND));

		debug_outln_info(F("Check SPS30 wiring"));
		sps30_init_failed = true;
		return;
	}
	debug_outln_info(F(" ... found, Serial-No.: "), String(serial));
	if (sps30_set_fan_auto_cleaning_interval(SPS30_AUTO_CLEANING_INTERVAL) != 0) {
		debug_outln_error(F("setting of Auto Cleaning Intervall SPS30 failed!"));
		sps30_init_failed = true;
		return;
	}
	delay(100);
	if (sps30_start_measurement() != 0) {
		debug_outln_error(F("SPS30 error starting measurement"));
		sps30_init_failed = true;
		return;
	}
}

/*****************************************************************
   Init DNMS - Digital Noise Measurement Sensor
 *****************************************************************/
static void initDNMS() {
	char dnms_version[DNMS_MAX_VERSION_LEN + 1];

	debug_out(F("Trying DNMS sensor on 0x55H "), DEBUG_MIN_INFO);
	dnms_reset();
	delay(1000);
	if (dnms_read_version(dnms_version) != 0) {
		debug_outln_info(FPSTR(DBG_TXT_NOT_FOUND));
		debug_outln_error(F("Check DNMS wiring"));
		dnms_init_failed = true;
	} else {
		dnms_version[DNMS_MAX_VERSION_LEN] = 0;
		debug_outln_info(FPSTR(DBG_TXT_FOUND), String(": ") + String(dnms_version));
	}
}

const int maxSensors = 10;
Sensor* activeSensors[maxSensors];
int activeSensorsCount = 0;

static void powerOnTestSensors() {

	debug_outln_info(F("Current reg: "), cfg::current_reg);

	for (int i = 0; i < sizeof(supported_sensor_names) / sizeof(supported_sensor_names[0]); i++) {
		Sensor* new_sensor = createSensor(supported_sensor_names[i], cfg::sending_intervall_ms);
		if (new_sensor->begin()) {
			activeSensors[activeSensorsCount] = new_sensor;
			activeSensorsCount++;
			debug_outln_info(F("Sensor was added: "), supported_sensor_names[i]);
		} else {
			debug_outln_info(F("Sensor was not added: "), supported_sensor_names[i]);
		}
	}

}

const int APIsCount = 2;
API* activeAPIs[APIsCount];

RobonomicsDatalogAPI robonomicsDatalogAPI;
RobonomicsHTTPAPI robonomicsHTTPAPI;

static void logEnabledAPIs() {
	debug_outln_info(F("Send to :"));

	robonomicsDatalogAPI.setRobonomcis(&robonomics);
	robonomicsHTTPAPI.setRobonomcis(&robonomics);

	activeAPIs[0] = &robonomicsDatalogAPI;
	activeAPIs[1] = &robonomicsHTTPAPI;

	for (int i = 0; i < APIsCount; i++) {
		activeAPIs[i]->setup();
	}

}

static void setupNetworkTime() {
	// server name ptrs must be persisted after the call to configTime because internally
	// the pointers are stored see implementation of lwip sntp_setservername()
	static char ntpServer1[18], ntpServer2[18];
#if defined(ESP8266)
	settimeofday_cb([]() {
		if (!sntp_time_set) {
			time_t now = time(nullptr);
			debug_outln_info(F("SNTP synced: "), ctime(&now));
			twoStageOTAUpdate();
			last_update_attempt = millis();
		}
		sntp_time_set++;
	});
#endif
	strcpy_P(ntpServer1, NTP_SERVER_1);
	strcpy_P(ntpServer2, NTP_SERVER_2);
	configTime(0, 0, ntpServer1, ntpServer2);
}

static unsigned long sendDataToOptionalApis(const String &data) {
	unsigned long sum_send_time = 0;

	// if (cfg::send2influx) {
	// 	debug_outln_info(FPSTR(DBG_TXT_SENDING_TO), F("custom influx db: "));
	// 	RESERVE_STRING(data_4_influxdb, LARGE_STR);
	// 	create_influxdb_string_from_data(data_4_influxdb, data);
	// 	sum_send_time += sendData(LoggerInflux, data_4_influxdb, 0, cfg::host_influx, cfg::url_influx);
	// }

	// if (cfg::send2custom) {
	// 	String data_to_send = data;
	// 	data_to_send.remove(0, 1);
	// 	String data_4_custom(F("{\"esp8266id\": \""));
	// 	data_4_custom += esp_chipid;
	// 	data_4_custom += "\", ";
	// 	data_4_custom += data_to_send;
	// 	debug_outln_info(FPSTR(DBG_TXT_SENDING_TO), F("custom api: "));
	// 	sum_send_time += sendData(LoggerCustom, data_4_custom, 0, cfg::host_custom, cfg::url_custom);
	// }

	if (cfg::send2csv) {
		debug_outln_info(F("## Sending as csv: "));
		send_csv(data);
	}

	return sum_send_time;
}


/*****************************************************************
 * The Setup                                                     *
 *****************************************************************/

void setup(void) {
	delay(3000);
	// Debug.begin(115200);		// Output to Serial at 115200 from web console 
	// Debug.println("Start Setup");
	// printf("Start Setup print");
	Serial.begin(115200);
	Serial.println("Start setup");

#if defined(WIFI_LoRa_32_V2)
	// reset the OLED display, e.g. of the heltec_wifi_lora_32 board
	pinMode(RST_OLED, OUTPUT);
	digitalWrite(RST_OLED, LOW);
	delay(50);
	digitalWrite(RST_OLED, HIGH);
#endif

#if defined(ESP8266)
	esp_chipid = std::move(String(ESP.getChipId()));
	esp_mac_id = std::move(String(WiFi.macAddress().c_str()));
	esp_mac_id.replace(":", "");
	esp_mac_id.toLowerCase();
#endif
#if defined(ESP32)
	uint64_t chipid_num;
	chipid_num = ESP.getEfuseMac();
	esp_chipid = String((uint16_t)(chipid_num >> 32), HEX);
	esp_chipid += String((uint32_t)chipid_num, HEX);
#endif
	cfg::initNonTrivials(esp_chipid.c_str());
	WiFi.persistent(false);

	debug_outln_info(F("airRohr: " SOFTWARE_VERSION_STR "/"), String(CURRENT_LANG));
#if defined(ESP8266)
	if ((airrohr_selftest_failed = !ESP.checkFlashConfig() /* after 2.7.0 update: || !ESP.checkFlashCRC() */)) {
		debug_outln_error(F("ERROR: SELF TEST FAILED!"));
		SOFTWARE_VERSION += F("-STF");
	}
#endif

	init_config();
	// init_display();
	setupNetworkTime();
	logEnabledAPIs();
	webserver.setRobonomicsAddress(robonomics.getSs58Address());
	connectWifi(webserver);
	webserver.setup();
	debug_outln_info(F("\nChipId: "), esp_chipid);
	debug_outln_info(F("\nMAC Id: "), esp_mac_id);
	twoStageOTAUpdate();

	if (cfg::gps_read) {
// #if defined(ESP8266)
// 		serialGPS = new SoftwareSerial;
// 		serialGPS->begin(9600, SWSERIAL_8N1, GPS_SERIAL_RX, GPS_SERIAL_TX, false, 128);
// #endif
// #if defined(ESP32)
// 		serialGPS->begin(9600, SERIAL_8N1, GPS_SERIAL_RX, GPS_SERIAL_TX);
// #endif
		debug_outln_info(F("Read GPS..."));
		disable_unneeded_nmea();
	}

	powerOnTestSensors();
	// logEnabledDisplays();

	sensors_data["service_data"]["robonomics_address"] = robonomics.getSs58Address();
	sensors_data["service_data"]["signal_strength"] = WiFi.RSSI();

	delay(50);

	starttime = millis();									// store the start time
	last_datalog_time = millis();
	last_update_attempt = time_point_device_start_ms = starttime;
	last_display_millis = starttime_SDS = starttime_DB = starttime;

	// debug_outln_info(F("Sending to "), FPSTR(HOST_ROBONOMICS[num_of_robonomics_API]));
	debug_outln_info(F("Active Sensors count: "), activeSensorsCount);

	deviceStatus.last_update_attempt = deviceStatus.time_point_device_start_ms = millis();

}

/*****************************************************************
 * And action                                                    *
 *****************************************************************/

void loop(void) {
	bool isSDSRunning = false;
	for (int i = 0; i < activeSensorsCount; i++) {
		if (activeSensors[i]->sensor_name == SDS_SENSOR_NAME) {
			isSDSRunning = static_cast<SDS011Sensor*>(activeSensors[i])->getIsSDSRunning();
		}
		if (activeSensors[i]->sensor_name == I2S_NOISE_SENSOR_NAME) {
			static_cast<I2SNoiseSensor*>(activeSensors[i])->setSDSRunning(isSDSRunning);
		}
		if (activeSensors[i]->isTimeToFetch()) {
			activeSensors[i]->fetch(sensors_data);
		}
	}


	for (int i = 0; i < APIsCount; i++) {
		if (activeAPIs[i]->isTimeToSend()) {
			sensors_data["service_data"]["signal_strength"] = WiFi.RSSI();
			activeAPIs[i]->send(sensors_data);
			activeAPIs[i]->updateDeviceStatus(deviceStatus);
			Serial.println(F("Device Status:"));
			for (const auto& [api_name, status] : deviceStatus.apis_status) {
				Serial.print(F("API Name: "));
				Serial.println(api_name.c_str());
				Serial.print(F("  Count Sends: "));
				Serial.println(status.count_sends);
				Serial.print(F("  Last Send Time: "));
				Serial.println(ctime(&status.last_send_time));
				Serial.print(F("  Is OK: "));
				Serial.println(status.is_ok ? F("Yes") : F("No"));
			}
		}
	}
	webserver.handleClient();
	yield();
	// delay(100);
}
