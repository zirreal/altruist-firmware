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

#define DISPLAY_REFRESH_INTERVAL 60000L  // 1 minute (was 5 minutes)
#define QR_MAP_AUTO_INTERVAL 7200000L   // 2 hours in milliseconds

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
    unsigned long last_qr_map_show_time = 0; // Track when QR map was last auto-shown
    UBYTE *BlackImage;

    ScreenPage currentScreenID = ScreenPage::MAIN;
    unsigned long last_refresh_time = -DISPLAY_REFRESH_INTERVAL;

    // Screen navigation helpers
    ScreenPage getNextScreen(ScreenPage current);
    ScreenPage getPrevScreen(ScreenPage current);

    // Auto navigation after wake: SENSOR_MAP -> MAIN in ~30s
    bool auto_to_main_active = false;
    uint32_t auto_to_main_deadline_ms = 0;
    
    // Force full refresh on next update (e.g., after wake from sleep)
    bool force_full_refresh = false;

    // Retry updating Sensor Map QR when Urban address not yet available
    bool     sensor_map_waiting_addr   = false;
    uint32_t next_sensor_map_check_ms  = 0;
    uint8_t  sensor_map_waiting_tries  = 0;

};

#endif // DISPLAY_MANAGER_H

#endif