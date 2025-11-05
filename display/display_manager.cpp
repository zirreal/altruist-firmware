#ifdef ALTRUIST_INSIDE

#include "display_manager.h"
#include "screens/screens.h"
#include "../defines.h"
#include "utils.h"
#include <SPIFFS.h>
#include "../leds/leds_controller_insight.h"

extern LedControllerInsight leds_controller_insight;

bool display_sleeping = false;

// Cycle order for screens when navigating with UP/SET
ScreenPage DisplayManager::getNextScreen(ScreenPage current) {
    if (current == ScreenPage::SENSOR_MAP) return ScreenPage::MAIN;
    if (current == ScreenPage::MAIN) return ScreenPage::GRAPHS;
    if (current == ScreenPage::GRAPHS) return ScreenPage::SENSOR_MAP;
    // Default: go to MAIN from other screens
    return ScreenPage::MAIN;
}

ScreenPage DisplayManager::getPrevScreen(ScreenPage current) {
    if (current == ScreenPage::SENSOR_MAP) return ScreenPage::GRAPHS;
    if (current == ScreenPage::MAIN) return ScreenPage::SENSOR_MAP;
    if (current == ScreenPage::GRAPHS) return ScreenPage::MAIN;
    // Default: go to MAIN from other screens
    return ScreenPage::MAIN;
}

void DisplayManager::setup() {
    createNewImage(BlackImage);
    // Load cached Urban address from SPIFFS to survive power cycles
    if (SPIFFS.begin(true)) {
        if (SPIFFS.exists("/urban_ss58.cache")) {
            File f = SPIFFS.open("/urban_ss58.cache", "r");
            if (f) {
                String v = f.readString();
                v.trim();
                if (v.length() > 0) cached_urban_address = v;
                f.close();
            }
        }
    }
}

void DisplayManager::setScreen(ScreenPage pageID) {
    currentScreenID = pageID;
    refresh_now = true;
}

