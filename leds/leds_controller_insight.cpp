#ifdef ALTRUIST_INSIDE

#include "leds_controller_insight.h"
#include <string.h>
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
    if (sleep_mode) {
        pixels.clear();
        pixels.show();
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
            
            // Handle noise - prefer max if available, otherwise use avg
            if (sensors_data[ATRUIST_URBAN_SENSOR].containsKey("PCBA_noiseMax")) {
                color = _getNoiseColor(sensors_data[ATRUIST_URBAN_SENSOR]["PCBA_noiseMax"]["value"].as<float>());
                _setPartColor(4, 6, color);
                debug_outln_info(F("Set Noise color "), getColorName(color));
            } else if (sensors_data[ATRUIST_URBAN_SENSOR].containsKey("PCBA_noiseAvg")) {
                color = _getNoiseColor(sensors_data[ATRUIST_URBAN_SENSOR]["PCBA_noiseAvg"]["value"].as<float>());
                _setPartColor(4, 6, color);
                debug_outln_info(F("Set Noise color "), getColorName(color));
            }
            if (sensors_data[ATRUIST_URBAN_SENSOR].containsKey("BME280_temperature")) {
                color = _getTempColor(sensors_data[ATRUIST_URBAN_SENSOR]["BME280_temperature"]["value"].as<float>());
                _setPartColor(7, 9, color);
                debug_outln_info(F("Set U Temp color "), getColorName(color));
            }
            if (sensors_data[ATRUIST_URBAN_SENSOR].containsKey("BME280_humidity")) {
                color = _getHumidityColor(sensors_data[ATRUIST_URBAN_SENSOR]["BME280_humidity"]["value"].as<float>());
                _setPartColor(10, 12, color);
                debug_outln_info(F("Set U Humidity color "), getColorName(color));
            }
            if (sensors_data[ATRUIST_URBAN_SENSOR].containsKey("BME280_pressure")) {
                color = _getPressureColor(sensors_data[ATRUIST_URBAN_SENSOR]["BME280_pressure"]["value"].as<float>() * 0.0075);
                _setPartColor(13, 16, color);
                debug_outln_info(F("Set U Pressure color "), getColorName(color));
            }
        }
        if (sensors_data.containsKey("BME680")) {
            color = _getTempColor(sensors_data["BME680"]["temperature"]["value"].as<float>());
            _setPartColor(17, 19, color);
            debug_outln_info(F("Set Temp color "), getColorName(color));
            color = _getHumidityColor(sensors_data["BME680"]["humidity"]["value"].as<float>());
            _setPartColor(20, 22, color);
            debug_outln_info(F("Set Humidity color "), getColorName(color));
            color = _getPressureColor(sensors_data["BME680"]["pressure"]["value"].as<float>() * 0.0075);
            _setPartColor(23, 25, color);
            debug_outln_info(F("Set Pressure color "), getColorName(color));
        }
        
        if (sensors_data.containsKey("SCD4x")) {
            color = _getCO2Color(sensors_data["SCD4x"]["co2"]["value"].as<float>());
            _setPartColor(26, 28, color);
            debug_outln_info(F("Set CO2 color "), getColorName(color));
        }
        pixels.show();
    }
}

