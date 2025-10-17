#ifdef ALTRUIST_INSIDE

#include "display_manager.h"
#include "screens/screens.h"
#include "../defines.h"

void DisplayManager::setup() {
    createNewImage(BlackImage);
}

void DisplayManager::setScreen(ScreenPage pageID) {
    currentScreenID = pageID;
    refresh_now = true;
}

void DisplayManager::process(button_pressed_t &btn_press) {
    if (btn_press.pressed && !btn_press.double_long) {
        btn_press.pressed = false;
        if (currentScreenID == ScreenPage::MAIN) {
            if (btn_press.button_num == ButtonNum::DOWN || btn_press.button_num == ButtonNum::UP) {
                currentScreenID = ScreenPage::GRAPHS;
            }
            refresh_now = true;
        } else if (currentScreenID == ScreenPage::GRAPHS) {
            if (btn_press.button_num == ButtonNum::DOWN) {
                setNextGraphScreen();
            } else if (btn_press.button_num == ButtonNum::UP) {
                setPrevGraphScreen();
            } else if (btn_press.button_num == ButtonNum::SET) {
                currentScreenID = ScreenPage::MAIN;
                current_graph_screen = 1;
            }
            refresh_now = true;
        }
    }
    if (refresh_time_for_qr > 0 && msSince(refresh_time_for_qr) > 30000) {
        refresh_time_for_qr = 0;
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
                // When Urban is connected (has IP_address), prefer its Robonomics address stored in service_data
                bool urban_connected = false;
                if (sensors_data.containsKey(ATRUIST_URBAN_SENSOR)) {
                    auto urban = sensors_data[ATRUIST_URBAN_SENSOR].as<JsonObject>();
                    if (!urban.isNull() && urban.containsKey("IP_address")) {
                        urban_connected = true;
                    }
                }
                String addr = String("");
                if (urban_connected && sensors_data.containsKey("service_data")) {
                    auto service = sensors_data["service_data"].as<JsonObject>();
                    if (!service.isNull() && service.containsKey("urban_robonomics_address")) {
                        String urban_addr = service["urban_robonomics_address"].as<String>();
                        if (urban_addr.length() > 0) {
                            addr = urban_addr;
                        }
                    }
                }
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
            // When Urban is connected (has IP_address), prefer its Robonomics address stored in service_data
            bool urban_connected = false;
            if (sensors_data.containsKey(ATRUIST_URBAN_SENSOR)) {
                auto urban = sensors_data[ATRUIST_URBAN_SENSOR].as<JsonObject>();
                if (!urban.isNull() && urban.containsKey("IP_address")) {
                    urban_connected = true;
                }
            }
            String addr = String("") ;
            if (urban_connected && sensors_data.containsKey("service_data")) {
                auto service = sensors_data["service_data"].as<JsonObject>();
                if (!service.isNull() && service.containsKey("urban_robonomics_address")) {
                    String urban_addr = service["urban_robonomics_address"].as<String>();
                    if (urban_addr.length() > 0) {
                        addr = urban_addr;
                    }
                }
            }
            showSensorsMapPage(addr);
        }
        last_refresh_time = millis();
        showImageLong(BlackImage);
    }
}

#endif // ALTRUIST_INSIDE