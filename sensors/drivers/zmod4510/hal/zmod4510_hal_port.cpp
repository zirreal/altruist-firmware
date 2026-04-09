#include "zmod4510_hal_port.h"

#include <Arduino.h>
#include "../../i2c.h"

static constexpr TickType_t ZMOD_I2C_TIMEOUT_TICKS = pdMS_TO_TICKS(200);

// This HAL expects the ESP32 I2C driver to be initialized by the caller
// before any Renesas ZMOD API function is invoked.
static int esp_i2c_read(void *handle,
                        uint8_t slave_addr,
                        uint8_t *tx_buf,
                        int tx_len,
                        uint8_t *rx_buf,
                        int rx_len)
{
    (void)handle;

    if (rx_len <= 0 || rx_buf == nullptr || tx_len < 0)
    {
        return -1;
    }

    esp_err_t err;
    if (tx_len > 0 && tx_buf != nullptr)
    {
        err = i2c_master_write_read_device(
            I2C_MASTER_NUM, slave_addr, tx_buf, tx_len, rx_buf, rx_len, ZMOD_I2C_TIMEOUT_TICKS);
    }
    else
    {
        err = i2c_master_read_from_device(
            I2C_MASTER_NUM, slave_addr, rx_buf, rx_len, ZMOD_I2C_TIMEOUT_TICKS);
    }

    return (err == ESP_OK) ? 0 : -1;
}

static int esp_i2c_write(void *handle,
                         uint8_t slave_addr,
                         uint8_t *tx_buf1,
                         int tx_len1,
                         uint8_t *tx_buf2,
                         int tx_len2)
{
    (void)handle;

    if (tx_len1 < 0 || tx_len2 < 0)
    {
        return -1;
    }

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    if (cmd == nullptr)
    {
        return -1;
    }

    esp_err_t err = i2c_master_start(cmd);
    if (err == ESP_OK)
    {
        err = i2c_master_write_byte(cmd, (slave_addr << 1) | I2C_MASTER_WRITE, true);
    }
    if (err == ESP_OK && tx_len1 > 0 && tx_buf1 != nullptr)
    {
        err = i2c_master_write(cmd, tx_buf1, tx_len1, true);
    }
    if (err == ESP_OK && tx_len2 > 0 && tx_buf2 != nullptr)
    {
        err = i2c_master_write(cmd, tx_buf2, tx_len2, true);
    }
    if (err == ESP_OK)
    {
        err = i2c_master_stop(cmd);
    }
    if (err == ESP_OK)
    {
        err = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, ZMOD_I2C_TIMEOUT_TICKS);
    }

    i2c_cmd_link_delete(cmd);
    return (err == ESP_OK) ? 0 : -1;
}

static void esp_ms_sleep(uint32_t ms)
{
    delay(ms);
}

int zmod4510_fill_interface(Interface_t *hal)
{
    if (hal == nullptr)
    {
        return -1;
    }

    hal->handle = nullptr;
    hal->i2cRead = esp_i2c_read;
    hal->i2cWrite = esp_i2c_write;
    hal->msSleep = esp_ms_sleep;
    hal->reset = nullptr;

    return 0;
}
