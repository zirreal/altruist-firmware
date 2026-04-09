#include "zmod4510_sensor.h"
#include "zmod4510_config_no2_o3.h"

#include "sensor_names.h"
#include "../utils.h"
#include "../intl.h"
#include "drivers/i2c.h"

// Renesas NO2_O3 mode requires a 6 s sampling cadence
#define ZMOD4510_SENSOR_MIN_TIMEOUT 6000UL

ZMOD4510Sensor::ZMOD4510Sensor(unsigned long sending_timeout)
    : Sensor(sending_timeout)
{
    timeout = (sending_timeout > ZMOD4510_SENSOR_MIN_TIMEOUT)
                  ? sending_timeout
                  : ZMOD4510_SENSOR_MIN_TIMEOUT;
    sensor_name = ZMOD4510_SENSOR_NAME;
}

bool ZMOD4510Sensor::begin()
{
    debug_outln_info(F("Begin ZMOD4510Sensor"));

    if (i2c_master_init() != ESP_OK)
    {
        debug_outln_error(F("ZMOD4510 i2c_master_init failed"));
        return false;
    }

    if (zmod4510_fill_interface(&hal) != 0)
    {
        debug_outln_error(F("ZMOD4510 HAL init failed"));
        deinit_i2c();
        return false;
    }

    // Bind sensor-specific configuration tables from the Renesas NO2_O3 package
    dev.i2c_addr = ZMOD4510_I2C_ADDR;
    dev.pid = ZMOD4510_PID;
    dev.init_conf = &zmod_no2_o3_sensor_cfg[INIT];
    dev.meas_conf = &zmod_no2_o3_sensor_cfg[MEASUREMENT];
    dev.prod_data = prod_data;

    // Attach platform HAL callbacks and verify that the sensor responds on I2C
    last_error = zmod4xxx_init(&dev, &hal);
    if (last_error != ZMOD4XXX_OK)
    {
        debug_outln_error(F("zmod4xxx_init failed"));
        debug_outln_info(F("zmod4xxx_init error code: "), String(last_error));
        deinit_i2c();
        return false;
    }

    // Read calibration/configuration data from the module and prepare measurement mode
    last_error = zmod4xxx_read_sensor_info(&dev);
    if (last_error != ZMOD4XXX_OK)
    {
        debug_outln_error(F("zmod4xxx_read_sensor_info failed: "));
        debug_outln_info(F("zmod4xxx_read_sensor_info error code: "), String(last_error));
        deinit_i2c();
        return false;
    }

    last_error = zmod4xxx_prepare_sensor(&dev);
    if (last_error != ZMOD4XXX_OK)
    {
        debug_outln_error(F("zmod4xxx_prepare_sensor failed: "));
        debug_outln_info(F("zmod4xxx_prepare_sensor error code: "), String(last_error));
        deinit_i2c();
        return false;
    }

    // Initialize the Renesas NO2/O3 algorithm state once at startup
    last_error = init_no2_o3(&algo_handle);
    if (last_error != NO2_O3_OK)
    {
        debug_outln_error(F("init_no2_o3 failed"));
        debug_outln_info(F("init_no2_o3 error code: "), String(last_error));
        deinit_i2c();
        return false;
    }

    initialized = true;
    last_fetch_time = millis() - timeout;

    debug_outln_info(F("ZMOD4510 started with fetch interval (sec): "), String(timeout / 1000));

    deinit_i2c();
    return true;
}

void ZMOD4510Sensor::_fetch(JsonDocument &data)
{
    (void)data;

    if (!initialized)
    {
        return;
    }

    debug_outln_verbose(F("fetch ZMOD4510"));
}