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
#include "../icons/icons/icons_15x15.h"
#include "../icons/icons/icons_10x10.h"
#include "../../defines.h"
#include "../utils.h"
#include "../../config_manager/config_helpers.h"
#include "display_common.h"

// Helper function to draw an image flipped vertically (upside down)
static void Paint_DrawImageFlippedVertical(const unsigned char *image_buffer, UWORD xStart, UWORD yStart, UWORD W_Image, UWORD H_Image) {
    UWORD x, y;
    UWORD byte_width = (W_Image % 8) ? (W_Image / 8 + 1) : (W_Image / 8);

    for (y = 0; y < H_Image; y++) {
        for (x = 0; x < W_Image; x++) {
            // Read from bottom row instead of top row (flip vertically)
            UWORD source_y = H_Image - 1 - y;
            UWORD byte_index = (source_y * byte_width) + (x / 8);
            UBYTE byte = image_buffer[byte_index];
            UBYTE bit = 0x80 >> (x % 8);  // MSB first

            UWORD color = (byte & bit) ? WHITE : BLACK;
            Paint_SetPixel(xStart + x, yStart + y, color);
        }
    }
}


// Data source indicators
enum DataSource {
    SOURCE_URBAN = 0,
    SOURCE_INSIGHT = 1
};

// Draw a value with icon, label, units and data source indicator
// danger_direction:
//   0  -> normal
//  +1  -> above green range (high)
  //  -1  -> below green range (low)
