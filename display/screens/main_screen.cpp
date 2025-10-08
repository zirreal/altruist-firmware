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
#include "../icons/icons/icons_40x40.h"
#include "../icons/icons/icons_35x35.h"
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
               uint16_t image_offset = 0, bool highlight = false, DataSource source = SOURCE_INSIGHT) {
    
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
        
        // More balanced value display - not too big
        Paint_DrawString_EN(x_start + image_size + image_offset + 3,
                            y_start + Font12.Height + 3,
                            value_str, &Font20, WHITE, BLACK);
        
        // Better positioned units - use actual value width for more accurate positioning
        uint16_t value_pixel_width = strlen(value_str) * Font20.Width;
        Paint_DrawString_EN(x_start + image_size + image_offset + value_pixel_width + 6,
                            y_start + Font12.Height + 6,
                            units, &Font12, WHITE, BLACK);
    }
}

// Validate sensor data ranges for quality indication
bool isValidRange(float value, float min_val, float max_val) {
    return (value >= min_val && value <= max_val);
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
            values.pm10 = isValidRange(pm10, 0, 500) ? pm10 : -1;
        }
        if (urban.containsKey("SDS_P2")) {
            float pm25 = urban["SDS_P2"]["value"].as<float>();
            values.pm25 = isValidRange(pm25, 0, 300) ? pm25 : -1;
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
            values.press_outdoor = isValidRange(press, 500, 800) ? press : -1;
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
            values.press_indoor = isValidRange(press, 500, 800) ? press : -1;
        }
    }
}

// Draw the full main screen with Urban 2-subcolumn layout
void drawMainScreen(UBYTE *BlackImage, const String &jsonString, const String &device_ip) {
    main_screen_values_t values;
    _parseJsonToStruct(jsonString, values);

    // Clear screen first to remove any white lines
    Paint_Clear(WHITE);
    
    // Simple black line/bar at top (a bit longer)
    uint16_t top_bar_height = Font16.Height + Font8.Height;
    Paint_DrawRectangle(0, 0, DISPLAY_WIDTH, top_bar_height, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    
    // Date and time on top of black bar
    struct tm timeinfo; 
    if (getLocalTime(&timeinfo)) {
        char date_buf[12], time_buf[8];
        strftime(date_buf, sizeof(date_buf), "%m/%d/%Y", &timeinfo);
        strftime(time_buf, sizeof(time_buf), "%H:%M", &timeinfo);
        
        int date_x = DISPLAY_WIDTH / 2 - strlen(date_buf) * Font12.Width / 2;
        int time_x = DISPLAY_WIDTH / 2 - strlen(time_buf) * Font16.Width / 2;
        
        Paint_DrawString_EN(date_x, 2, date_buf, &Font12, BLACK, WHITE);
        Paint_DrawString_EN(time_x, Font12.Height + 2, time_buf, &Font16, BLACK, WHITE);
    }

    // Section headers row (below time) - bigger gap
    uint16_t y_start = top_bar_height + 18;
    uint16_t urban_width = (DISPLAY_WIDTH * 2) / 3; // Urban gets 2/3 of screen
    uint16_t insight_width = DISPLAY_WIDTH / 3;     // Insight gets 1/3 of screen
    uint16_t value_spacing = 50;

    // Vertical separator line with border
    // Paint_DrawLine(urban_width, y_start, urban_width, DISPLAY_HEIGHT, WHITE, DOT_PIXEL_2X2, LINE_STYLE_SOLID);
    // Paint_DrawLine(urban_width - 1, y_start, urban_width - 1, DISPLAY_HEIGHT, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    // Paint_DrawLine(urban_width + 1, y_start, urban_width + 1, DISPLAY_HEIGHT, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

    // === URBAN SENSOR SECTION (2 sub-columns) ===
    // Simple section header
    Paint_DrawString_EN(8, y_start, "URBAN SENSOR", &Font16, WHITE, BLACK);
    String urban_status = values.ip_address.length() > 0 ? values.ip_address : "Offline";
    Paint_DrawString_EN(8, y_start + Font16.Height + 2, urban_status.c_str(), &Font12, WHITE, BLACK);
    
    uint16_t urban_y = y_start + Font16.Height + Font12.Height + 15;
    uint16_t urban_subcol_width = urban_width / 2;
    
    // Urban sub-column 1 (left)
    drawValue("PM10", values.pm10, 1, air_filter_35x35, "ppm", 35, 8, urban_y, 5, false, SOURCE_URBAN);
    drawValue("PM2.5", values.pm25, 1, air_pollution_35x35, "ppm", 35, 8, urban_y + value_spacing, 5, false, SOURCE_URBAN);
    drawValue("Noise Max", values.noise_max, 0, ear_hearing_35x35, "dB", 35, 8, urban_y + 2 * value_spacing, 5, false, SOURCE_URBAN);
    drawValue("Noise Avg", values.noise_avg, 0, ear_hearing_35x35, "dB", 35, 8, urban_y + 3 * value_spacing, 5, false, SOURCE_URBAN);
    
    // Urban sub-column 2 (right)
    drawValue("Temperature", values.temp_outdoor, 1, wi_thermometer_cropped_35x35, "C", 35, urban_subcol_width + 8, urban_y, 5, false, SOURCE_URBAN);
    drawValue("Humidity", values.hum_outdoor, 0, wi_humidity_cropped_35x35, "%", 35, urban_subcol_width + 8, urban_y + value_spacing, 5, false, SOURCE_URBAN);
    drawValue("Pressure", values.press_outdoor, 0, pressure_35x35, "mmHg", 35, urban_subcol_width + 8, urban_y + 2 * value_spacing, 2, false, SOURCE_URBAN);

    // === INSIGHT DEVICE SECTION (1 column) ===
    // Simple section header
    Paint_DrawString_EN(urban_width + 8, y_start, "INSIGHT", &Font16, WHITE, BLACK);
    String insight_status = device_ip.length() > 0 ? device_ip : "Offline";
    Paint_DrawString_EN(urban_width + 8, y_start + Font16.Height + 2, insight_status.c_str(), &Font12, WHITE, BLACK);
    
    uint16_t insight_y = y_start + Font16.Height + Font12.Height + 15;
    
    // Insight device data (single column)
    drawValue("Temperature", values.temp_indoor, 1, house_thermometer_35x35, "C", 35, urban_width + 8, insight_y, 2, false, SOURCE_INSIGHT);
    drawValue("Humidity", values.hum_indoor, 0, wi_humidity_cropped_35x35, "%", 35, urban_width + 8, insight_y + value_spacing, 2, false, SOURCE_INSIGHT);
    drawValue("Pressure", values.press_indoor, 0, pressure_35x35, "mmHg", 35, urban_width + 8, insight_y + 2 * value_spacing, 2, false, SOURCE_INSIGHT);
    drawValue("CO2", values.co2, 0, co2_svgrepo_com_35x35, "ppm", 35, urban_width + 8, insight_y + 3 * value_spacing, 5, false, SOURCE_INSIGHT);
}

#endif