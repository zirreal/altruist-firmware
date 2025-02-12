#include "i2s_noise_sensor.h"
#include "../utils.h"
#include "helpers/i2s_noise/i2s_noise.h"
#include "../intl.h"

#define NOISE_READ_TIMEOUT 1000

I2SNoiseSensor::I2SNoiseSensor(unsigned long sending_timeout)
    : Sensor(sending_timeout) {
    timeout = NOISE_READ_TIMEOUT;
    sensor_name = "Noise Sensor";
}

bool I2SNoiseSensor::begin() {
    float db_mean = 0;
    fetchSensorI2sSound(&last_value_DBMETER, &db_mean);
    return db_mean > 0;
}

void I2SNoiseSensor::_fetch(JsonDocument &data) {
	debug_outln_verbose(FPSTR(DBG_TXT_START_READING), sensor_name);
	if (is_SDS_running) {
		debug_outln_verbose(F("Don't measure noise: SDS is running"));
	} else {
		float db_mean;
		fetchSensorI2sSound(&last_value_DBMETER, &db_mean);
		if (last_value_DBMETER > last_value_DBMETER_max) {
			last_value_DBMETER_max = last_value_DBMETER;
		}
		last_value_DBMETER_sum += db_mean;
		last_value_DBMETER_count++;
		last_value_DBMETER_mean = (float)last_value_DBMETER_sum / (float)last_value_DBMETER_count;
	}
    // debug_outln_info(F("Noise sum: "), last_value_DBMETER_sum);
    // debug_outln_info(F("Noise count: "), last_value_DBMETER_count);
    // debug_outln_info(F("Noise max: "), last_value_DBMETER_max);
    // debug_outln_info(F("Noise mean: "), last_value_DBMETER_mean);
    debug_outln_info(FPSTR(DBG_TXT_SEP));
    addValueToJSON(data, F("noiseMax"), last_value_DBMETER_max, INTL_NOISE_MAX, F("dB"));
    addValueToJSON(data, F("noiseAvg"), last_value_DBMETER_mean, INTL_NOISE_MEAN, F("dB"));
	debug_outln_verbose(FPSTR(DBG_TXT_END_READING), sensor_name);
    updateFetchTime();
}

