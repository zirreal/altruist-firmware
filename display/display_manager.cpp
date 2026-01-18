#ifdef ALTRUIST_INSIDE

#include "display_manager.h"
#include "screens/screens.h"
#include "screens/graph.h"
#include "screens/display_common.h"
#include "../defines.h"
#include "utils.h"
#include <SPIFFS.h>
#include <WiFi.h>
#include "../leds/leds_controller_insight.h"
#include "../config_manager/config_helpers.h"
#ifdef DISPLAY_4IN2
#include "driver/EPD_4in2_SSD1683.h"
#endif

extern LedControllerInsight leds_controller_insight;

bool display_sleeping = false;

// Cycle order for screens when navigating with UP/SET
// Order: MAIN -> GRAPHS -> SENSOR_MAP -> SETTINGS -> MAIN
ScreenPage DisplayManager::getNextScreen(ScreenPage current) {
    if (current == ScreenPage::MAIN) return ScreenPage::GRAPHS;
    if (current == ScreenPage::GRAPHS) return ScreenPage::SENSOR_MAP;
    if (current == ScreenPage::SENSOR_MAP) return ScreenPage::SETTINGS;
    if (current == ScreenPage::SETTINGS) return ScreenPage::MAIN;
    // Default: go to MAIN from other screens
    return ScreenPage::MAIN;
}