void DisplayManager::process(button_pressed_t &btn_press) {
    if (btn_press.pressed && !btn_press.double_long) {
        btn_press.pressed = false;

        if (display_sleeping) {
            if (btn_press.button_num == ButtonNum::DOWN) {
                EPD_4IN2_V2_Init();
                DEV_Delay_ms(500);
                display_sleeping = false;
                // Restore LEDs after wake
                leds_controller_insight.setSleepMode(false);
                // Show sensor map on wake
                setScreen(ScreenPage::SENSOR_MAP);
                // Start 30s auto-transition back to MAIN after wake
                auto_to_main_active = true;
                auto_to_main_deadline_ms = millis() + 30000;
            }
            return;
        }
        else {
            // Global: long DOWN to sleep from any screen
            if (btn_press.button_num == ButtonNum::DOWN && btn_press.press_type == PressType::LONG) {
                initAndClearScreen();
                Paint_DrawString_EN_Center("Going to sleep...", &Font24, WHITE, BLACK);
                EPD_4IN2_V2_Init();
                DEV_Delay_ms(200);
                EPD_4IN2_V2_Display(BlackImage);
                DEV_Delay_ms(700);
                EPD_4IN2_V2_Clear();
                DEV_Delay_ms(300);
                // Turn off LEDs during sleep
                leds_controller_insight.setSleepMode(true);
                EPD_4IN2_V2_Sleep();
                DEV_Delay_ms(100);
                display_sleeping = true;
                return;
            }
            if (currentScreenID == ScreenPage::MAIN) {
                // Short presses: navigate screens
                if (btn_press.press_type == PressType::SHORT) {
                    if (btn_press.button_num == ButtonNum::UP) {
                        setScreen(getPrevScreen(currentScreenID));
                        return;
                    } else if (btn_press.button_num == ButtonNum::SET) {
                        setScreen(getNextScreen(currentScreenID));
                        return;
                    }
                }
            } 
            else if (currentScreenID == ScreenPage::GRAPHS) {
                if (btn_press.button_num == ButtonNum::DOWN) {
                    setNextGraphScreen();
                    refresh_now = true;
                    return;
                }
                if (btn_press.press_type == PressType::SHORT) {
                    if (btn_press.button_num == ButtonNum::UP) {
                        setScreen(getPrevScreen(currentScreenID));
                        return;
                    } else if (btn_press.button_num == ButtonNum::SET) {
                        setScreen(getNextScreen(currentScreenID));
                        return;
                    }
                }
            }
            else if (currentScreenID == ScreenPage::SENSOR_MAP) {
                if (btn_press.press_type == PressType::SHORT) {
                    if (btn_press.button_num == ButtonNum::UP) {
                        setScreen(getPrevScreen(currentScreenID));
                        return;
                    } else if (btn_press.button_num == ButtonNum::SET) {
                        setScreen(getNextScreen(currentScreenID));
                        return;
                    }
                }
            }
        }
    }
    // Handle auto-navigation from SENSOR_MAP to MAIN ~30s after wake
    if (auto_to_main_active && currentScreenID == ScreenPage::SENSOR_MAP) {
        if ((int32_t)(millis() - auto_to_main_deadline_ms) >= 0) {
            auto_to_main_active = false;
            setScreen(ScreenPage::MAIN);
        }
    }

    if (refresh_time_for_qr > 0 && msSince(refresh_time_for_qr) > 30000) {
        refresh_time_for_qr = 0;
        refresh_now = true;
    }
    // Update cached Urban address every cycle; if it changes and we're on SENSOR_MAP, trigger redraw
    if (sensors_data.containsKey("service_data")) {
        auto service = sensors_data["service_data"].as<JsonObject>();
        if (!service.isNull() && service.containsKey("urban_robonomics_address")) {
            String urban_addr = service["urban_robonomics_address"].as<String>();
            if (urban_addr.length() > 0 && cached_urban_address != urban_addr) {
                bool was_empty = cached_urban_address.length() == 0;
                cached_urban_address = urban_addr;
                if (currentScreenID == ScreenPage::SENSOR_MAP || was_empty) {
                    refresh_now = true; // force first render after boot/flash
                }
                // Persist to SPIFFS so we have it after power cycles
                if (SPIFFS.begin(true)) {
                    File f = SPIFFS.open("/urban_ss58.cache", "w");
                    if (f) { f.print(cached_urban_address); f.close(); }
                }
            }
        }
    }

    if (currentScreenID == ScreenPage::SENSOR_MAP && sensor_map_waiting_addr && (int32_t)(millis() - next_sensor_map_check_ms) >= 0) {
        refresh_now = true;
    }

    if (msSince(last_refresh_time) > DISPLAY_REFRESH_INTERVAL || refresh_now || currentScreenID == ScreenPage::CONNECTING) {
        refresh_now = false;
        initAndClearScreen();
        // Show sensors map every few refresh cycles when on main screen
        if (msSince(last_refresh_time) > DISPLAY_REFRESH_INTERVAL && currentScreenID == ScreenPage::MAIN) {
            if (refresh_count_for_qr >= 1) { // Show sensors map after 1 main screen refresh
                refresh_count_for_qr = 0;
                refresh_time_for_qr = millis();
                // Update cached Urban address if present in sensors_data
                if (sensors_data.containsKey("service_data")) {
                    auto service = sensors_data["service_data"].as<JsonObject>();
                    if (!service.isNull() && service.containsKey("urban_robonomics_address")) {
                        String urban_addr = service["urban_robonomics_address"].as<String>();
                        if (urban_addr.length() > 0 && cached_urban_address != urban_addr) {
                            cached_urban_address = urban_addr;
                        }
                    }
                }
                String addr = cached_urban_address; // may be empty -> default link
                showSensorsMapPage(addr);
                last_refresh_time = millis();
                showImageLong(BlackImage);
                return;
            } else {
                refresh_count_for_qr++;
            }
        }
        if (currentScreenID == ScreenPage::MAIN) {
            String jsonString;
		    serializeJson(sensors_data, jsonString);
            debug_outln_info(F("Refresh main screen"));
            drawMainScreen(BlackImage, jsonString, deviceStatus.ip_address);
        } else if (currentScreenID == ScreenPage::GRAPHS) {
            drawGraphScreen();
        } else if (currentScreenID == ScreenPage::SETUP) {
            showSetupPage(BlackImage);
        } else if (currentScreenID == ScreenPage::LOADING) {
            showLoadingPage(BlackImage);
        } else if (currentScreenID == ScreenPage::CONNECTING) {
            // Simple static connecting screen
            debug_outln_info(F("Showing connecting screen"));
            showConnectingPage(BlackImage, 25);
        } else if (currentScreenID == ScreenPage::LOGO) {
            showLogoPage();
        } else if (currentScreenID == ScreenPage::SENSOR_MAP) {
            // Refresh cache if Urban address present; if it appears newly, force redraw immediately
            if (sensors_data.containsKey("service_data")) {
                auto service = sensors_data["service_data"].as<JsonObject>();
                if (!service.isNull() && service.containsKey("urban_robonomics_address")) {
                    String urban_addr = service["urban_robonomics_address"].as<String>();
                    if (urban_addr.length() > 0 && cached_urban_address != urban_addr) {
                        cached_urban_address = urban_addr;
                        refresh_now = true; // trigger immediate redraw with correct link
                    }
                }
            }
            sensor_map_waiting_addr = false;
            String addr = cached_urban_address; // empty => default link
            showSensorsMapPage(addr);
        }
        last_refresh_time = millis();
        showImageLong(BlackImage);
    }
}

#endif // ALTRUIST_INSIDE