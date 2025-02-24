#ifndef __I2C_H__
#define __I2C_H__

#include "driver/i2c.h"
#include <Arduino.h>

#define I2C_MASTER_FREQ_HZ         100000
#define I2C_MASTER_TX_BUF_DISABLE  0
#define I2C_MASTER_RX_BUF_DISABLE  0
#define I2C_MASTER_NUM             I2C_NUM_0

esp_err_t i2c_master_init(void);

void deinit_i2c(void);

#endif  // __I2C_H__