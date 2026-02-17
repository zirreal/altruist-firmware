#ifdef ALTRUIST_INSIDE

#include "display_manager.h"
#include "screens/screens.h"
#include "screens/graph.h"
#include "screens/display_common.h"
#include "../defines.h"
#include "utils.h"
#include "../intl.h"
#include "paint_driver/fonts/fonts.h"
#include <SPIFFS.h>
#include <WiFi.h>
#include "../leds/leds_controller_insight.h"
#include "../config_manager/config_helpers.h"
#ifdef DISPLAY_4IN2
#include "driver/EPD_4in2_SSD1683.h"
#endif

extern LedControllerInsight leds_controller_insight;

// Sensor map Urban ID waiting policy:
// If Urban ID is not yet known, we will perform up to 3 checks,
// each spaced 5 minutes apart, before falling back to the default QR.
static const uint8_t SENSOR_MAP_MAX_WAIT_TRIES      = 3;
static const unsigned long SENSOR_MAP_WAIT_INTERVAL = 5UL * 60UL * 1000UL; // 5 minutes

bool display_sleeping = false;

// For MAIN screen we want the refresh to be aligned with the real time
// minute boundary (xx:00) instead of a fixed "every N ms since boot".
// This keeps the clock in the header changing exactly when the minute
// changes, instead of "late" in the middle of a minute.
static unsigned long next_main_refresh_ms = 0;

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
                Paint_DrawString_Display_Center(INTL_DISP_GOING_TO_SLEEP, &Font24, &font_24_cyrillic, &font_24_ascii, WHITE, BLACK);
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

    // Periodic EPD re-initialization watchdog:
    // This helps recover from stuck display states after many partial updates.
    if (currentScreenID == ScreenPage::MAIN) {
        unsigned long now_ms = millis();
        if (last_epd_reinit_time_ms == 0) {
            // Initialize baseline on first run after boot
            last_epd_reinit_time_ms = now_ms;
        } else if (msSince(last_epd_reinit_time_ms) >= EPD_REINIT_INTERVAL_MS) {
            debug_outln_verbose(F("[EPD] Periodic watchdog: recovering display and scheduling FULL refresh"));
            last_epd_reinit_time_ms = now_ms;
            // Use recovery function to reset and re-init the controller
            epdRecoverFromStuck();
            // Force a full refresh on the next draw of MAIN
            force_full_refresh = true;
            refresh_now = true;
        }
    }

    // Handle wake-up loading screen: show loading for 15-20s, then check WiFi and show MAIN or SETUP
    if (wake_loading_active) {
        if ((int32_t)(millis() - wake_loading_deadline_ms) >= 0) {
            // Loading period complete, check WiFi and transition to appropriate screen
            wake_loading_active = false;
            bool wifi_connected = (WiFi.status() == WL_CONNECTED);
            if (wifi_connected) {
                currentScreenID = ScreenPage::MAIN;
                debug_outln_verbose(F("[EPD] Wake complete - WiFi connected, showing MAIN screen"));
            } else {
                currentScreenID = ScreenPage::SETUP;
                debug_outln_verbose(F("[EPD] Wake complete - WiFi not connected, showing SETUP screen"));
            }
            refresh_now = true; // Force refresh to show the new screen
            force_full_refresh = true; // Force full refresh for the transition
        }
        // Continue showing loading screen while timer is active
    }

    // Update cached Urban address every cycle; if it changes and we're on SENSOR_MAP, trigger redraw
    // Acquire mutex to safely read sensors_data (it may be modified by sensor task)
    bool need_spiffs_save = false;
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(100))) {
        if (sensors_data.containsKey("service_data")) {
            auto service = sensors_data["service_data"].as<JsonObject>();
            if (!service.isNull() && service.containsKey("urban_robonomics_address")) {
                String urban_addr = service["urban_robonomics_address"].as<String>();
                if (urban_addr.length() > 0 && cached_urban_address != urban_addr) {
                    bool was_empty = cached_urban_address.length() == 0;
                    cached_urban_address = urban_addr;
                    need_spiffs_save = true;
                    if (currentScreenID == ScreenPage::SENSOR_MAP || was_empty) {
                        refresh_now = true; // force first render after boot/flash
                    }
                }
            }
        }
        xSemaphoreGive(mutex);
    }
    // Persist to SPIFFS outside mutex - SPIFFS writes are slow
    if (need_spiffs_save) {
        if (SPIFFS.begin(true)) {
            File f = SPIFFS.open("/urban_ss58.cache", "w");
            if (f) { f.print(cached_urban_address); f.close(); }
        }
    }

    if (currentScreenID == ScreenPage::SENSOR_MAP && sensor_map_waiting_addr && (int32_t)(millis() - next_sensor_map_check_ms) >= 0) {
        refresh_now = true;
    }

    // Always refresh on first run (loading screen) or when explicitly requested.
    //
    // On MAIN screen, instead of a fixed "every DISPLAY_REFRESH_INTERVAL ms"
    // cadence (which can drift far away from the actual minute boundary),
    // we try to align the refresh with the *real* local time, so that the
    // minute in the header changes right when the clock minute changes.
    // OTA screen: 15 sec interval for progress updates; partial/full handled in draw_complete.
    bool time_based_refresh = false;
    static bool prev_ota_in_progress = false;
    if (deviceStatus.ota_in_progress || deviceStatus.ota_failed) {
        // Reset timer when transitioning from progress to failed so error screen shows immediately
        if (prev_ota_in_progress && !deviceStatus.ota_in_progress) {
            last_ota_display_ms = 0;
        }
        bool ota_first_show = (last_ota_display_ms == 0);
        bool ota_interval_passed = (last_ota_display_ms > 0) && (msSince(last_ota_display_ms) > OTA_DISPLAY_REFRESH_INTERVAL);
        time_based_refresh = ota_first_show || ota_interval_passed;
    } else {
        last_ota_display_ms = 0;  // reset so next OTA session shows immediately
        if (currentScreenID == ScreenPage::MAIN) {
            if (next_main_refresh_ms == 0) {
                time_based_refresh = msSince(last_refresh_time) > DISPLAY_REFRESH_INTERVAL;
            } else {
                time_based_refresh = (int32_t)(millis() - next_main_refresh_ms) >= 0;
            }
        } else {
            time_based_refresh = msSince(last_refresh_time) > DISPLAY_REFRESH_INTERVAL;
            next_main_refresh_ms = 0;
        }
    }
    prev_ota_in_progress = deviceStatus.ota_in_progress;

    bool should_refresh = (last_refresh_time == (unsigned long)-DISPLAY_REFRESH_INTERVAL) || 
                          time_based_refresh || 
                          refresh_now || 
                          currentScreenID == ScreenPage::CONNECTING;
    
    if (should_refresh) {
        debug_outln_verbose(F("[Display] Starting refresh cycle for screen "), String((int)currentScreenID));
        refresh_now = false;
        Paint_SelectImage(BlackImage);
        Paint_Clear(WHITE);
        
        if (deviceStatus.ota_failed) {
            showOTAFailedPage(BlackImage);
            last_ota_display_ms = millis();
            goto draw_complete;
        }
        
        if (deviceStatus.ota_in_progress) {
            showOTAUpdatePage(BlackImage, deviceStatus);
            last_ota_display_ms = millis();
            goto draw_complete;
        }
        
        if (currentScreenID == ScreenPage::MAIN) {
            // Acquire mutex while serializing sensors_data to prevent race conditions
            // Use longer timeout since display refresh is infrequent (every 60s)
            if (xSemaphoreTake(mutex, pdMS_TO_TICKS(500))) {
                cached_json_string = "";
                serializeJson(sensors_data, cached_json_string);
                xSemaphoreGive(mutex);
            }
            // If mutex failed, cached_json_string still has previous data - display stays consistent
            debug_outln_verbose(F("[Display] Refresh MAIN screen"));
            drawMainScreen(BlackImage, cached_json_string, deviceStatus.ip_address, robonomics_address, cached_urban_address);
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
            debug_outln_verbose(F("Showing connecting screen"));
            showConnectingPage(BlackImage, 25);
        } else if (currentScreenID == ScreenPage::LOGO) {
            showLogoPage();
        } else if (currentScreenID == ScreenPage::SENSOR_MAP) {
            // Refresh cache if Urban address present; if it appears newly, use it (with mutex)
            if (xSemaphoreTake(mutex, pdMS_TO_TICKS(100))) {
                if (sensors_data.containsKey("service_data")) {
                    auto service = sensors_data["service_data"].as<JsonObject>();
                    if (!service.isNull() && service.containsKey("urban_robonomics_address")) {
                        String urban_addr = service["urban_robonomics_address"].as<String>();
                        if (urban_addr.length() > 0 && cached_urban_address != urban_addr) {
                            cached_urban_address = urban_addr;
                        }
                    }
                }
                xSemaphoreGive(mutex);
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
                Paint_DrawString_Display_Center(INTL_DISP_WAITING_URBAN_ID, &Font16, &font_16_cyrillic, &font_16_ascii, WHITE, BLACK);

                // Schedule next check; after a limited number of tries, give up and show default QR.
                sensor_map_waiting_tries++;
                if (sensor_map_waiting_tries < SENSOR_MAP_MAX_WAIT_TRIES) {
                    // Wait 5 minutes between checks to give Urban time to appear / boot.
                    next_sensor_map_check_ms = millis() + SENSOR_MAP_WAIT_INTERVAL;
                } else {
                    // We've checked multiple times and still have no Urban ID:
                    // fall back to default QR (without specific sensor parameter).
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
            // Fallback: try to get from sensors_data if config is empty (with mutex)
            if (urban_ip.length() == 0) {
                if (xSemaphoreTake(mutex, pdMS_TO_TICKS(100))) {
                    if (sensors_data.containsKey("service_data")) {
                        auto service = sensors_data["service_data"].as<JsonObject>();
                        if (!service.isNull() && service.containsKey("altruist_addresses")) {
                            JsonArray addresses = service["altruist_addresses"];
                            if (addresses.size() > 0) {
                                urban_ip = addresses[0].as<String>();
                            }
                        }
                    }
                    xSemaphoreGive(mutex);
                }
            }
            showSettingsPage(BlackImage, deviceStatus, urban_ip, robonomics_address);
        }
        
draw_complete:
        // Draw screen indicator (skip for OTA/failure - full-screen message)
        if (!deviceStatus.ota_in_progress && !deviceStatus.ota_failed) {
            drawScreenIndicator(currentScreenID);
        }
        
        last_refresh_time = millis();


        if (deviceStatus.ota_in_progress || deviceStatus.ota_failed) {
            ota_display_refresh_count++;
            bool ota_do_full = (ota_display_refresh_count % OTA_FULL_REFRESH_EVERY_N == 0);
            epdDisplay(ota_do_full ? DisplayMode::FULL : DisplayMode::PARTIAL, BlackImage);
        } else {
            ota_display_refresh_count = 0;  // reset for next OTA session
        }

        // For MAIN screen schedule the next refresh close to the next
        // local minute boundary so that the displayed time is always
        // up-to-date right after the minute changes.
        if (currentScreenID == ScreenPage::MAIN) {
            struct tm timeinfo;
            if (getLocalTime(&timeinfo)) {
                int sec = timeinfo.tm_sec;
                // How many seconds left until the next minute tick.
                int seconds_to_next_minute = (sec == 0) ? 60 : (60 - sec);
                unsigned long now_ms = millis();
                unsigned long ms_to_next_minute = (unsigned long)seconds_to_next_minute * 1000UL;
                // Small safety margin so that getLocalTime() used in drawMainScreen
                // already reports the new minute when we refresh.
                const unsigned long safety_margin_ms = 150;
                if (ms_to_next_minute > safety_margin_ms) {
                    ms_to_next_minute -= safety_margin_ms;
                }
                next_main_refresh_ms = now_ms + ms_to_next_minute;
            } else {
                // If we cannot get time yet, keep simple behaviour.
                next_main_refresh_ms = last_refresh_time + DISPLAY_REFRESH_INTERVAL;
            }
        }
        // After wake use FULL mode for first update
        if (force_full_refresh) {
            force_full_refresh = false; // Reset flag after use
            epdResetPeriodPosition(); // Reset period counter after full refresh
            debug_outln_verbose(F("[EPD] FULL refresh (watchdog/wake/main) pushed to panel"));
            epdDisplay(DisplayMode::FULL, BlackImage);
        } else if (!deviceStatus.ota_in_progress && !deviceStatus.ota_failed) {
            // Pass current screen to showImageFast for adaptive update logic:
            // MAIN screen: 10 partial + 1 full
            // Other pages: 5 partial + 1 full
            debug_outln_verbose(F("[EPD] FAST/partial refresh pushed for screen "), String((int)currentScreenID));
            showImageFast(BlackImage, currentScreenID);
        }
    }
}

#endif // ALTRUIST_INSIDE
