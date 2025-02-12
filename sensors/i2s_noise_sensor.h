#ifndef __I2SNOISE_H__
#define __I2SNOISE_H__

#include "sensor.h"

class I2SNoiseSensor : public Sensor
{

public:
    I2SNoiseSensor(unsigned long sending_timeout = 1000UL);

    bool begin() override;
    void reset_values()
    {
        last_value_DBMETER_max = 0;
        last_value_DBMETER_mean = 0;
        last_value_DBMETER_count = 0;
        last_value_DBMETER_sum = 0;
    }
    void setSDSRunning(bool running) { is_SDS_running = running; }

private:
    void _fetch(JsonDocument &data) override;
    bool is_SDS_running = false;
    uint8_t last_value_DBMETER = 0;
    uint8_t last_value_DBMETER_max = 0;
    uint32_t last_value_DBMETER_sum = 0;
    uint8_t last_value_DBMETER_count = 0;
    float last_value_DBMETER_mean = 0;
};

#endif // __I2SNOISE_H__