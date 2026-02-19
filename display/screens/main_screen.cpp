#ifdef ALTRUIST_INSIDE

#include "main_screen.h"
#include <ArduinoJson.h>
#include <string.h>
#include <stdio.h>
#include "../driver/DEV_Config.h"
#include "../driver/EPD.h"
#include <stdlib.h>
#include "utils.h"
#include "../icons/icons/icons_20x20.h"
#include "../icons/icons/icons_15x15.h"
#include "../icons/icons/icons_10x10.h"
#include "../icons/icons/icons_30x30.h"
#include "../icons/icons/icons_35x35.h"
#include "../../defines.h"
#include "../utils.h"
#include "../../utils.h"
#include "../../config_manager/config_helpers.h"
#include "display_common.h"
#include "../../intl.h"
#include "../paint_driver/fonts/fonts.h"
#include <qrcode.h>

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
               uint16_t image_size,
               uint16_t x_start, uint16_t y_start,
               uint16_t column_right_x,
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
    
    uint16_t label_x = x_start;
    uint16_t label_y = y_start;
    // Slightly smaller font for measure titles 
    uint16_t label_width = Paint_GetStringWidth_Display(label, &Font12, &font_12_cyrillic, &font_12_ascii);

    // Decide danger direction and whether we show warning state
    int dir = danger_direction;
    if (dir == 0 && is_dangerous) {
        dir = 1;
    }
    bool has_warning = (dir != 0);

    // Draw label 
    Paint_DrawString_Display(label_x, label_y, label, &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);

    // Add a small right padding so values don't touch the border
    uint16_t effective_column_right = (column_right_x > 4) ? (column_right_x - 4) : column_right_x;

    // "No data" handling:
    // - Most values use -1 as the no-data sentinel
    // - Temperature uses a sentinel outside valid range so -1°C can be displayed
    // Use epsilon comparison to account for floating point precision
    const float NO_DATA_SENTINEL      = -1.0f;
    const float NO_TEMP_DATA_SENTINEL = -1000.0f;
    const float EPSILON = 0.1f;
    const bool  is_temperature = (strcmp(label, INTL_DISP_TEMPERATURE) == 0);
    const float sentinel = is_temperature ? NO_TEMP_DATA_SENTINEL : NO_DATA_SENTINEL;
    if (value < (sentinel + EPSILON) && value > (sentinel - EPSILON)) {
        // No data: show "--" right-aligned within the column
        const char *no_data = INTL_DISP_NO_DATA;
        uint16_t nd_width = strlen(no_data) * Font12.Width;
        uint16_t min_value_x = label_x + label_width + 8;
        uint16_t value_x = (effective_column_right > nd_width && effective_column_right - nd_width > min_value_x)
                               ? (effective_column_right - nd_width)
                               : min_value_x;
        uint16_t value_y = label_y;
        Paint_DrawString_Display(value_x, value_y, no_data, &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
    } else {
        char value_str[12];
        stringFromFloat(value_str, value, precision);
        
        // Use the same font as labels for values
        sFONT* value_font = &Font12;
        
        uint16_t value_pixel_width = Paint_GetStringWidth_Display(value_str, value_font, &font_12_cyrillic, &font_12_ascii);
        const int8_t bold_letter_spacing = 1;  /* extra px between digits when bold for readability (e.g. CO2 1058) */
        uint16_t value_width_for_layout = value_pixel_width;
        if (has_warning) {
            size_t len = strlen(value_str);
            if (len > 1) value_width_for_layout += (uint16_t)((len - 1) * bold_letter_spacing);
        }
        uint16_t units_pixel_width = Paint_GetStringWidth_Display(units, &Font12, &font_12_cyrillic, &font_12_ascii);
        const uint16_t gap_value_units = 4;
        uint16_t total_width = value_width_for_layout + gap_value_units + units_pixel_width;


        const uint16_t warning_icon_width = 16; // warning icon 16x16 (manually drawn)
        const uint16_t arrow_icon_width   = 14; // arrow icon 14x14 (manually drawn)
        const uint16_t warning_and_arrow_width = warning_icon_width + arrow_icon_width + 2 + 1;
        uint16_t min_value_x = label_x + label_width +
                               (has_warning ? (warning_and_arrow_width + 4) : 8);

        uint16_t value_x = (effective_column_right > total_width && effective_column_right - total_width > min_value_x)
                               ? (effective_column_right - total_width)
                               : min_value_x;
        uint16_t value_y = label_y;
        uint16_t units_x = value_x + value_width_for_layout + gap_value_units;
        uint16_t units_y = label_y;

        // Make the numeric value bold if there is a warning (with letter spacing for readability); otherwise normal
        if (has_warning) {
            Paint_DrawString_Display_WithSpacing(value_x,     value_y, value_str, value_font, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK, bold_letter_spacing);
            Paint_DrawString_Display_WithSpacing(value_x + 1, value_y, value_str, value_font, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK, bold_letter_spacing);
        } else {
            Paint_DrawString_Display(value_x, value_y, value_str, value_font, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
        }
        Paint_DrawString_Display(units_x, units_y, units, &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
    }

    // Draw warning icon and arrow next to the title (label)
    if (has_warning) {
        debug_outln_verbose(String(F("Drawing warning icon and arrow for dangerous value: ")) + String(label) + F(" = ") + String(value));

        // Warning icon - manually drawn 16x16 filled triangle with exclamation
        const uint16_t warning_size = 16;
        uint16_t warning_x = label_x + label_width + 4;
        uint16_t warning_y = label_y + (Font12.Height - warning_size) / 2;
        
        // Draw filled warning triangle (pointing up) with border radius
        uint16_t tri_top_x = warning_x + warning_size / 2;
        uint16_t tri_top_y = warning_y;
        uint16_t tri_bottom_y = warning_y + warning_size - 1;
        uint16_t tri_height = tri_bottom_y - tri_top_y;
        
        // Draw filled triangle by drawing horizontal lines from top to bottom
        for (uint16_t y = tri_top_y; y <= tri_bottom_y; y++) {
            // Calculate width at this y position (triangle widens as we go down)
            uint16_t height_from_top = y - tri_top_y;
            // Width increases linearly from 1 at top to warning_size at bottom
            uint16_t width_at_y = 1 + (height_from_top * (warning_size - 1)) / tri_height;
            uint16_t left_x = tri_top_x - width_at_y / 2;
            uint16_t right_x = tri_top_x + (width_at_y - 1) / 2;
            
            // Add border radius effect: round only the bottom corners, keep top sharp
            if (y >= tri_bottom_y - 1) {
                // Bottom rows: round the bottom corners by slightly reducing width
                uint16_t corner_reduction = (y == tri_bottom_y) ? 1 : 0;
                Paint_DrawLine(left_x + corner_reduction, y, right_x - corner_reduction, y, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
            } else {
                // Top and middle rows: normal triangle (sharp top)
                Paint_DrawLine(left_x, y, right_x, y, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
            }
        }
        
        // Draw exclamation mark in the center (white on black triangle) - properly centered
        uint16_t exclam_x = tri_top_x;  // Centered on triangle (triangle center point)
        uint16_t exclam_top_y = warning_y + 5;  // Start position for smaller triangle
        uint16_t exclam_mid_y = warning_y + warning_size - 6;  // End before dot
        uint16_t exclam_bottom_y = warning_y + warning_size - 3;  // Dot position
        // Exclamation line (vertical) - perfectly straight, 2 pixels wide (a bit thicker)
        Paint_DrawLine(exclam_x - 1, exclam_top_y, exclam_x - 1, exclam_mid_y, 
                      WHITE, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        Paint_DrawLine(exclam_x, exclam_top_y, exclam_x, exclam_mid_y, 
                      WHITE, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        // Exclamation dot (bottom) - slimmer shape with added width
        // Draw a rectangle (2x2) for better appearance
        Paint_DrawRectangle(exclam_x - 1, exclam_bottom_y - 1, 
                           exclam_x, exclam_bottom_y + 1,
                           WHITE, DOT_PIXEL_1X1, DRAW_FILL_FULL);

        // Direction arrow icon - manually drawn clean arrow shape (bigger and bolder)
        const uint16_t arrow_size = 14;
        uint16_t arrow_x = warning_x + warning_size; // Moved even closer (reduced spacing to 0)
        // Center arrow to bottom of triangle, but move it up a bit
        uint16_t arrow_y = warning_y + warning_size - arrow_size - 2; // Align with bottom but move up 2px
        
        uint16_t arrow_center_x = arrow_x + arrow_size / 2; // Centered, no offset
        uint16_t arrow_top_y = arrow_y;
        uint16_t arrow_bottom_y = arrow_y + arrow_size; // Shorter - removed the +2 extension
        
        if (dir > 0) {
            // Above green range -> arrow UP
            // Bolder arrowhead: triangle at top (4 rows for better visibility)
            // Top point
            Paint_DrawPoint(arrow_center_x, arrow_top_y, BLACK, DOT_PIXEL_1X1, DOT_STYLE_DFT);
            // Second row: 3 pixels
            Paint_DrawLine(arrow_center_x - 1, arrow_top_y + 1, arrow_center_x + 1, arrow_top_y + 1,
                          BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
            // Third row: 5 pixels
            Paint_DrawLine(arrow_center_x - 2, arrow_top_y + 2, arrow_center_x + 2, arrow_top_y + 2,
                          BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
            // Fourth row: 7 pixels (bolder)
            Paint_DrawLine(arrow_center_x - 3, arrow_top_y + 3, arrow_center_x + 3, arrow_top_y + 3,
                          BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
            // Thicker shaft: 3 pixels wide, centered in arrowhead
            Paint_DrawLine(arrow_center_x - 1, arrow_top_y + 4, arrow_center_x - 1, arrow_bottom_y,
                          BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
            Paint_DrawLine(arrow_center_x, arrow_top_y + 4, arrow_center_x, arrow_bottom_y,
                          BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
            Paint_DrawLine(arrow_center_x + 1, arrow_top_y + 4, arrow_center_x + 1, arrow_bottom_y,
                          BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        } else if (dir < 0) {
            // Below green range -> arrow DOWN
            // Thicker shaft: 3 pixels wide, centered in arrowhead
            Paint_DrawLine(arrow_center_x - 1, arrow_top_y, arrow_center_x - 1, arrow_bottom_y - 4,
                          BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
            Paint_DrawLine(arrow_center_x, arrow_top_y, arrow_center_x, arrow_bottom_y - 4,
                          BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
            Paint_DrawLine(arrow_center_x + 1, arrow_top_y, arrow_center_x + 1, arrow_bottom_y - 4,
                          BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
            // Bolder arrowhead: triangle at bottom (4 rows for better visibility)
            // Fourth row from bottom: 7 pixels
            Paint_DrawLine(arrow_center_x - 3, arrow_bottom_y - 3, arrow_center_x + 3, arrow_bottom_y - 3,
                          BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
            // Third row from bottom: 5 pixels
            Paint_DrawLine(arrow_center_x - 2, arrow_bottom_y - 2, arrow_center_x + 2, arrow_bottom_y - 2,
                          BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
            // Second row from bottom: 3 pixels
            Paint_DrawLine(arrow_center_x - 1, arrow_bottom_y - 1, arrow_center_x + 1, arrow_bottom_y - 1,
                          BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
            // Bottom point
            Paint_DrawPoint(arrow_center_x, arrow_bottom_y, BLACK, DOT_PIXEL_1X1, DOT_STYLE_DFT);
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
                values.temp_outdoor = isValidRange(temp, -40, 80) ? temp : -1000;
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
    
    // Update metrics to get current uptime
    updateMetrics();
    
    // Determine which sensor to use for temperature and humidity based on uptime
    // First 6 minutes (360 seconds): use BME680
    // After 5 minutes: use SCD4x
    bool use_bme680_for_temp_hum = (system_metrics.uptime_sec < 360);
    
    // Indoor CO2 with validation (300-5000 ppm) - always from SCD4x
    if (data.containsKey("SCD4x")) {
        auto scd = data["SCD4x"];
        if (scd.containsKey("co2")) {
            float co2 = scd["co2"]["value"].as<float>();
            values.co2 = isValidRange(co2, 300, 5000) ? co2 : -1;
        }
        
        // SCD4x also provides temperature and humidity - use after 5 minutes
        if (!use_bme680_for_temp_hum) {
            if (scd.containsKey("temperature")) {
                float temp = scd["temperature"]["value"].as<float>();
                values.temp_indoor = isValidRange(temp, -40, 80) ? temp : -1000;
            }
            if (scd.containsKey("humidity")) {
                float hum = scd["humidity"]["value"].as<float>();
                values.hum_indoor = isValidRange(hum, 0, 100) ? hum : -1;
            }
        }
    }
    
    // Indoor environment with validation
    // BME680: use for temp/humidity during first 5 minutes, always use for pressure
    if (data.containsKey("BME680")) {
        auto bme = data["BME680"];
        if (use_bme680_for_temp_hum) {
            // First 6 minutes: use BME680 for temperature and humidity
            if (bme.containsKey("temperature")) {
                float temp = bme["temperature"]["value"].as<float>();
                values.temp_indoor = isValidRange(temp, -40, 80) ? temp : -1000;
            }
            if (bme.containsKey("humidity")) {
                float hum = bme["humidity"]["value"].as<float>();
                values.hum_indoor = isValidRange(hum, 0, 100) ? hum : -1;
            }
        }
        // Always use BME680 for pressure (SCD4x doesn't provide pressure)
        if (bme.containsKey("pressure")) {
            float press = bme["pressure"]["value"].as<float>() * 0.0075;
            values.press_indoor = isValidRange(press, 500, 1000) ? press : -1;
        }
    }
}

// Draw the full main screen with Urban 2-subcolumn layout
void drawMainScreen(UBYTE *BlackImage, const String &jsonString, const String &device_ip, const String &insight_robonomics_address, const String &urban_robonomics_address) {
    main_screen_values_t values;
    _parseJsonToStruct(jsonString, values);

    // Clear screen first to remove any white lines
    Paint_Clear(WHITE);
    
    // === HEADER: left icon (current page), center time, right date ===
    struct tm timeinfo; 
    const uint16_t header_top_y = 6;
    const uint16_t header_row_height = Font16.Height + 2; 
    uint16_t       header_bottom_border_y = header_top_y + header_row_height + 2;

    if (getLocalTime(&timeinfo)) {
        char date_buf[12], time_buf[8];
        strftime(date_buf, sizeof(date_buf), "%m/%d/%Y", &timeinfo);
        strftime(time_buf, sizeof(time_buf), "%H:%M",    &timeinfo);

        // Left: current page icon 
        const uint16_t header_icon_size = 15;
        const uint16_t header_icon_x    = 4;
        const uint16_t header_icon_y    = header_top_y;
        Paint_DrawImage(home_nav_15x15, header_icon_x, header_icon_y, header_icon_size, header_icon_size);

        // Center: time in bold (same display font as rest of UI)
        int time_width = (int)Paint_GetStringWidth_Display(time_buf, &Font16, &font_16_cyrillic, &font_16_ascii);
        int time_x = (DISPLAY_WIDTH - time_width) / 2;
        int time_y = header_top_y;
        Paint_DrawString_Display(time_x, time_y, time_buf, &Font16, &font_16_cyrillic, &font_16_ascii, WHITE, BLACK);

        // Right: date, smaller but still bold-ish (same display font as rest of UI)
        int date_width = (int)Paint_GetStringWidth_Display(date_buf, &Font12, &font_12_cyrillic, &font_12_ascii);
        const int right_margin = 4;
        int date_x = DISPLAY_WIDTH - right_margin - date_width;
        int date_y = header_top_y + 2;
        Paint_DrawString_Display(date_x,     date_y, date_buf, &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
        Paint_DrawString_Display(date_x + 1, date_y, date_buf, &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
    }

    // Draw bottom border for header 
    Paint_DrawLine(0, header_bottom_border_y, DISPLAY_WIDTH, header_bottom_border_y, 
                   BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

    // Section headers row (below header)
    uint16_t y_start = header_bottom_border_y + 10;

    // Save right sidebar for vertical navigation icons (approx 26px)
    const uint16_t nav_sidebar_width = 26;
    uint16_t usable_width = (DISPLAY_WIDTH > nav_sidebar_width) ? (DISPLAY_WIDTH - nav_sidebar_width) : DISPLAY_WIDTH;

    // Split content into two equal columns: Urban (left) and Insight (right)
    uint16_t urban_width   = usable_width / 2;
    uint16_t insight_width = usable_width - urban_width;
    const uint16_t column_right_margin = 6;  /* same gap from right edge in both columns */

    uint16_t divider_x = urban_width + 3;
    Paint_DrawLine(divider_x, y_start - 4, divider_x, DISPLAY_HEIGHT - 4,
                   BLACK, DOT_PIXEL_1X1, LINE_STYLE_DOTTED);

    uint16_t value_spacing = 26; 

    // === URBAN SECTION HEADER (left column) ===
    const uint16_t header_icon_size        = 30;
    const uint16_t header_icon_text_offset = 4;
    const char*    urban_title             = INTL_DISP_MAIN_URBAN;

    uint16_t subheader_top_y     = (y_start > 2) ? (y_start - 2) : y_start;
    uint16_t urban_header_icon_x = 0;
    uint16_t urban_header_text_x = urban_header_icon_x + header_icon_size + header_icon_text_offset;
    uint16_t urban_text_y        = subheader_top_y + (header_icon_size - Font16.Height) / 2;

    Paint_DrawImage(urban_30x30, urban_header_icon_x, subheader_top_y - 2, header_icon_size, header_icon_size);
    Paint_DrawString_Display(urban_header_text_x, urban_text_y, urban_title, &Font16, &font_16_cyrillic, &font_16_ascii, WHITE, BLACK);

    // WiFi status icon for Urban
    bool urban_wifi_ok = (values.ip_address.length() > 0);
    const uint16_t urban_wifi_icon_size = 20;
    const uint16_t urban_wifi_margin    = 4;
    int urban_title_width = (int)Paint_GetStringWidth_Display(urban_title, &Font16, &font_16_cyrillic, &font_16_ascii);
    uint16_t urban_wifi_x = urban_header_text_x + urban_title_width + urban_wifi_margin;
    uint16_t urban_wifi_y = subheader_top_y + (header_icon_size - urban_wifi_icon_size) / 2;
    const unsigned char* urban_wifi_icon = urban_wifi_ok ? wifi_20x20 : wifi_x_20x20;
    Paint_DrawImage(urban_wifi_icon, urban_wifi_x, urban_wifi_y, urban_wifi_icon_size, urban_wifi_icon_size);

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
    
    // Make subheaders a bit more narrow vertically
    uint16_t subheader_border_y = subheader_top_y + header_icon_size + 2;

    uint16_t urban_y = subheader_border_y + 12; // bigger gap from subheader to first measure row

    // Urban single column 
    int temp_out_dir  = tempDangerDirection(values.temp_outdoor);
    int hum_out_dir   = humidityDangerDirection(values.hum_outdoor);
    int press_out_dir = pressureDangerDirection(values.press_outdoor);
    int pm_dir        = pmDangerDirection(values.pm10, values.pm25);
    int noise_dir     = noiseDangerDirection(values.noise_max); // use max for direction

    uint16_t urban_x_start   = 8;
    uint16_t urban_col_right = urban_width - column_right_margin; 
    drawValue(INTL_DISP_TEMPERATURE, values.temp_outdoor, 1, wi_thermometer_cropped_20x20, "C",
              20, urban_x_start, urban_y,                 urban_col_right, 5, false, SOURCE_URBAN, (temp_out_dir != 0),  temp_out_dir);
    drawValue(INTL_DISP_HUMIDITY,    values.hum_outdoor,  0, wi_humidity_cropped_20x20,     "%",
              20, urban_x_start, urban_y + value_spacing, urban_col_right, 5, false, SOURCE_URBAN, (hum_out_dir != 0),   hum_out_dir);
    drawValue(INTL_DISP_PRESSURE,    values.press_outdoor,0, pressure_20x20,               "mmHg",
              20, urban_x_start, urban_y + 2 * value_spacing, urban_col_right, 2, false, SOURCE_URBAN, (press_out_dir != 0), press_out_dir);
    drawValue("PM10",        values.pm10,      1, air_filter_20x20,    "ppm",
              20, urban_x_start, urban_y + 3 * value_spacing, urban_col_right, 5, false, SOURCE_URBAN, (pm_dir != 0),    pm_dir);
    drawValue("PM2.5",       values.pm25,      1, air_pollution_20x20, "ppm",
              20, urban_x_start, urban_y + 4 * value_spacing, urban_col_right, 5, false, SOURCE_URBAN, (pm_dir != 0),    pm_dir);
    drawValue(INTL_DISP_NOISE_MAX,  values.noise_max, 0, ear_hearing_20x20,   "dB",
              20, urban_x_start, urban_y + 5 * value_spacing, urban_col_right, 5, false, SOURCE_URBAN, (noise_dir != 0), noise_dir);
    drawValue(INTL_DISP_NOISE_AVG,  values.noise_avg, 0, ear_hearing_20x20,   "dB",
              20, urban_x_start, urban_y + 6 * value_spacing, urban_col_right, 5, false, SOURCE_URBAN, (noise_dir != 0), noise_dir);

    // === INSIGHT DEVICE SECTION HEADER (right column) ===
    const char* insight_title = INTL_DISP_MAIN_INSIGHT;
    uint16_t insight_column_start_x = urban_width;
    uint16_t insight_header_icon_x  = insight_column_start_x + 8;
    uint16_t insight_header_text_x  = insight_header_icon_x + header_icon_size + header_icon_text_offset;
    uint16_t insight_text_y         = subheader_top_y + (header_icon_size - Font16.Height) / 2;

    Paint_DrawImage(insight_30x30, insight_header_icon_x, subheader_top_y - 2, header_icon_size, header_icon_size);
    Paint_DrawString_Display(insight_header_text_x, insight_text_y, insight_title, &Font16, &font_16_cyrillic, &font_16_ascii, WHITE, BLACK);

    // WiFi status icon for Insight
    bool insight_online = (device_ip.length() > 0);
    const uint16_t insight_wifi_icon_size = 20;
    const uint16_t insight_wifi_margin    = 4;
    int insight_title_width = (int)Paint_GetStringWidth_Display(insight_title, &Font16, &font_16_cyrillic, &font_16_ascii);
    uint16_t insight_wifi_x = insight_header_text_x + insight_title_width + insight_wifi_margin;
    uint16_t insight_wifi_y = subheader_top_y + (header_icon_size - insight_wifi_icon_size) / 2;
    const unsigned char* insight_wifi_icon = insight_online ? wifi_20x20 : wifi_x_20x20;
    Paint_DrawImage(insight_wifi_icon, insight_wifi_x, insight_wifi_y, insight_wifi_icon_size, insight_wifi_icon_size);

    // QR code with sensors.social map link for Insight (on the far right)
    if (insight_online && insight_robonomics_address.length() > 0) {
        QRCode mainScreenQR;
        // Compact URL with sensor parameter (fits in smaller QR)
        char qr_data[128];
        snprintf(qr_data, sizeof(qr_data), "sensors.social/?sensor=%s", insight_robonomics_address.c_str());

        // Version 5 for ~39px QR
        uint8_t qr_version = 5;
        uint8_t qrcodeData[qrcode_getBufferSize(qr_version)];
        qrcode_initText(&mainScreenQR, qrcodeData, qr_version, ECC_LOW, qr_data);

        int scale_factor = 1;
        int quiet_zone   = 1;
        int total_width  = mainScreenQR.size * scale_factor + 2 * quiet_zone;
        int total_height = mainScreenQR.size * scale_factor + 2 * quiet_zone;
        int qr_bitmap_width_bytes = (total_width + 7) / 8;
        int qr_bitmap_size        = total_height * qr_bitmap_width_bytes;

        unsigned char *qr_bitmap_scaled = (unsigned char*)malloc(qr_bitmap_size);
        if (qr_bitmap_scaled) {
            memset(qr_bitmap_scaled, 0x00, qr_bitmap_size); // White background

            for (uint8_t qr_y = 0; qr_y < mainScreenQR.size; qr_y++) {
                for (uint8_t qr_x = 0; qr_x < mainScreenQR.size; qr_x++) {
                    if (qrcode_getModule(&mainScreenQR, qr_x, qr_y)) {
                        for (int sy = 0; sy < scale_factor; sy++) {
                            for (int sx = 0; sx < scale_factor; sx++) {
                                int pixel_x = quiet_zone + qr_x * scale_factor + sx;
                                int pixel_y = quiet_zone + qr_y * scale_factor + sy;
                                if (pixel_x >= 0 && pixel_x < total_width &&
                                    pixel_y >= 0 && pixel_y < total_height) {
                                    int byte_index = pixel_y * qr_bitmap_width_bytes + (pixel_x / 8);
                                    qr_bitmap_scaled[byte_index] |= (0x80 >> (pixel_x % 8));
                                }
                            }
                        }
                    }
                }
            }

            // Place QR on the far right of the Insight header band
            int qr_x = usable_width - total_width - 4;
            int qr_y = subheader_top_y + (header_icon_size - total_height) / 2 - 1;
            if (qr_y < 0) qr_y = 0;
            Paint_DrawImage(qr_bitmap_scaled, qr_x, qr_y, total_width, total_height);

            free(qr_bitmap_scaled);
        }
    }

    // QR code with sensors.social map link for Urban
    if (urban_robonomics_address.length() > 0) {
        QRCode urbanQR;
        // Compact URL with sensor parameter (fits in smaller QR)
        char qr_data_urban[128];
        snprintf(qr_data_urban, sizeof(qr_data_urban), "sensors.social/?sensor=%s", urban_robonomics_address.c_str());

        // Version 5 for ~39px QR
        uint8_t qr_version_u = 5;
        uint8_t qrcodeData_u[qrcode_getBufferSize(qr_version_u)];
        qrcode_initText(&urbanQR, qrcodeData_u, qr_version_u, ECC_LOW, qr_data_urban);

        int scale_factor_u = 1;
        int quiet_zone_u   = 1;
        int total_width_u  = urbanQR.size * scale_factor_u + 2 * quiet_zone_u;
        int total_height_u = urbanQR.size * scale_factor_u + 2 * quiet_zone_u;
        int qr_bitmap_width_bytes_u = (total_width_u + 7) / 8;
        int qr_bitmap_size_u        = total_height_u * qr_bitmap_width_bytes_u;

        unsigned char *qr_bitmap_scaled_u = (unsigned char*)malloc(qr_bitmap_size_u);
        if (qr_bitmap_scaled_u) {
            memset(qr_bitmap_scaled_u, 0x00, qr_bitmap_size_u);

            for (uint8_t qr_y = 0; qr_y < urbanQR.size; qr_y++) {
                for (uint8_t qr_x = 0; qr_x < urbanQR.size; qr_x++) {
                    if (qrcode_getModule(&urbanQR, qr_x, qr_y)) {
                        for (int sy = 0; sy < scale_factor_u; sy++) {
                            for (int sx = 0; sx < scale_factor_u; sx++) {
                                int pixel_x = quiet_zone_u + qr_x * scale_factor_u + sx;
                                int pixel_y = quiet_zone_u + qr_y * scale_factor_u + sy;
                                if (pixel_x >= 0 && pixel_x < total_width_u &&
                                    pixel_y >= 0 && pixel_y < total_height_u) {
                                    int byte_index = pixel_y * qr_bitmap_width_bytes_u + (pixel_x / 8);
                                    qr_bitmap_scaled_u[byte_index] |= (0x80 >> (pixel_x % 8));
                                }
                            }
                        }
                    }
                }
            }

            // Place Urban QR on the far right of the Urban column
            int qr_x_u = urban_width - total_width_u - 4;
            int qr_y_u = subheader_top_y + (header_icon_size - total_height_u) / 2 - 1;
            if (qr_y_u < 0) qr_y_u = 0;
            Paint_DrawImage(qr_bitmap_scaled_u, qr_x_u, qr_y_u, total_width_u, total_height_u);

            free(qr_bitmap_scaled_u);
        }
    }
    
    uint16_t insight_y = urban_y;
    
    // Insight device data 
    int temp_in_dir  = tempDangerDirection(values.temp_indoor);
    int hum_in_dir   = humidityDangerDirection(values.hum_indoor);
    int press_in_dir = pressureDangerDirection(values.press_indoor);
    int co2_dir      = co2DangerDirection(values.co2);

    uint16_t insight_x_start  = insight_column_start_x + 8;
    uint16_t insight_col_right = usable_width - column_right_margin;
    drawValue(INTL_DISP_TEMPERATURE, values.temp_indoor, 1, house_thermometer_20x20, "C",
              20, insight_x_start, insight_y,                 insight_col_right, 2, false, SOURCE_INSIGHT, (temp_in_dir != 0),  temp_in_dir);
    drawValue(INTL_DISP_HUMIDITY,    values.hum_indoor,  0, wi_humidity_cropped_20x20,     "%",
              20, insight_x_start, insight_y + value_spacing, insight_col_right, 2, false, SOURCE_INSIGHT, (hum_in_dir != 0),   hum_in_dir);
    drawValue(INTL_DISP_PRESSURE, values.press_indoor,0, pressure_20x20,               "mmHg",
              20, insight_x_start, insight_y + 2 * value_spacing, insight_col_right, 2, false, SOURCE_INSIGHT, (press_in_dir != 0), press_in_dir);
    drawValue(INTL_CO2,         values.co2,         0, co2_svgrepo_com_20x20,        "ppm",
              20, insight_x_start, insight_y + 3 * value_spacing, insight_col_right, 5, false, SOURCE_INSIGHT, (co2_dir != 0),      co2_dir);
}

#endif