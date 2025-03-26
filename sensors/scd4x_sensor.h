#ifndef __SCD4x_H__
#define __SCD4x_H__

#include "sensor.h"
#include "drivers/SCD4x_Library/src/SparkFun_SCD4x_Arduino_Library.h"

class SCD4xSensor : public Sensor
{

public:
    SCD4xSensor(unsigned long sending_timeout = 1000UL);

    bool begin() override;

private:
    void _fetch(JsonDocument &data) override;
    SCD4x mySensor;
    float last_co2_value = 0.0;
    float last_temerature_value = 0.0;
    float last_humidity_value = 0.0;
};

#endif // __SCD4x_H__