void LedControllerInsight::setSleepMode(bool enabled) {
    sleep_mode = enabled;
    if (LED_PIN == -1 || !cfg::leds_on) {
        return;
    }
    if (sleep_mode) {
        pixels.clear();
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

// Sensor threshold configurations - definitions (declared in header)
namespace SensorConfigs {
    // Noise thresholds (dB)
    const float noise_thresholds[] = {50, 70, 85, 100};
    const ColorName noise_colors[] = {ColorName::GREEN_LED, ColorName::BLUE_LED, ColorName::YELLOW_LED, ColorName::ORANGE_LED, ColorName::RED_LED};
    
    // CO2 thresholds (ppm)
    const float co2_thresholds[] = {1000, 2000, 5000};
    const ColorName co2_colors[] = {ColorName::GREEN_LED, ColorName::YELLOW_LED, ColorName::ORANGE_LED, ColorName::RED_LED};
    
    // Temperature thresholds (°C) 
    const float temp_thresholds[] = {1, 10, 27, 35};
    const ColorName temp_colors[] = {ColorName::DARKBLUE_LED, ColorName::BLUE_LED, ColorName::GREEN_LED, ColorName::YELLOW_LED, ColorName::ORANGE_LED};
    
    // PM thresholds 
    // PM10 thresholds (μg/m³)
    const float pm10_thresholds[] = {50, 100, 250, 350};
    // PM2.5 thresholds (μg/m³)
    const float pm25_thresholds[] = {30, 55, 110, 250};
    const ColorName pm_colors[] = {ColorName::GREEN_LED, ColorName::BLUE_LED, ColorName::YELLOW_LED, ColorName::ORANGE_LED, ColorName::RED_LED};
    
    // Humidity thresholds (%)
    const float humidity_thresholds[] = {30, 40, 60, 70};
    const ColorName humidity_colors[] = {ColorName::ORANGE_LED, ColorName::YELLOW_LED, ColorName::GREEN_LED, ColorName::BLUE_LED, ColorName::DARKBLUE_LED};
    
    // Pressure thresholds (mmHg)
    const float pressure_thresholds[] = {747, 768, 775};
    const ColorName pressure_colors[] = {ColorName::BLUE_LED, ColorName::GREEN_LED, ColorName::YELLOW_LED, ColorName::ORANGE_LED};
}

// Generic function to get color based on value and thresholds
// thresholds array should be in ascending order, colors array should have count elements
// (one color for each threshold range, and the last color is used for values above all thresholds)
uint32_t LedControllerInsight::_getColorByThresholds(float value, const float* thresholds, const ColorName* colors, uint8_t count) {
    for (uint8_t i = 0; i < count; i++) {
        if (value < thresholds[i]) {
            return getColor(colors[i]);
        }
    }
    // Value is above all thresholds, return the last color from the array
    return getColor(colors[count - 1]);
}

// PM color function
uint32_t LedControllerInsight::_getPMColor(float pm10, float pm25) {
    // PM color requires checking both PM10 and PM2.5 values simultaneously
    // Check if either value exceeds the last threshold
    const uint8_t threshold_count = 5;
    
    // If either PM10 >= 350 OR PM25 >= 250, return RED (last color)
    if (pm10 >= SensorConfigs::pm10_thresholds[threshold_count - 1] || 
        pm25 >= SensorConfigs::pm25_thresholds[threshold_count - 1]) {
        return getColor(SensorConfigs::pm_colors[threshold_count]); 
    }
    
    // Otherwise, check both values against thresholds
    for (uint8_t i = 0; i < threshold_count; i++) {
        if (pm10 < SensorConfigs::pm10_thresholds[i] && pm25 < SensorConfigs::pm25_thresholds[i]) {
            return getColor(SensorConfigs::pm_colors[i]);
        }
    }
    return getColor(SensorConfigs::pm_colors[threshold_count]);
}

uint32_t LedControllerInsight::_getNoiseColor(float noise) {
    return _getColorByThresholds(noise, SensorConfigs::noise_thresholds, SensorConfigs::noise_colors, 4);
}

uint32_t LedControllerInsight::_getCO2Color(float co2) {
    return _getColorByThresholds(co2, SensorConfigs::co2_thresholds, SensorConfigs::co2_colors, 3);
}

uint32_t LedControllerInsight::_getTempColor(float temperature) {
    // Use SensorConfigs thresholds and colors directly
    return _getColorByThresholds(temperature, SensorConfigs::temp_thresholds, SensorConfigs::temp_colors, 4);
}

uint32_t LedControllerInsight::_getHumidityColor(float humidity) {
    return _getColorByThresholds(humidity, SensorConfigs::humidity_thresholds, SensorConfigs::humidity_colors, 4);
}

uint32_t LedControllerInsight::_getPressureColor(float pressure) {
    return _getColorByThresholds(pressure, SensorConfigs::pressure_thresholds, SensorConfigs::pressure_colors, 4);
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