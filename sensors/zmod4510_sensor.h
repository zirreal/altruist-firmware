#ifndef __ZMOD4510_H__
#define __ZMOD4510_H__

#include "sensor.h"

extern "C"
{
#include "zmod4xxx.h"
#include "zmod4xxx_types.h"
#include "no2_o3.h"
#include "hal.h"
#include "zmod4xxx_hal.h"
#include "zmod4510_hal_port.h"
}

class ZMOD4510Sensor : public Sensor
{

public:
    ZMOD4510Sensor(unsigned long sending_timeout = 1000UL);

    bool begin() override;

private:
    void _fetch(JsonDocument &data) override;

    // Renesas HAL interface populated with ESP32-specific I2C/delay callbacks
    Interface_t hal = {};

    // Renesas sensor device context used by zmod4xxx_* API calls
    zmod4xxx_dev_t dev = {};

    // Persistent algorithm state required by the NO2/O3 library
    no2_o3_handle_t algo_handle = {};
    no2_o3_inputs_t algo_input = {};
    no2_o3_results_t algo_results = {};

    static constexpr size_t ADC_DATA_LEN = 32;
    static constexpr size_t PROD_DATA_LEN = 10;
    uint8_t adc_result[ADC_DATA_LEN] = {};
    uint8_t prod_data[PROD_DATA_LEN] = {};
    uint8_t status = 0;

    bool initialized = false;
    int last_error = 0;

    float last_o3_value = 0.0f;
    float last_no2_value = 0.0f;
    float last_fast_aqi_value = 0.0f;
    float last_epa_aqi_value = 0.0f;
};

#endif // __ZMOD4510_H__