/*
 * ESP32-specific HAL adapter for the Renesas ZMOD4510 SDK.
 *
 * The Renesas SDK does not talk to ESP-IDF/Arduino I2C directly. Instead it
 * expects an Interface_t filled with platform callbacks for:
 * - I2C read
 * - I2C write
 * - millisecond delay
 * - optional hardware reset
 *
 * This file provides those callbacks using the project's existing ESP32 I2C
 * driver layer.
 */

#ifndef ZMOD4510_HAL_PORT_H
#define ZMOD4510_HAL_PORT_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "hal.h"

    /*
    * Populate a Renesas Interface_t with ESP32 callback implementations.
    *
    * This is the entry point used by ZMOD4510Sensor::begin() before calling
    * zmod4xxx_init(...).
    */
    int zmod4510_fill_interface(Interface_t *hal);

#ifdef __cplusplus
}
#endif

#endif // ZMOD4510_HAL_PORT_H