ScreenPage DisplayManager::getPrevScreen(ScreenPage current) {
    if (current == ScreenPage::MAIN) return ScreenPage::SETTINGS;
    if (current == ScreenPage::SETTINGS) return ScreenPage::SENSOR_MAP;
    if (current == ScreenPage::SENSOR_MAP) return ScreenPage::GRAPHS;
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
    // Initialize display and show loading screen immediately
    // initAndClearScreen();
    // Paint_SelectImage(BlackImage);
    // Paint_Clear(WHITE);
    if (currentScreenID == ScreenPage::LOADING) {
        showLoadingPage(BlackImage);
        showImageFast(BlackImage, currentScreenID);
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
            // Wake up from any button press (DOWN, UP, or SET)
            // Display wake: FULL initialization and update
            debug_outln_info(F("[EPD] Wake up from sleep - FULL init and update"));
            // Reset display state
            epdResetState();
            // FULL initialization (like on first power-on)
            EPD_4IN2_V2_Init();
            Paint_Clear(WHITE);
            display_sleeping = false;
            // Restore LEDs after wake - do this BEFORE screen update
            leds_controller_insight.setSleepMode(false);            
            // Start wake-up loading screen sequence
            // currentScreenID = ScreenPage::LOADING;
            wake_loading_active = true;
            wake_loading_deadline_ms = millis() + WAKE_LOADING_DURATION_MS;
            force_full_refresh = true; // Force full refresh after wake
            
            // Draw and display loading screen immediately to avoid white screen flash
            Paint_SelectImage(BlackImage);
            showLoadingPage(BlackImage);
            epdDisplay(DisplayMode::FULL, BlackImage);
            
            return;
        }
        else {
            // Global: long DOWN to sleep from any screen (works on MAIN, GRAPHS, SENSOR_MAP, SETTINGS, SETUP, CONNECTING, LOADING, LOGO)
            if (btn_press.button_num == ButtonNum::DOWN && btn_press.press_type == PressType::LONG) {
                debug_outln_info(F("[EPD] Going to sleep - starting sleep cycle"));
                initAndClearScreen();
                // Clear the image buffer to white before drawing sleep message
                Paint_SelectImage(BlackImage);
                Paint_Clear(WHITE);
                Paint_DrawString_EN_Center("Going to sleep...", &Font24, WHITE, BLACK);
                // Full cycle: init (FULL) -> display -> clear -> sleep.
                epdInit(DisplayMode::FULL);
                DEV_Delay_ms(200);
                epdDisplay(DisplayMode::FULL, BlackImage);
                // Also count full clear as additional update.
                DEV_Delay_ms(700);
                debug_outln_info(F("[EPD] Clear screen before sleep"));
                EPD_4IN2_V2_Clear();
                epdIncrementUpdateCount();
                DEV_Delay_ms(300);
                // Turn off LEDs during sleep
                leds_controller_insight.setSleepMode(true);
                epdSleep();
                DEV_Delay_ms(100);
                display_sleeping = true;
                return;
            }
            if (currentScreenID == ScreenPage::MAIN) {
                // Short presses: navigate screens
                if (btn_press.press_type == PressType::SHORT) {
                    if (btn_press.button_num == ButtonNum::UP) {
                        ScreenPage target = getPrevScreen(currentScreenID);
                        epdIncrementScreenCounter(target);
                        setScreen(target);
                        return;
                    } else if (btn_press.button_num == ButtonNum::SET) {
                        ScreenPage target = getNextScreen(currentScreenID);
                        epdIncrementScreenCounter(target);
                        setScreen(target);
                        return;
                    }
                }
            } 
            else if (currentScreenID == ScreenPage::GRAPHS) {
                // On graphs page:
                // - If graphs are not available: always navigate to next/prev screen
                // - If graphs are available:
                //   - SHORT UP/SET: cycle graph values (or switch screen if at last graph)
                //   - LONG  UP/SET: change screens (prev/next)
                if (!areGraphsAvailable()) {
                    // No graphs available - navigate to next/prev screen on any button press
                    if (btn_press.button_num == ButtonNum::UP) {
                        ScreenPage target = getPrevScreen(currentScreenID);
                        epdIncrementScreenCounter(target);
                        setScreen(target);
                        return;
                    } else if (btn_press.button_num == ButtonNum::SET) {
                        ScreenPage target = getNextScreen(currentScreenID);
                        epdIncrementScreenCounter(target);
                        setScreen(target);
                        return;
                    }
                } else {
                    // Graphs available - normal behavior
                    if (btn_press.press_type == PressType::LONG) {
                        if (btn_press.button_num == ButtonNum::UP) {
                            ScreenPage target = getPrevScreen(currentScreenID);
                            epdIncrementScreenCounter(target);
                            setScreen(target);
                            return;
                        } else if (btn_press.button_num == ButtonNum::SET) {
                            ScreenPage target = getNextScreen(currentScreenID);
                            epdIncrementScreenCounter(target);
                            setScreen(target);
                            return;
                        }
                    } else if (btn_press.press_type == PressType::SHORT) {
                        if (btn_press.button_num == ButtonNum::UP) {
                            // If at first graph, switch to previous screen instead of looping
                            if (setPrevGraphValue()) {
                                ScreenPage target = getPrevScreen(currentScreenID);
                                epdIncrementScreenCounter(target);
                                setScreen(target);
                                return;
                            }
                            refresh_now = true;
                            return;
                        } else if (btn_press.button_num == ButtonNum::SET) {
                            // If at last graph, switch to next screen instead of looping
                            if (setNextGraphValue()) {
                                ScreenPage target = getNextScreen(currentScreenID);
                                epdIncrementScreenCounter(target);
                                setScreen(target);
                                return;
                            }
                            refresh_now = true;
                            return;
                        }
                    }
                }
            }
            else if (currentScreenID == ScreenPage::SENSOR_MAP) {
                if (btn_press.press_type == PressType::SHORT) {
                    if (btn_press.button_num == ButtonNum::UP) {
                        ScreenPage target = getPrevScreen(currentScreenID);
                        epdIncrementScreenCounter(target);
                        setScreen(target);
                        return;
                    } else if (btn_press.button_num == ButtonNum::SET) {
                        ScreenPage target = getNextScreen(currentScreenID);
                        epdIncrementScreenCounter(target);
                        setScreen(target);
                        return;
                    }
                }
            }
            else if (currentScreenID == ScreenPage::SETTINGS) {
                if (btn_press.press_type == PressType::SHORT) {
                    if (btn_press.button_num == ButtonNum::UP) {
                        ScreenPage target = getPrevScreen(currentScreenID);
                        epdIncrementScreenCounter(target);
                        setScreen(target);
                        return;
                    } else if (btn_press.button_num == ButtonNum::SET) {
                        ScreenPage target = getNextScreen(currentScreenID);
                        epdIncrementScreenCounter(target);
                        setScreen(target);
                        return;
                    }
                }
            }
        }
    }
    // Skip all refresh logic when display is sleeping - only wake on button press
    if (display_sleeping) {
        return;
    }

    // Handle wake-up loading screen: show loading for 15-20s, then check WiFi and show MAIN or SETUP
    if (wake_loading_active) {
        if ((int32_t)(millis() - wake_loading_deadline_ms) >= 0) {
            // Loading period complete, check WiFi and transition to appropriate screen
            wake_loading_active = false;
            bool wifi_connected = (WiFi.status() == WL_CONNECTED);
            if (wifi_connected) {
                currentScreenID = ScreenPage::MAIN;
                debug_outln_info(F("[EPD] Wake complete - WiFi connected, showing MAIN screen"));
            } else {
                currentScreenID = ScreenPage::SETUP;
                debug_outln_info(F("[EPD] Wake complete - WiFi not connected, showing SETUP screen"));
            }
            refresh_now = true; // Force refresh to show the new screen
            force_full_refresh = true; // Force full refresh for the transition
        }
        // Continue showing loading screen while timer is active
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

    // Always refresh on first run (loading screen) or when explicitly requested
    bool should_refresh = (last_refresh_time == (unsigned long)-DISPLAY_REFRESH_INTERVAL) || 
                          msSince(last_refresh_time) > DISPLAY_REFRESH_INTERVAL || 
                          refresh_now || 
                          currentScreenID == ScreenPage::CONNECTING;
    
    if (should_refresh) {
        refresh_now = false;
        Paint_SelectImage(BlackImage);
        Paint_Clear(WHITE);
        
        // Show sensors map QR screen automatically every 2 hours when on main screen
        if (currentScreenID == ScreenPage::MAIN) {
            unsigned long now = millis();
            // Initialize on first check (don't show immediately on boot)
            if (last_qr_map_show_time == 0) {
                last_qr_map_show_time = now;
            }
            // Check if 2 hours have passed since last QR map display
            if (msSince(last_qr_map_show_time) >= QR_MAP_AUTO_INTERVAL) {
                last_qr_map_show_time = now;
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
                String addr = cached_urban_address; // may be empty => default link
                showSensorsMapPage(addr);
                drawScreenIndicator(ScreenPage::SENSOR_MAP);
                last_refresh_time = millis();
                showImageFast(BlackImage, ScreenPage::SENSOR_MAP);
                return;
            }
        }
        
        if (currentScreenID == ScreenPage::MAIN) {
            String jsonString;
		    serializeJson(sensors_data, jsonString);
            debug_outln_info(F("Refresh main screen"));
            drawMainScreen(BlackImage, jsonString, deviceStatus.ip_address);
        } else if (currentScreenID == ScreenPage::GRAPHS) {
            // Always draw graph screen - it will show appropriate message if no data/card
            drawGraphScreen();
            goto draw_complete;  // Skip the rest of the screen drawing logic
        }
        
        // If we changed screens (no graphs available), continue to draw the new screen
        if (currentScreenID == ScreenPage::SETUP) {
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
            // Refresh cache if Urban address present; if it appears newly, use it
            if (sensors_data.containsKey("service_data")) {
                auto service = sensors_data["service_data"].as<JsonObject>();
                if (!service.isNull() && service.containsKey("urban_robonomics_address")) {
                    String urban_addr = service["urban_robonomics_address"].as<String>();
                    if (urban_addr.length() > 0 && cached_urban_address != urban_addr) {
                        cached_urban_address = urban_addr;
                    }
                }
            }

            // If we don't yet have an Urban address, wait a few cycles before
            // falling back to the default QR (without sensor parameter).
            if (cached_urban_address.length() == 0) {
                if (!sensor_map_waiting_addr) {
                    // Start waiting phase
                    sensor_map_waiting_addr  = true;
                    sensor_map_waiting_tries = 0;
                }

                // Draw a simple "waiting for ID" screen so user understands
                Paint_Clear(WHITE);
                Paint_DrawString_EN_Center("Waiting for Urban ID...", &Font16, WHITE, BLACK);

                // Schedule next check; after a few tries, give up and show default QR
                sensor_map_waiting_tries++;
                if (sensor_map_waiting_tries < 3) {
                    next_sensor_map_check_ms = millis() + 3000; // wait ~3s between checks
                } else {
                    sensor_map_waiting_addr  = false;
                    sensor_map_waiting_tries = 0;
                    String addr; // empty => default link in showSensorsMapPage
                    showSensorsMapPage(addr);
                }
            } else {
                // We have a valid Urban address: show final QR immediately
                sensor_map_waiting_addr  = false;
                sensor_map_waiting_tries = 0;
                String addr = cached_urban_address;
                showSensorsMapPage(addr);
            }
        } else if (currentScreenID == ScreenPage::SETTINGS) {
            // Get urban IP address from config or sensors_data
            String urban_ip = String(cfg::chosen_altruist_urban);
            // Fallback: try to get from sensors_data if config is empty
            if (urban_ip.length() == 0 && sensors_data.containsKey("service_data")) {
                auto service = sensors_data["service_data"].as<JsonObject>();
                if (!service.isNull() && service.containsKey("altruist_addresses")) {
                    JsonArray addresses = service["altruist_addresses"];
                    if (addresses.size() > 0) {
                        urban_ip = addresses[0].as<String>();
                    }
                }
            }
            showSettingsPage(BlackImage, deviceStatus, urban_ip, robonomics_address);
        }
        
draw_complete:
        // Draw screen indicator dots in bottom right corner
        drawScreenIndicator(currentScreenID);
        
        last_refresh_time = millis();
        // After wake use FULL mode for first update
        if (force_full_refresh) {
            force_full_refresh = false; // Reset flag after use
            epdResetPeriodPosition(); // Reset period counter after full refresh
            debug_outln_info(F("[EPD] FULL refresh after wake"));
            epdDisplay(DisplayMode::FULL, BlackImage);
        } else {
            // Pass current screen to showImageFast for adaptive update logic:
            // MAIN screen: 10 partial + 1 full
            // Other pages: 5 partial + 1 full
            showImageFast(BlackImage, currentScreenID);
        }
    }
}

#endif // ALTRUIST_INSIDE
