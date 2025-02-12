#ifndef SENSOR_FACTORY_H
#define SENSOR_FACTORY_H

#include "sensor.h"
#include "bmx280i2c_sensor.h"
#include "sds011_sensor.h"
#include "i2s_noise_sensor.h"

String supported_sensor_names[] = {"SDS011", "BMX280", "NoiseSensor"};

Sensor* createSensor(const String &sensorType, unsigned long sending_timeout) {
  if (sensorType == "SDS011") {
    return new SDS011Sensor(sending_timeout);
  } else if (sensorType == "BMX280") {
    return new BMX280Sensor(sending_timeout);
  } else if (sensorType == "NoiseSensor") {
    return new I2SNoiseSensor(sending_timeout);
  }
  return nullptr;
}

#endif  // SENSOR_FACTORY_H