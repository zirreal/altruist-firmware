#ifndef SENSOR_H
#define SENSOR_H

#include "Arduino.h"
#include <ArduinoJson.h>

class Sensor {
protected:
  unsigned long sending_timeout;  // Private variable for sending timeout
  unsigned long timeout;  // Private variable for sensor timeout
  unsigned long last_fetch_time;

  virtual void _fetch(JsonDocument &data) = 0;

  void updateFetchTime() {
    last_fetch_time = millis();
  }

  void addValueToJSON(JsonDocument &data, const String &meas_id, String &value, const char* intl_name, const String &units) {
    data[sensor_name][meas_id][F("value")] = value;
    data[sensor_name][meas_id][F("intl_name")] = intl_name;
    data[sensor_name][meas_id][F("units")] = units;
  }

  void addValueToJSON(JsonDocument &data, const String &meas_id, uint8_t &value, const char* intl_name, const String &units) {
    data[sensor_name][meas_id][F("value")] = value;
    data[sensor_name][meas_id][F("intl_name")] = intl_name;
    data[sensor_name][meas_id][F("units")] = units;
  }

public:
  // Constructor with a default timeout value (e.g., 1000 milliseconds)
  Sensor(unsigned long sending_timeout = 1000UL) : sending_timeout(sending_timeout) {}

  virtual ~Sensor() {}

  const char* sensor_name;

  // Pure virtual function to attempt sensor initialization.
  virtual bool begin() = 0;

  void fetch(JsonDocument &data) {
    _fetch(data);
    updateFetchTime();
  };

  // Optional getter for the timeout.
  bool isTimeToFetch() const {
    return (millis() - last_fetch_time > timeout);
  }
};

#endif  // SENSOR_H