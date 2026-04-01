#ifdef ALTRUIST_URBAN

#include "leds_controller_urban.h"
#include "../defines.h"
#include "../utils.h"
#include "../config_manager/config_helpers.h"

#define LED_NUM_DATA_SENDING 0
#define LED_NUM_CONNECTION 1

LedControllerUrban::LedControllerUrban():
    pixels(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800) {}

void LedControllerUrban::init() {
    if (LED_PIN != -1 && cfg::leds_on) {
        pixels.begin();
        pixels.clear();
        uint8_t brightness;
        if (cfg::leds_brightness * 255 / 100 < 0) {
            brightness = 0;
        } else if (cfg::leds_brightness * 255 / 100 > 255) {
            brightness = 255;
        } else {
            brightness = cfg::leds_brightness * 255 / 100;
        }
        pixels.setBrightness(brightness);
        pixels.show();
        debug_outln_info(F("Setup leds on pin "), LED_PIN);
    } else {
        debug_outln_info(F("Will not setup leds on pin "), LED_PIN);
    }
}

void LedControllerUrban::setMode(LedMode mode) {
    mode_changed = true;
    current_mode = mode;
}

void LedControllerUrban::process() {
    if (LED_PIN == -1 || !cfg::leds_on) {
        return;
    }
    if (mode_changed) {
        mode_changed = false;
        switch (current_mode) {
            case LedMode::NONE:
                pixels.clear();
                connection_color_set = false;
                break;
            case LedMode::BLUE:
                connection_color = pixels.Color(0, 0, 255);
                connection_color_set = true;
                pixels.setPixelColor(LED_NUM_CONNECTION, connection_color);
                break;
            case LedMode::GREEN:
                connection_color = pixels.Color(0, 255, 0);
                connection_color_set = true;
                pixels.setPixelColor(LED_NUM_CONNECTION, connection_color);
                break;
            case LedMode::PROVISIONING:
                // Purple: AP/provisioning mode
                connection_color = pixels.Color(160, 0, 255);
                connection_color_set = true;
                pixels.setPixelColor(LED_NUM_CONNECTION, connection_color);
                break;
            case LedMode::RESETTING:
                // Yellow: reset in progress (both LEDs)
                connection_color = pixels.Color(255, 200, 0);
                connection_color_set = true;
                pixels.setPixelColor(LED_NUM_CONNECTION, connection_color);
                pixels.setPixelColor(LED_NUM_DATA_SENDING, connection_color);
                break;
            case LedMode::BLINK_RED:
                // Keep connection LED as-is; blink only the data LED.
                if (connection_color_set) {
                    pixels.setPixelColor(LED_NUM_CONNECTION, connection_color);
                }
                for (int blink_count = 0; blink_count < MAX_BLINK_COUNT; blink_count++) {
                    pixels.setPixelColor(LED_NUM_DATA_SENDING, pixels.Color(255, 0, 0));
                    pixels.show();
                    delay(150);
                    pixels.setPixelColor(LED_NUM_DATA_SENDING, pixels.Color(0, 0, 0));
                    pixels.show();
                    delay(150);
                }
                break;
            case LedMode::BLINK_GREEN:
                // Keep connection LED as-is; blink only the data LED.
                if (connection_color_set) {
                    pixels.setPixelColor(LED_NUM_CONNECTION, connection_color);
                }
                for (int blink_count = 0; blink_count < MAX_BLINK_COUNT; blink_count++) {
                    pixels.setPixelColor(LED_NUM_DATA_SENDING, pixels.Color(0, 255, 0));
                    pixels.show();
                    delay(150);
                    pixels.setPixelColor(LED_NUM_DATA_SENDING, pixels.Color(0, 0, 0));
                    pixels.show();
                    delay(150);
                }
                break;
        }
        pixels.show();
    }
}

void LedControllerUrban::_setAllPixels(uint32_t color) {
    for (int pixel = 0; pixel < LED_COUNT; pixel++) {
        pixels.setPixelColor(pixel, color);
    }
}

#endif