void drawValue(const char *label, float value, uint8_t precision,
               const unsigned char *image, const char *units,
               uint16_t image_size, uint16_t x_start, uint16_t y_start,
               uint16_t image_offset = 0, bool highlight = false,
               DataSource source = SOURCE_INSIGHT,
               bool is_dangerous = false,
               int  danger_direction = 0) {
    
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
        
        // Draw warning icon (circle with exclamation point) and optional direction arrow
        int dir = danger_direction;
        if (dir == 0 && is_dangerous) {
            // If only a boolean is given, assume "high"
            dir = 1;
        }

        if (dir != 0) {
            String debug_msg = String(F("Drawing warning icon and arrow for dangerous value: ")) + String(label) + F(" = ") + String(value_str);
            debug_outln_info(debug_msg);
            uint16_t units_width = strlen(units) * Font12.Width;
            
            // Position for warning icon (pre-generated bitmap)
            uint16_t icon_x = units_x + units_width + 6;  // Position after units
            uint16_t icon_y = y_start + Font12.Height + 5;
            Paint_DrawImage(warning_10x10, icon_x, icon_y, 10, 10);

            // Direction arrow icon - place it right after the measure title (label)
            const uint16_t arrow_size = 10;
            uint16_t label_width = strlen(label) * Font12.Width;
            uint16_t label_x = x_start + image_size + image_offset + 3; // Same x as label text
            uint16_t arrow_x = label_x + label_width + 1; // Right after label text
            uint16_t arrow_y = y_start + (Font12.Height / 2) - (arrow_size / 2); // Vertically centered with label

            if (dir > 0) {
                // Above green range -> arrow up (use arrow icon as-is)
                Paint_DrawImage(arrow_10x10, arrow_x, arrow_y - 2, arrow_size, 20);
            } else {
                // Below green range -> flip arrow vertically (upside down)
                Paint_DrawImageFlippedVertical(arrow_10x10, arrow_x, arrow_y, arrow_size, 20);
            }
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

// Direction helpers: -1 = below green, +1 = above, 0 = normal/OK
int tempDangerDirection(float temperature) {
    if (temperature < -40 || temperature > 80) return 0;
    if (temperature < 10)  return -1;
    if (temperature >= 27) return 1;
    return 0;
}

int humidityDangerDirection(float humidity) {
    if (humidity < 0 || humidity > 120) return 0;
    if (humidity < 40)  return -1;
    if (humidity >= 70) return 1;
    return 0;
}

int pressureDangerDirection(float pressure) {
    if (pressure < 500 || pressure > 1000) return 0;
    if (pressure < 747)  return -1;
    if (pressure >= 767) return 1;
    return 0;
}

int pmDangerDirection(float pm10, float pm25) {
    if (pm10 < 0 || pm25 < 0) return 0;
    return (pm10 >= 100 || pm25 >= 55) ? 1 : 0;  // only "too high" is dangerous
}

int noiseDangerDirection(float noise) {
    if (noise < 0) return 0;
    return (noise >= 70) ? 1 : 0; // only "too loud" is dangerous
}

int co2DangerDirection(float co2) {
    if (co2 < 0) return 0;
    return (co2 >= 1000) ? 1 : 0;
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
    
    // === HEADER: left icon (current page), center time, right date ===
    struct tm timeinfo; 
    const uint16_t header_top_y     = 6;
    const uint16_t header_row_height = Font16.Height + 2; // room for 16px font
    uint16_t header_bottom_border_y = header_top_y + header_row_height + 2;

    if (getLocalTime(&timeinfo)) {
        char date_buf[12], time_buf[8];
        strftime(date_buf, sizeof(date_buf), "%m/%d/%Y", &timeinfo);
        strftime(time_buf, sizeof(time_buf), "%H:%M",    &timeinfo);

        // Left: current page icon (home for main screen)
        const uint16_t header_icon_size = 15;
        const uint16_t header_icon_x    = 4;
        const uint16_t header_icon_y    = header_top_y;
        Paint_DrawImage(home_nav_15x15, header_icon_x, header_icon_y, header_icon_size, header_icon_size);

        // Center: time in bold (use Font16)
        int time_width = strlen(time_buf) * Font16.Width;
        int time_x = (DISPLAY_WIDTH - time_width) / 2;
        int time_y = header_top_y;
        Paint_DrawString_EN(time_x, time_y, time_buf, &Font16, WHITE, BLACK);

        // Right: date, smaller but still bold-ish (Font12 drawn twice with 1px offset)
        int date_width = strlen(date_buf) * Font12.Width;
        const int right_margin = 4;
        int date_x = DISPLAY_WIDTH - right_margin - date_width;
        int date_y = header_top_y + 2;
        // draw twice with slight offset to fake bold
        Paint_DrawString_EN(date_x,     date_y, date_buf, &Font12, WHITE, BLACK);
        Paint_DrawString_EN(date_x + 1, date_y, date_buf, &Font12, WHITE, BLACK);
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

    // === URBAN SECTION (2 sub-columns) ===
    // Section header with icon on the left
    const uint16_t header_icon_size = 20;
    const uint16_t header_icon_text_offset = 4;
    const uint16_t urban_header_icon_x = 8;

    // Use correct 20x20 size for urban icon
    // Add small x offset to prevent left-side clipping (icon might have left padding in data)
    Paint_DrawImage(urban_20x20, urban_header_icon_x + 1, y_start + 2, header_icon_size, header_icon_size);
    uint16_t urban_header_text_x = urban_header_icon_x + header_icon_size + header_icon_text_offset;
    Paint_DrawString_EN(urban_header_text_x, y_start + 2, "URBAN", &Font16, WHITE, BLACK);

    // Determine Urban status with simple debouncing & caching:
    // - Consider Urban "online" if we have IP address OR any valid sensor data
    // - Require several consecutive "offline" reads before showing "Offline" on screen
    //   to avoid random glitches / missing packets
    static uint8_t consecutive_urban_offline_reads = 0;
    static String  last_urban_status = "";

    bool urban_has_data = (values.pm10  >= 0 ||
                           values.pm25  >= 0 ||
                           values.temp_outdoor > -40 ||
                           values.hum_outdoor  >= 0 ||
                           values.press_outdoor >= 0 ||
                           values.noise_max   >= 0 ||
                           values.noise_avg   >= 0);

    bool urban_now_online = (values.ip_address.length() > 0) || urban_has_data;
    String urban_status;

    if (urban_now_online) {
        // Immediately trust an online reading and reset the counter
        consecutive_urban_offline_reads = 0;
        urban_status = (values.ip_address.length() > 0)
                           ? values.ip_address
                           : String(F("Online"));
        last_urban_status = urban_status;
    } else {
        // Only switch to "Offline" after several consecutive missing reads
        if (consecutive_urban_offline_reads < 3) {
            consecutive_urban_offline_reads++;
            // While we are not yet sure it's really offline, keep showing last known status if any
            if (last_urban_status.length() > 0) {
                urban_status = last_urban_status;
            } else {
                urban_status = String(F("Offline"));
            }
        } else {
            urban_status = String(F("Offline"));
            last_urban_status = urban_status;
        }
    }

    Paint_DrawString_EN(urban_header_text_x, y_start + Font16.Height + 2, urban_status.c_str(), &Font12, WHITE, BLACK);
    
    uint16_t urban_y = y_start + Font16.Height + Font12.Height + 15;
    uint16_t urban_subcol_width = urban_width / 2;

    // Urban sub-column 1 (left)
    int pm_dir    = pmDangerDirection(values.pm10, values.pm25);
    int noise_dir = noiseDangerDirection(values.noise_max); // use max for direction
    drawValue("PM10",      values.pm10,      1, air_filter_20x20,    "ppm",
              20, 8, urban_y,                 5, false, SOURCE_URBAN, (pm_dir != 0),    pm_dir);
    drawValue("PM2.5",     values.pm25,      1, air_pollution_20x20, "ppm",
              20, 8, urban_y + value_spacing, 5, false, SOURCE_URBAN, (pm_dir != 0),    pm_dir);
    drawValue("Noise Max", values.noise_max, 0, ear_hearing_20x20,   "dB",
              20, 8, urban_y + 2 * value_spacing, 5, false, SOURCE_URBAN, (noise_dir != 0), noise_dir);
    drawValue("Noise Avg", values.noise_avg, 0, ear_hearing_20x20,   "dB",
              20, 8, urban_y + 3 * value_spacing, 5, false, SOURCE_URBAN, (noise_dir != 0), noise_dir);
    
    // Urban sub-column 2 (right)
    int temp_out_dir  = tempDangerDirection(values.temp_outdoor);
    int hum_out_dir   = humidityDangerDirection(values.hum_outdoor);
    int press_out_dir = pressureDangerDirection(values.press_outdoor);
    drawValue("Temperature", values.temp_outdoor, 1, wi_thermometer_cropped_20x20, "C",
              20, urban_subcol_width + 8, urban_y,                 5, false, SOURCE_URBAN, (temp_out_dir != 0),  temp_out_dir);
    drawValue("Humidity",    values.hum_outdoor,  0, wi_humidity_cropped_20x20,     "%",
              20, urban_subcol_width + 8, urban_y + value_spacing, 5, false, SOURCE_URBAN, (hum_out_dir != 0),   hum_out_dir);
    drawValue("Pressure",    values.press_outdoor,0, pressure_20x20,               "mmHg",
              20, urban_subcol_width + 8, urban_y + 2 * value_spacing, 2, false, SOURCE_URBAN, (press_out_dir != 0), press_out_dir);

    // === INSIGHT DEVICE SECTION (1 column) ===
    // Section header with icon on the left
    const uint16_t insight_header_icon_x = urban_width + 8;
    Paint_DrawImage(insight_20x20, insight_header_icon_x, y_start + 4, header_icon_size, header_icon_size);
    uint16_t insight_header_text_x = insight_header_icon_x + header_icon_size + header_icon_text_offset;
    Paint_DrawString_EN(insight_header_text_x, y_start + 2, "INSIGHT", &Font16, WHITE, BLACK);
    String insight_status = device_ip.length() > 0 ? device_ip : "Offline";
    Paint_DrawString_EN(insight_header_text_x, y_start + Font16.Height + 2, insight_status.c_str(), &Font12, WHITE, BLACK);
    
    uint16_t insight_y = y_start + Font16.Height + Font12.Height + 15;
    
    // Insight device data (single column)
    int temp_in_dir  = tempDangerDirection(values.temp_indoor);
    int hum_in_dir   = humidityDangerDirection(values.hum_indoor);
    int press_in_dir = pressureDangerDirection(values.press_indoor);
    int co2_dir      = co2DangerDirection(values.co2);

    drawValue("Temperature", values.temp_indoor, 1, house_thermometer_20x20, "C",
              20, urban_width + 8, insight_y,                 2, false, SOURCE_INSIGHT, (temp_in_dir != 0),  temp_in_dir);
    drawValue("Humidity",    values.hum_indoor,  0, wi_humidity_cropped_20x20,     "%",
              20, urban_width + 8, insight_y + value_spacing, 2, false, SOURCE_INSIGHT, (hum_in_dir != 0),   hum_in_dir);
    drawValue("Pressure",    values.press_indoor,0, pressure_20x20,               "mmHg",
              20, urban_width + 8, insight_y + 2 * value_spacing, 2, false, SOURCE_INSIGHT, (press_in_dir != 0), press_in_dir);
    drawValue("CO2",         values.co2,         0, co2_svgrepo_com_20x20,        "ppm",
              20, urban_width + 8, insight_y + 3 * value_spacing, 5, false, SOURCE_INSIGHT, (co2_dir != 0),      co2_dir);
}

#endif