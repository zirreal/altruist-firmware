#include "ags3871_sensor.h"

#include "sensor_names.h"
#include "../utils.h"
#include "drivers/i2c.h"

AGS3871Sensor::AGS3871Sensor(unsigned long sending_timeout)
    : Sensor(sending_timeout)
{
    timeout = sending_timeout;
    sensor_name = AGS3871_SENSOR_NAME;
}

bool AGS3871Sensor::begin()
{
    debug_outln_info(F("Begin AGS3871Sensor"));
    debug_outln_info(F("AGS3871 I2C address: 0x"), String(AGS3871Driver::ADDRESS, HEX));

    if (i2c_master_init() != ESP_OK)
    {
        debug_outln_error(F("AGS3871 i2c_master_init failed"));
        return false;
    }

    initialized = ags3871.begin();
    if (initialized)
    {
        warmup_started_at_ms = millis();
        last_fetch_time = millis() - timeout;
        debug_outln_info(F("AGS3871 found on I2C"));
    }
    else
    {
        debug_outln_error(F("AGS3871 not found on I2C"));
        debug_outln_info(F("AGS3871 last I2C error: "), String(ags3871.lastI2CError()));
    }

    deinit_i2c();
    return initialized;
}

void AGS3871Sensor::_fetch(JsonDocument &data)
{
    (void)data;

    // Minimal bring-up step: begin() only checks that the module ACKs on I2C.
    // Reading CO values will be added after the hardware presence test passes.
}
