#ifdef ALTRUIST_INSIDE

#include "leds_controller_insight.h"
#include "../utils.h"
#include "../config_manager/config_helpers.h"


void LedControllerInsight::init() {
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
    last_refresh_time = millis() - REFRESH_INTERVAL; 
}

void LedControllerInsight::process() {
    if (LED_PIN == -1 || !cfg::leds_on) {
        return;
    }
    
    // Check if it's night time - if so, turn off LEDs completely
    if (_isNightTime()) {
        pixels.clear();
        pixels.show();
        return;
    }
    
    uint32_t color;
    if (msSince(last_refresh_time) > REFRESH_INTERVAL) {
        last_refresh_time = millis();
        
        // Calculate time-based brightness and apply it
        current_time_brightness = _calculateTimeBrightness();
        uint8_t final_brightness = (cfg::leds_brightness * current_time_brightness) / 100;
        if (final_brightness > 255) final_brightness = 255;
        pixels.setBrightness(final_brightness);
        
        debug_outln_info(F("LED brightness set to: "), final_brightness);
        
        _setAllPixels(pixels.Color(255, 255, 255));
        if (sensors_data.containsKey(ATRUIST_URBAN_SENSOR)) {
            if (sensors_data[ATRUIST_URBAN_SENSOR].containsKey("SDS_P1") && sensors_data[ATRUIST_URBAN_SENSOR].containsKey("SDS_P2")) {
                color = _getPMColor(sensors_data[ATRUIST_URBAN_SENSOR]["SDS_P1"]["value"].as<float>(), sensors_data[ATRUIST_URBAN_SENSOR]["SDS_P2"]["value"].as<float>());
                _setPartColor(1, 3, color);
                debug_outln_info(F("Set PM color "), getColorName(color));
            }
            if (sensors_data[ATRUIST_URBAN_SENSOR].containsKey("PCBA_noiseAvg")) {
                color = _getNoiseColor(sensors_data[ATRUIST_URBAN_SENSOR]["PCBA_noiseAvg"]["value"].as<float>());
                _setPartColor(4, 6, color);
                debug_outln_info(F("Set Noise color "), getColorName(color));
            }
            if (sensors_data[ATRUIST_URBAN_SENSOR].containsKey("BME280_temperature")) {
                color = _getTempColor(sensors_data[ATRUIST_URBAN_SENSOR]["BME280_temperature"]["value"].as<float>());
                _setPartColor(10, 12, color);
                debug_outln_info(F("Set Temp color "), getColorName(color));
            }
        }
        if (sensors_data.containsKey("SCD4x")) {
            color = _getCO2Color(sensors_data["SCD4x"]["co2"]["value"].as<float>());
            _setPartColor(7, 9, color);
            debug_outln_info(F("Set CO2 color "), getColorName(color));
        }
        pixels.show();
    }
}

void LedControllerInsight::_setAllPixels(uint32_t color) {
    for (int pixel = 0; pixel < LED_COUNT; pixel++) {
        pixels.setPixelColor(pixel, color);
    }
}

void LedControllerInsight::_setPartColor(uint8_t start_led, uint8_t end_led, uint32_t color) {
    for (int pixel = start_led - 1; pixel < end_led; pixel++) {
        pixels.setPixelColor(pixel, color);
    }
}

uint32_t LedControllerInsight::_getPMColor(float pm10, float pm25) {
    if (pm10 < 50 && pm25 < 36) {
        return getColor(ColorName::GREEN_LED);
    } else if (pm10 < 100 && pm25 < 70) {
        return getColor(ColorName::BLUE_LED);
    } else if (pm10 < 250 && pm25 < 150) {
        return getColor(ColorName::ORANGE_LED);
    } else if (pm10 < 350 && pm25 < 250) {
        return getColor(ColorName::RED_LED);
    } else {
        return getColor(ColorName::PURPLE_LED);
    }
}

uint32_t LedControllerInsight::_getNoiseColor(float noise) {
    if (noise < 60) {
        return getColor(ColorName::GREEN_LED);
    } else if (noise < 80) {
        return getColor(ColorName::BLUE_LED);
    } else if (noise < 100) {
        return getColor(ColorName::ORANGE_LED);
    } else if (noise < 120) {
        return getColor(ColorName::RED_LED);
    } else {
        return getColor(ColorName::PURPLE_LED);
    }
}

uint32_t LedControllerInsight::_getCO2Color(float co2) {
    if (co2 < 800) {
        return getColor(ColorName::GREEN_LED);
    } else if (co2 < 1000) {
        return getColor(ColorName::BLUE_LED);
    } else if (co2 < 2500) {
        return getColor(ColorName::ORANGE_LED);
    } else if (co2 < 5000) {
        return getColor(ColorName::RED_LED);
    } else {
        return getColor(ColorName::PURPLE_LED);
    }
}

uint32_t LedControllerInsight::_getTempColor(float temperature) {
    if (temperature < -9) {
        return getColor(ColorName::PURPLE_LED);
    } else if (temperature < 10) {
        return getColor(ColorName::BLUE_LED);
    } else if (temperature < 25) {
        return getColor(ColorName::GREEN_LED);
    } else {
        return getColor(ColorName::ORANGE_LED);
    }
}

// Calculate brightness based on time of day
uint8_t LedControllerInsight::_calculateTimeBrightness() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        // If we can't get time, use full brightness
        return 255;
    }
    
    int hour = timeinfo.tm_hour;
    int minute = timeinfo.tm_min;
    
    // Convert to minutes since midnight for easier calculation
    int minutes_since_midnight = hour * 60 + minute;
    
    // Time schedule:
    // 06:00 - 22:00 (360 - 1320): Full brightness (100%)
    // 22:00 - 23:00 (1320 - 1380): Gradual dim from 100% to 20%
    // 23:00 - 24:00 (1380 - 1440): Gradual dim from 20% to 5%
    // 00:00 - 06:00 (0 - 360): Off (handled by _isNightTime)
    
    if (minutes_since_midnight >= 360 && minutes_since_midnight < 1320) {
        // Daytime: Full brightness
        return 255;
    } else if (minutes_since_midnight >= 1320 && minutes_since_midnight < 1380) {
        // 22:00 - 23:00: Gradual dimming from 100% to 20%
        int minutes_into_dimming = minutes_since_midnight - 1320;
        float brightness_percent = 100.0 - (80.0 * minutes_into_dimming / 60.0); // 100% to 20%
        return (uint8_t)(255 * brightness_percent / 100.0);
    } else if (minutes_since_midnight >= 1380 && minutes_since_midnight < 1440) {
        // 23:00 - 24:00: Further dimming from 20% to 5%
        int minutes_into_late_dimming = minutes_since_midnight - 1380;
        float brightness_percent = 20.0 - (15.0 * minutes_into_late_dimming / 60.0); // 20% to 5%
        return (uint8_t)(255 * brightness_percent / 100.0);
    }
    
    // Default to full brightness if something goes wrong
    return 255;
}

// Check if it's night time (complete LED turn-off period)
bool LedControllerInsight::_isNightTime() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        // If we can't get time, don't turn off LEDs for safety
        return false;
    }
    
    int hour = timeinfo.tm_hour;
    
    // Night time: 00:00 - 06:00 (complete turn-off)
    return (hour >= 0 && hour < 6);
}

#endif