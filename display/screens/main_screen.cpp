#ifdef ALTRUIST_INSIDE

#include "main_screen.h"
#include <ArduinoJson.h>
#include <string.h>
#include <stdio.h>
#include "../driver/DEV_Config.h"
#include "../driver/EPD.h"
// #include "graph.h"
#include <stdlib.h>
#include "utils.h"
#include "../icons/icons/icons_20x20.h"
#include "../../defines.h"
#include "../utils.h"
#include "../../config_manager/config_helpers.h"
#include "display_common.h"


// Data source indicators
enum DataSource {
    SOURCE_URBAN = 0,
    SOURCE_INSIGHT = 1
};

// Draw a value with icon, label, units and data source indicator
void drawValue(const char *label, float value, uint8_t precision,
               const unsigned char *image, const char *units,
               uint16_t image_size, uint16_t x_start, uint16_t y_start,
               uint16_t image_offset = 0, bool highlight = false, DataSource source = SOURCE_INSIGHT, bool is_dangerous = false) {
    
    // Draw background highlight for important values
    if (highlight) {
        Paint_DrawRectangle(x_start - 2, y_start - 2, 
                          x_start + 120, y_start + 45, 
                          BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    }
    
    Paint_DrawImage(image, x_start, y_start, image_size, image_size);
    
    // Use smaller font for labels to reduce visual clutter
    Paint_DrawString_EN(x_start + image_size + image_offset + 3, y_start, label, &Font12, WHITE, BLACK);

    if (value < 0) {
        // Clean "no data" display
        Paint_DrawString_EN(x_start + image_size + image_offset + 3,
                            y_start + Font12.Height + 3,
                            "--", &Font20, WHITE, BLACK);
    } else {
        char value_str[12];
        stringFromFloat(value_str, value, precision);
        
        sFONT* value_font = &Font16;
        
        // Draw the numeric value
        Paint_DrawString_EN(x_start + image_size + image_offset + 3,
                            y_start + Font12.Height + 3,
                            value_str, value_font, WHITE, BLACK);
        
        // Position units based on rendered value width and chosen font
        uint16_t value_pixel_width = strlen(value_str) * value_font->Width;
        uint16_t units_x = x_start + image_size + image_offset + value_pixel_width + 6;
        Paint_DrawString_EN(units_x,
                            y_start + Font12.Height + 6,
                            units, &Font12, WHITE, BLACK);
        
        // Draw warning icon (circle with exclamation point) if value is dangerous
        if (is_dangerous) {
            String debug_msg = String(F("Drawing ! for dangerous value: ")) + String(label) + F(" = ") + String(value_str);
            debug_outln_info(debug_msg);
            uint16_t units_width = strlen(units) * Font12.Width;
            
            // Calculate position for warning icon (circle with exclamation point)
            const uint16_t circle_radius = 5; 
            uint16_t icon_x = units_x + units_width + 6;  // Position after units
            uint16_t icon_y = y_start + Font12.Height + 5; 
            uint16_t circle_center_x = icon_x + circle_radius;
            uint16_t circle_center_y = icon_y + circle_radius;
            
            // Draw filled black circle
            Paint_DrawCircle(circle_center_x, circle_center_y, circle_radius, 
                           BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
            
            // Draw white exclamation point inside the circle
            uint16_t exclamation_x = circle_center_x - (Font8.Width / 2) - 1;
            uint16_t exclamation_y = circle_center_y - (Font8.Height / 2) + 1; 
            
            Paint_DrawChar(exclamation_x, exclamation_y, '!', &Font8, WHITE, WHITE);
        }
    }
}

// Validate sensor data ranges for quality indication
bool isValidRange(float value, float min_val, float max_val) {
    return (value >= min_val && value <= max_val);
}

// Check if PM values are dangerous 
// Based on LED controller thresholds: GREEN=OK, BLUE=moderate, ORANGE/RED/YELLOW=dangerous
bool isPMDangerous(float pm10, float pm25) {
    if (pm10 < 0 || pm25 < 0) return false; // Invalid data
    // Dangerous if: pm10 >= 100 OR pm25 >= 70 
    bool dangerous = (pm10 >= 100 || pm25 >= 55);
    return dangerous;
}

// Check if noise is dangerous 
// Based on LED controller: GREEN<50, BLUE<70, ORANGE/RED/YELLOW>=70
bool isNoiseDangerous(float noise) {
    if (noise < 0) return false; // Invalid data
    // Dangerous if: noise >= 80 (beyond moderate/blue threshold)
    return (noise >= 70);
}

// Check if CO2 is dangerous (not OK or moderate)
// Based on LED controller
bool isCO2Dangerous(float co2) {
    if (co2 < 0) return false; // Invalid data
    // Dangerous if: co2 >= 1000 
    return (co2 >= 1000);
}

// Check if temperature is dangerous 
// Based on LED controller
bool isTempDangerous(float temperature) {
    if (temperature < -40 || temperature > 80) return false; // Invalid data
    // Dangerous if: temp < 10 OR temp >= 25 (outside green range)
    return (temperature < 10 || temperature >= 27);
}

// Check if humidity is dangerous
bool isHumidityDangerous(float humidity) {
    if (humidity < 0 || humidity > 120) return false; // Invalid data
    // Dangerous if: humidity < 40 OR humidity >= 60 
    return (humidity < 40 || humidity >= 70);
}

// Check if pressure is dangerous 
bool isPressureDangerous(float pressure) {
    if (pressure < 500 || pressure > 1000) return false; // Invalid data
    // Dangerous if: pressure < 747 OR pressure >= 775 
    return (pressure < 747 || pressure >= 767);
}

// Parse JSON data into struct with validation
void _parseJsonToStruct(const String &jsonString, main_screen_values_t &values) {
    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, jsonString);
     if (error) {
        debug_outln_info(F("deserializeJson() failed display: "), error.f_str());
        return;
    }

    JsonObject data = doc.as<JsonObject>();
    String urban_key = ATRUIST_URBAN_SENSOR;

    if (data.containsKey(urban_key)) {
        auto urban = data[urban_key];
        if (urban.containsKey("IP_address")) values.ip_address = urban["IP_address"]["value"].as<String>();
        
        // PM values with validation (reasonable ranges: 0-500 μg/m³)
        if (urban.containsKey("SDS_P1")) {
            float pm10 = urban["SDS_P1"]["value"].as<float>();
            values.pm10 = isValidRange(pm10, 0, 1500) ? pm10 : -1;
        }
        if (urban.containsKey("SDS_P2")) {
            float pm25 = urban["SDS_P2"]["value"].as<float>();
            values.pm25 = isValidRange(pm25, 0, 800) ? pm25 : -1;
        }
        
        // Environmental values with validation
        if (urban.containsKey("BME280_humidity")) {
            float hum = urban["BME280_humidity"]["value"].as<float>();
            values.hum_outdoor = isValidRange(hum, 0, 100) ? hum : -1;
        }
        if (urban.containsKey("BME280_temperature")) {
            float temp = urban["BME280_temperature"]["value"].as<float>();
            values.temp_outdoor = isValidRange(temp, -40, 80) ? temp : -1;
        }
        if (urban.containsKey("BME280_pressure")) {
            float press = urban["BME280_pressure"]["value"].as<float>() * 0.0075;
            values.press_outdoor = isValidRange(press, 500, 1000) ? press : -1;
        }
        
        // Noise values with validation (0-120 dB)
        if (urban.containsKey("PCBA_noiseMax")) {
            float noise = urban["PCBA_noiseMax"]["value"].as<float>();
            values.noise_max = isValidRange(noise, 0, 120) ? noise : -1;
        }
        if (urban.containsKey("PCBA_noiseAvg")) {
            float noise = urban["PCBA_noiseAvg"]["value"].as<float>();
            values.noise_avg = isValidRange(noise, 0, 120) ? noise : -1;
        }
    }
    
    // Indoor CO2 with validation (300-5000 ppm)
    if (data.containsKey("SCD4x")) {
        float co2 = data["SCD4x"]["co2"]["value"].as<float>();
        values.co2 = isValidRange(co2, 300, 5000) ? co2 : -1;
    }
    
    // Indoor environment with validation
    if (data.containsKey("BME680")) {
        auto bme = data["BME680"];
        if (bme.containsKey("temperature")) {
            float temp = bme["temperature"]["value"].as<float>();
            values.temp_indoor = isValidRange(temp, -40, 80) ? temp : -1;
        }
        if (bme.containsKey("humidity")) {
            float hum = bme["humidity"]["value"].as<float>();
            values.hum_indoor = isValidRange(hum, 0, 100) ? hum : -1;
        }
        if (bme.containsKey("pressure")) {
            float press = bme["pressure"]["value"].as<float>() * 0.0075;
            values.press_indoor = isValidRange(press, 500, 1000) ? press : -1;
        }
    }
}

// Draw the full main screen with Urban 2-subcolumn layout
void drawMainScreen(UBYTE *BlackImage, const String &jsonString, const String &device_ip) {
    main_screen_values_t values;
    _parseJsonToStruct(jsonString, values);

    // Clear screen first to remove any white lines
    Paint_Clear(WHITE);
    
    // Date and time in a row
    struct tm timeinfo; 
    uint16_t header_height = Font16.Height + 4; // Height for header row
    uint16_t header_y_offset = 12; // Move date/time lower
    uint16_t header_bottom_border_y = 0;
    if (getLocalTime(&timeinfo)) {
        char date_buf[12], time_buf[8];
        strftime(date_buf, sizeof(date_buf), "%m/%d/%Y", &timeinfo);
        strftime(time_buf, sizeof(time_buf), "%H:%M", &timeinfo);
        
        // Calculate total width of date, pipe separator, and time with spacing
        int date_width = strlen(date_buf) * Font12.Width;
        int time_width = strlen(time_buf) * Font12.Width;
        int pipe_width = Font12.Width; // width of "|" character
        int spacing = 8; // spacing between date and pipe, and pipe and time
        int total_width = date_width + spacing + pipe_width + spacing + time_width;
        
        // Center the combined date, pipe, and time
        int start_x = DISPLAY_WIDTH / 2 - total_width / 2;
        int center_y = header_y_offset;
        
        // Draw date, pipe separator, and time
        Paint_DrawString_EN(start_x, center_y, date_buf, &Font12, WHITE, BLACK);
        Paint_DrawString_EN(start_x + date_width + spacing, center_y, "|", &Font12, WHITE, BLACK);
        Paint_DrawString_EN(start_x + date_width + spacing + pipe_width + spacing, center_y, time_buf, &Font12, WHITE, BLACK);
        
        // Calculate where the bottom border should be
        header_bottom_border_y = center_y + Font16.Height + 6;
    }

    // Draw bottom border for header
    Paint_DrawLine(0, header_bottom_border_y, DISPLAY_WIDTH, header_bottom_border_y, 
                   BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

    // Section headers row (below header) - bigger gap
    uint16_t y_start = header_bottom_border_y + 18;

    // Save right sidebar for vertical navigation icons (approx 28px)
    const uint16_t nav_sidebar_width = 28;
    uint16_t usable_width = (DISPLAY_WIDTH > nav_sidebar_width) ? (DISPLAY_WIDTH - nav_sidebar_width) : DISPLAY_WIDTH;

    uint16_t urban_width = (usable_width * 2) / 3; // Urban gets 2/3 of usable area
    uint16_t insight_width = usable_width / 3;     // Insight gets 1/3 of usable area
    uint16_t value_spacing = 50;

    // === URBAN SENSOR SECTION (2 sub-columns) ===
    // Simple section header
    Paint_DrawString_EN(8, y_start, "URBAN SENSOR", &Font16, WHITE, BLACK);
    // Determine Urban status: online if we have IP address OR any valid sensor data
    // This is more reliable than just checking IP address, as sensor data indicates Urban is actually working
    bool urban_has_data = (values.pm10 >= 0 || values.pm25 >= 0 || values.temp_outdoor >= -40 || 
                           values.hum_outdoor >= 0 || values.press_outdoor >= 0 || 
                           values.noise_max >= 0 || values.noise_avg >= 0);
    String urban_status = (values.ip_address.length() > 0) ? values.ip_address : 
                          (urban_has_data ? "Online" : "Offline");
    Paint_DrawString_EN(8, y_start + Font16.Height + 2, urban_status.c_str(), &Font12, WHITE, BLACK);
    
    uint16_t urban_y = y_start + Font16.Height + Font12.Height + 15;
    uint16_t urban_subcol_width = urban_width / 2;

    // Urban sub-column 1 (left)
    drawValue("PM10", values.pm10, 1, air_filter_20x20, "ppm", 20, 8, urban_y, 5, false, SOURCE_URBAN, isPMDangerous(values.pm10, values.pm25));
    drawValue("PM2.5", values.pm25, 1, air_pollution_20x20, "ppm", 20, 8, urban_y + value_spacing, 5, false, SOURCE_URBAN, isPMDangerous(values.pm10, values.pm25));
    drawValue("Noise Max", values.noise_max, 0, ear_hearing_20x20, "dB", 20, 8, urban_y + 2 * value_spacing, 5, false, SOURCE_URBAN, isNoiseDangerous(values.noise_max));
    drawValue("Noise Avg", values.noise_avg, 0, ear_hearing_20x20, "dB", 20, 8, urban_y + 3 * value_spacing, 5, false, SOURCE_URBAN, isNoiseDangerous(values.noise_avg));
    
    // Urban sub-column 2 (right)
    drawValue("Temperature", values.temp_outdoor, 1, wi_thermometer_cropped_20x20, "C", 20, urban_subcol_width + 8, urban_y, 5, false, SOURCE_URBAN, isTempDangerous(values.temp_outdoor));
    drawValue("Humidity", values.hum_outdoor, 0, wi_humidity_cropped_20x20, "%", 20, urban_subcol_width + 8, urban_y + value_spacing, 5, false, SOURCE_URBAN, isHumidityDangerous(values.hum_outdoor));
    drawValue("Pressure", values.press_outdoor, 0, pressure_20x20, "mmHg", 20, urban_subcol_width + 8, urban_y + 2 * value_spacing, 2, false, SOURCE_URBAN, isPressureDangerous(values.press_outdoor));

    // === INSIGHT DEVICE SECTION (1 column) ===
    // Simple section header
    Paint_DrawString_EN(urban_width + 8, y_start, "INSIGHT", &Font16, WHITE, BLACK);
    String insight_status = device_ip.length() > 0 ? device_ip : "Offline";
    Paint_DrawString_EN(urban_width + 8, y_start + Font16.Height + 2, insight_status.c_str(), &Font12, WHITE, BLACK);
    
    uint16_t insight_y = y_start + Font16.Height + Font12.Height + 15;
    
    // Insight device data (single column)
    drawValue("Temperature", values.temp_indoor, 1, house_thermometer_20x20, "C", 20, urban_width + 8, insight_y, 2, false, SOURCE_INSIGHT, isTempDangerous(values.temp_indoor));
    drawValue("Humidity", values.hum_indoor, 0, wi_humidity_cropped_20x20, "%", 20, urban_width + 8, insight_y + value_spacing, 2, false, SOURCE_INSIGHT, isHumidityDangerous(values.hum_indoor));
    drawValue("Pressure", values.press_indoor, 0, pressure_20x20, "mmHg", 20, urban_width + 8, insight_y + 2 * value_spacing, 2, false, SOURCE_INSIGHT, isPressureDangerous(values.press_indoor));
    drawValue("CO2", values.co2, 0, co2_svgrepo_com_20x20, "ppm", 20, urban_width + 8, insight_y + 3 * value_spacing, 5, false, SOURCE_INSIGHT, isCO2Dangerous(values.co2));
}

#endif