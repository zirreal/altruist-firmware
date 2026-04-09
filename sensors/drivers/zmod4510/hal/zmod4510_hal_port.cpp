// This file is the platform bridge between the generic Renesas HAL interface
// and the ESP32 I2C implementation used by this firmware.

#include "zmod4510_hal_port.h"

#include <Arduino.h>
#include "../../i2c.h"

static constexpr TickType_t ZMOD_I2C_TIMEOUT_TICKS = pdMS_TO_TICKS(200);

// Renesas HAL read callback.
// For register reads, the SDK first writes a register pointer and then performs
// a repeated-start read from the same slave address.
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

// Renesas HAL write callback.
// The SDK may pass up to two transmit buffers: typically a register address
// followed by payload bytes. For device probing it may also call this function
// with zero-length payloads.
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

// Renesas SDK relies on millisecond delays to preserve measurement timing.
static void esp_ms_sleep(uint32_t ms)
{
    delay(ms);
}

// Wire the generic Renesas interface to this project's ESP32-specific
// implementations. The caller is still responsible for initializing the I2C
// peripheral before using the sensor.
// Important: this HAL layer does not own the I2C bus lifecycle.
// It only provides read/write/delay callbacks expected by the Renesas SDK.
// Bus initialization and teardown are handled by the sensor driver.
// zmod4xxx_init(...) from the Renesas SDK consumes Interface_t and maps these
// callbacks into the legacy zmod4xxx_dev_t read/write/delay function pointers.
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
    // Hardware reset is intentionally not provided because RES_N is not connected
    // on the current board revision.
    hal->reset = nullptr;

    return 0;
}
