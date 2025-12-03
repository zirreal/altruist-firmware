#ifdef ALTRUIST_INSIDE

#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <stdlib.h>
#include <ArduinoJson.h>
#include "paint_driver/GUI_Paint.h"
#include "driver/EPD.h"
#include "driver/DEV_Config.h"
#include "../utils.h"
#include "../buttons/button_manager.h"

#define DISPLAY_REFRESH_INTERVAL 300000L

enum class ScreenPage {
    MAIN,
    GRAPHS,
    CONNECTING,
    SETUP,
    LOADING,
    LOGO,
    SENSOR_MAP,
    SETTINGS
};

class DisplayManager {
public:
    DisplayManager(JsonDocument &_data, device_status_t &_deviceStatus) : sensors_data(_data), deviceStatus(_deviceStatus) {}

    void setup();
    void process(button_pressed_t &btn_press);
    void setScreen(ScreenPage pageID);
    void setRobonomicsAddress(const char *address) { robonomics_address = String(address);};
private:
    device_status_t &deviceStatus;
    JsonDocument &sensors_data;
    bool refresh_now = false;
    String robonomics_address;
    String cached_urban_address; // cached Urban Robonomics address once discovered
    uint8_t refresh_count_for_qr = 0;
    uint32_t refresh_time_for_qr = 0;
    UBYTE *BlackImage;

    ScreenPage currentScreenID = ScreenPage::MAIN;
    unsigned long last_refresh_time = -DISPLAY_REFRESH_INTERVAL;

    // Screen navigation helpers
    ScreenPage getNextScreen(ScreenPage current);
    ScreenPage getPrevScreen(ScreenPage current);

    // Auto navigation after wake: SENSOR_MAP -> MAIN in ~30s
    bool auto_to_main_active = false;
    uint32_t auto_to_main_deadline_ms = 0;

    // Retry updating Sensor Map QR when Urban address not yet available
    bool     sensor_map_waiting_addr   = false;
    uint32_t next_sensor_map_check_ms  = 0;
    uint8_t  sensor_map_waiting_tries  = 0;

};

#endif // DISPLAY_MANAGER_H

#endif