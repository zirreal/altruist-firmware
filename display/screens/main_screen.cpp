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
#include "../icons/icons/32x32/info_32x32.h"
#include "../icons/icons/28x28/wifi_28x28.h"
#include "../icons/icons/28x28/wifi_x_28x28.h"
#include "../icons/icons/32x32/urban_32x32.h"
#include "../icons/icons/32x32/insight_32x32.h"
#include "../icons/icons/16x16/warning_new_16x16.h"
#include "../icons/icons/34x32/wi_thermometer_cropped_34x32.h"
#include "../icons/icons/34x34/wi_humidity_cropped_34x34.h"
#include "../icons/icons/34x34/dust_34x34.h"
#include "../icons/icons/34x34/ear_hearing_34x34.h"
#include "../icons/icons/32x32/pressure_32x32.h"
#include "../icons/icons/32x32/co2_svgrepo_com_32x32.h"
#include "../../defines.h"
#include "../utils.h"
#include "../../utils.h"
#include "../../config_manager/config_helpers.h"
#include "display_common.h"
#include "../../intl.h"
#include "../paint_driver/fonts/fonts.h"
#include <qrcode.h>
#include <math.h>

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

// Extract main screen values directly from shared sensors data.
// This avoids serialize->deserialize on every refresh.
void extractMainScreenValues(const JsonDocument &doc, main_screen_values_t &values) {
    JsonObjectConst data = doc.as<JsonObjectConst>();
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

static bool isTempNoData(float value) {
    const float NO_TEMP_DATA_SENTINEL = -1000.0f;
    const float EPSILON = 0.1f;
    return (value < (NO_TEMP_DATA_SENTINEL + EPSILON) && value > (NO_TEMP_DATA_SENTINEL - EPSILON));
}

static bool isGenericNoData(float value) {
    const float NO_DATA_SENTINEL = -1.0f;
    const float EPSILON = 0.1f;
    return (value < (NO_DATA_SENTINEL + EPSILON) && value > (NO_DATA_SENTINEL - EPSILON));
}

static void formatMetricValue(char *out, size_t out_size, float value, uint8_t precision, bool is_temperature) {
    bool no_data = is_temperature ? isTempNoData(value) : isGenericNoData(value);
    if (no_data) {
        snprintf(out, out_size, "%s", INTL_DISP_NO_DATA);
        return;
    }
    stringFromFloat(out, value, precision);
}

static float calculateDewPointC(float temperature_c, float humidity_percent) {
    if (isTempNoData(temperature_c) || isGenericNoData(humidity_percent) || humidity_percent <= 0.0f) {
        return -1000.0f;
    }
    // Clamp to physical range for stable output if sensor spikes.
    if (humidity_percent > 100.0f) humidity_percent = 100.0f;
    const float a = 17.62f;
    const float b = 243.12f;
    float gamma = logf(humidity_percent / 100.0f) + (a * temperature_c) / (b + temperature_c);
    return (b * gamma) / (a - gamma);
}

// Returns true if GPS coords are set and not (0,0) — used to avoid showing QRs with wrong location
static bool hasValidGpsCoords() {
    if (cfg::coords_gps == nullptr || strlen(cfg::coords_gps) == 0) return false;
    double lat = 0.0, lon = 0.0;
    if (sscanf(cfg::coords_gps, "%lf,%lf", &lat, &lon) != 2) return false;
    return (fabs(lat) > 1e-6 || fabs(lon) > 1e-6);
}

static int drawSensorQR(const String &sensor_address, int x, int y) {
    if (sensor_address.length() == 0) return 0;

    QRCode qr;
    char qr_data[128];
    snprintf(qr_data, sizeof(qr_data), "sensors.social/?sensor=%s", sensor_address.c_str());

    const uint8_t qr_version = 5; // reliable density for sensor URLs
    uint8_t qrcodeData[qrcode_getBufferSize(qr_version)];
    qrcode_initText(&qr, qrcodeData, qr_version, ECC_LOW, qr_data);

    const int scale_factor = 1;
    const int quiet_zone = 1;
    const int total_width = qr.size * scale_factor + 2 * quiet_zone;
    const int total_height = qr.size * scale_factor + 2 * quiet_zone;
    const int qr_bitmap_width_bytes = (total_width + 7) / 8;
    const int qr_bitmap_size = total_height * qr_bitmap_width_bytes;

    unsigned char *qr_bitmap_scaled = (unsigned char*)malloc(qr_bitmap_size);
    if (!qr_bitmap_scaled) return 0;

    memset(qr_bitmap_scaled, 0x00, qr_bitmap_size);

    for (uint8_t qr_y = 0; qr_y < qr.size; qr_y++) {
        for (uint8_t qr_x = 0; qr_x < qr.size; qr_x++) {
            if (!qrcode_getModule(&qr, qr_x, qr_y)) continue;
            for (int sy = 0; sy < scale_factor; sy++) {
                for (int sx = 0; sx < scale_factor; sx++) {
                    int pixel_x = quiet_zone + qr_x * scale_factor + sx;
                    int pixel_y = quiet_zone + qr_y * scale_factor + sy;
                    if (pixel_x >= 0 && pixel_x < total_width && pixel_y >= 0 && pixel_y < total_height) {
                        int byte_index = pixel_y * qr_bitmap_width_bytes + (pixel_x / 8);
                        qr_bitmap_scaled[byte_index] |= (0x80 >> (pixel_x % 8));
                    }
                }
            }
        }
    }

    Paint_DrawImage(qr_bitmap_scaled, x, y, total_width, total_height);
    free(qr_bitmap_scaled);
    return total_height;
}

static void drawWarningGlyph(uint16_t x, uint16_t y, int danger_direction) {
    // Slightly larger warning marker for readability.
    const uint16_t tri_w = 18;
    const uint16_t tri_h = 16;
    uint16_t mid_x = x + tri_w / 2;
    for (uint16_t row = 0; row < tri_h; row++) {
        uint16_t row_w = 1 + (row * tri_w) / tri_h;
        uint16_t left = mid_x - row_w / 2;
        uint16_t right = mid_x + (row_w - 1) / 2;
        Paint_DrawLine(left, y + row, right, y + row, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    }
    Paint_DrawLine(mid_x, y + 6, mid_x, y + 10, WHITE, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    Paint_DrawPoint(mid_x, y + 12, WHITE, DOT_PIXEL_1X1, DOT_STYLE_DFT);

    uint16_t arrow_x = x + tri_w + 2;
    uint16_t arrow_mid = y + 8;
    if (danger_direction > 0) {
        Paint_DrawLine(arrow_x + 2, y + 1, arrow_x + 2, y + 15, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        Paint_DrawLine(arrow_x + 1, y + 2, arrow_x + 2, y + 1, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        Paint_DrawLine(arrow_x + 3, y + 2, arrow_x + 2, y + 1, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        Paint_DrawLine(arrow_x, y + 3, arrow_x + 2, y + 1, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        Paint_DrawLine(arrow_x + 4, y + 3, arrow_x + 2, y + 1, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    } else if (danger_direction < 0) {
        Paint_DrawLine(arrow_x + 2, y + 1, arrow_x + 2, y + 15, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        Paint_DrawLine(arrow_x + 1, y + 14, arrow_x + 2, y + 15, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        Paint_DrawLine(arrow_x + 3, y + 14, arrow_x + 2, y + 15, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        Paint_DrawLine(arrow_x, y + 13, arrow_x + 2, y + 15, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        Paint_DrawLine(arrow_x + 4, y + 13, arrow_x + 2, y + 15, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    } else {
        Paint_DrawPoint(arrow_x + 2, arrow_mid, BLACK, DOT_PIXEL_1X1, DOT_STYLE_DFT);
    }
}

static void drawMetricCard(const char *title,
                           const char *value_text,
                           const char *units,
                           uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                           const char *secondary_title = nullptr,
                           bool bold_value = false,
                           int danger_direction = 0,
                           const unsigned char *icon = nullptr,
                           uint16_t icon_w = 0,
                           uint16_t icon_h = 0) {
    if (w < 24 || h < 24) return;
    Paint_DrawRectangle(x, y, x + w, y + h, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);

    const uint16_t pad_x = 8;
    const uint16_t title_y = y + 6;
    uint16_t title_x = x + pad_x;
    if (icon != nullptr && icon_w > 0 && icon_h > 0) {
        uint16_t icon_x = x + 5;
        uint16_t icon_y = y + 4 + ((Font12.Height > icon_h) ? ((Font12.Height - icon_h) / 2) : 0);
        Paint_DrawImage(icon, icon_x, icon_y, icon_w, icon_h);
        title_x = icon_x + icon_w + 4;
    }
    Paint_DrawString_Display(title_x, title_y, title, &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);

    if (secondary_title != nullptr) {
        uint16_t sec_width = Paint_GetStringWidth_Display(secondary_title, &Font12, &font_12_cyrillic, &font_12_ascii);
        uint16_t sec_x = (x + w > sec_width + pad_x) ? (x + w - sec_width - pad_x) : (x + pad_x);
        Paint_DrawString_Display(sec_x, title_y, secondary_title, &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
    }

    uint16_t value_width = Paint_GetStringWidth_Display(value_text, &Font20, &font_20_cyrillic, &font_20_ascii);
    bool has_units = (units != nullptr && units[0] != '\0');
    uint16_t units_width = has_units ? Paint_GetStringWidth_Display(units, &Font12, &font_12_cyrillic, &font_12_ascii) : 0;
    const uint16_t units_gap = has_units ? 3 : 0;
    uint16_t total_width = value_width + units_gap + units_width;

    uint16_t value_x = (x + w > total_width + pad_x) ? (x + w - total_width - pad_x) : (x + pad_x);
    uint16_t value_y = y + h - Font20.Height - 5;
    uint16_t units_x = value_x + value_width + units_gap;
    uint16_t units_y = value_y + (Font20.Height > Font12.Height ? (Font20.Height - Font12.Height) : 0);

    if (bold_value) {
        Paint_DrawString_Display(value_x, value_y, value_text, &Font20, &font_20_cyrillic, &font_20_ascii, WHITE, BLACK);
        Paint_DrawString_Display(value_x + 1, value_y, value_text, &Font20, &font_20_cyrillic, &font_20_ascii, WHITE, BLACK);
    } else {
        Paint_DrawString_Display(value_x, value_y, value_text, &Font20, &font_20_cyrillic, &font_20_ascii, WHITE, BLACK);
    }

    if (has_units) {
        Paint_DrawString_Display(units_x, units_y, units, &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
    }

    if (danger_direction != 0) {
        uint16_t icon_x = (x + w > 30) ? (x + w - 28) : x;
        drawWarningGlyph(icon_x, y + 8, danger_direction);
    }
}

static void drawWarningLevelIcon(uint16_t x, uint16_t y, int danger_direction) {
    if (danger_direction != 0) {
        Paint_DrawImage(warning_new_16x16, x, y, 16, 16);
    }
}

static uint16_t drawNumberWithUnit(uint16_t x, uint16_t y, const char *value, const char *unit) {
    // Use 22px glyph font for numeric values.
    sFONT* value_font_en = &Font24; // EN uses glyph overrides when provided
    const Font* value_font_ru = &font_22_cyrillic;
    const Font* value_font_ascii = &font_22_ascii;
    uint16_t value_w = Paint_GetStringWidth_Display(value, value_font_en, value_font_ru, value_font_ascii);
    Paint_DrawString_Display(x, y, value, value_font_en, value_font_ru, value_font_ascii, WHITE, BLACK);

    // Make decimal point more noticeable on e-ink.
    // Some glyph fonts render '.' very lightly; redraw '.' on top (slightly offset) to darken it.
    const char* dot = strchr(value, '.');
    if (dot) {
        char prefix[16];
        size_t n = (size_t)(dot - value);
        if (n >= sizeof(prefix)) n = sizeof(prefix) - 1;
        memcpy(prefix, value, n);
        prefix[n] = '\0';
        const uint16_t prefix_w = Paint_GetStringWidth_Display(prefix, value_font_en, value_font_ru, value_font_ascii);
        const uint16_t dx = x + prefix_w;
        Paint_DrawString_Display(dx,     y, ".", value_font_en, value_font_ru, value_font_ascii, WHITE, BLACK);
        Paint_DrawString_Display(dx + 1, y, ".", value_font_en, value_font_ru, value_font_ascii, WHITE, BLACK);
    }

    if (unit != nullptr && unit[0] != '\0') {
        const uint16_t value_h = value_font_ascii->line_height ? value_font_ascii->line_height : Font16.Height;
        uint16_t unit_y = y + ((value_h > Font12.Height) ? (value_h - Font12.Height) : 0);
        Paint_DrawString_Display(x + value_w + 2, unit_y, unit, &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
        return x + value_w + 2 + Paint_GetStringWidth_Display(unit, &Font12, &font_12_cyrillic, &font_12_ascii);
    }
    return x + value_w;
}

static void drawPairNumbersWithUnits(uint16_t x, uint16_t y,
                                     const char *v1, const char *u1,
                                     const char *v2, const char *u2) {
    uint16_t cur_x = x;
    cur_x = drawNumberWithUnit(cur_x, y, v1, u1);
    // Symmetric spacing around separator so it doesn't look one-sided.
    const uint16_t sep_gap_left = 6;
    const uint16_t sep_gap_right = 6;
    cur_x += sep_gap_left;
    // Draw separator slightly smaller than the numbers (18px).
    sFONT* sep_font_en = &Font24; // EN uses glyph overrides when provided
    const Font* sep_font_ru = &font_18_cyrillic;
    const Font* sep_font_ascii = &font_18_ascii;
    const char *sep = "|";
    const uint16_t sep_y = y + 2; // separator slightly lower
    Paint_DrawString_Display(cur_x, sep_y, sep, sep_font_en, sep_font_ru, sep_font_ascii, WHITE, BLACK);
    // Advance by the actual separator width for clean spacing.
    const uint16_t sep_w = Paint_GetStringWidth_Display(sep, sep_font_en, sep_font_ru, sep_font_ascii);
    cur_x += sep_w + sep_gap_right;
    drawNumberWithUnit(cur_x, y, v2, u2);
}

// Draw the full main screen with card-based main content layout
void drawMainScreen(UBYTE *BlackImage, const main_screen_values_t &values, const String &device_ip, const String &insight_robonomics_address, const String &urban_robonomics_address) {

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

        // Right: date (same font as time for consistent header look)
        int date_width = (int)Paint_GetStringWidth_Display(date_buf, &Font16, &font_16_cyrillic, &font_16_ascii);
        const int right_margin = 4;
        int date_x = DISPLAY_WIDTH - right_margin - date_width;
        int date_y = header_top_y;
        Paint_DrawString_Display(date_x, date_y, date_buf, &Font16, &font_16_cyrillic, &font_16_ascii, WHITE, BLACK);
    }

    // Draw bottom border for header 
    Paint_DrawLine(0, header_bottom_border_y, DISPLAY_WIDTH, header_bottom_border_y, 
                   BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

    // Keep space for right sidebar (navigation icons).
    const uint16_t nav_sidebar_width = 26;
    const uint16_t content_left = 2;
    const uint16_t content_right = (DISPLAY_WIDTH > nav_sidebar_width + 2) ? (DISPLAY_WIDTH - nav_sidebar_width - 2) : (DISPLAY_WIDTH - 1);
    const uint16_t content_width = (content_right > content_left) ? (content_right - content_left) : 0;
    const uint16_t body_top = header_bottom_border_y + 4;

    // Top strip: QR codes left/right + combined title in the middle.
    // Hide Insight QR when GPS coords are (0,0) — we can't verify Urban's coords (different device)
    bool show_insight_qr = hasValidGpsCoords();
    const uint16_t left_qr_x = content_left + 2;
    const uint16_t right_qr_x = (content_right > 41) ? (content_right - 41) : (content_left + 2);
    const uint16_t qr_pad_top_y = 2;
    const uint16_t qr_pad_bottom_y = 6;       // bigger bottom padding under QRs
    const uint16_t top_qr_y = body_top + qr_pad_top_y;
    int left_qr_size = drawSensorQR(urban_robonomics_address, left_qr_x, top_qr_y);
    int right_qr_size = show_insight_qr ? drawSensorQR(insight_robonomics_address, right_qr_x, top_qr_y) : 0;
    bool urban_online = (values.ip_address.length() > 0);
    bool insight_online = (device_ip.length() > 0);
    const int16_t left_wifi_nudge_x = -3;  // keep Urban side as-is
    const int16_t right_wifi_nudge_x = +1; // move ONLY Insight-side WiFi to the right
    uint16_t left_wifi_x = (uint16_t)((int32_t)left_qr_x + (left_qr_size > 0 ? (int32_t)left_qr_size + 8 : 43) + left_wifi_nudge_x);
    if (left_wifi_x < content_left) left_wifi_x = content_left;
    uint16_t right_wifi_x = (uint16_t)((int32_t)((right_qr_x > 36) ? (right_qr_x - 36) : content_left) + right_wifi_nudge_x);
    if (right_wifi_x < content_left) right_wifi_x = content_left;
    uint16_t wifi_y = top_qr_y + 6;
    Paint_DrawImage(urban_online ? wifi_28x28 : wifi_x_28x28, left_wifi_x, wifi_y, 28, 28);
    Paint_DrawImage(insight_online ? wifi_28x28 : wifi_x_28x28, right_wifi_x, wifi_y, 28, 28);
    const uint16_t source_icon_gap = 4;
    const uint16_t insight_icon_gap = 0; // as close as possible to its WiFi icon
    const uint16_t source_icon_size = 32;
    uint16_t source_icon_y = (wifi_y >= 2) ? (wifi_y - 2) : 0; // visually center 32px icon with 28px WiFi
    const int16_t urban_icon_nudge_x = -3; // move Urban icon a bit to the left
    uint16_t urban_icon_x = (uint16_t)((int32_t)left_wifi_x + 28 + source_icon_gap + urban_icon_nudge_x);
    if (urban_icon_x < content_left) urban_icon_x = content_left;
    uint16_t insight_icon_x = (right_wifi_x > (source_icon_size + insight_icon_gap)) ? (right_wifi_x - (source_icon_size + insight_icon_gap)) : content_left;
    uint16_t max_icon_x = (content_right > source_icon_size) ? (content_right - source_icon_size) : content_left;
    if (urban_icon_x > max_icon_x) urban_icon_x = max_icon_x;
    if (insight_icon_x > max_icon_x) insight_icon_x = max_icon_x;
    Paint_DrawImage(urban_32x32, urban_icon_x, source_icon_y, source_icon_size, source_icon_size);
    Paint_DrawImage(insight_32x32, insight_icon_x, source_icon_y, source_icon_size, source_icon_size);

    // Draw "URBAN | INSIGHT" with controlled pixel gaps (match number separator feel).
    const char *title_left = "URBAN";
    const char *title_sep = "|";
    const char *title_right = "INSIGHT";
    const uint16_t title_gap_left = 4;
    const uint16_t title_gap_right = 4;
    const uint16_t w_left = Paint_GetStringWidth_Display(title_left, &Font16, &font_16_cyrillic, &font_16_ascii);
    const uint16_t w_sep = Paint_GetStringWidth_Display(title_sep, &Font16, &font_16_cyrillic, &font_16_ascii);
    const uint16_t w_right = Paint_GetStringWidth_Display(title_right, &Font16, &font_16_cyrillic, &font_16_ascii);
    const uint16_t title_w = w_left + title_gap_left + w_sep + title_gap_right + w_right;
    int title_x = content_left + ((int)content_width - (int)title_w) / 2;
    uint16_t tx = (title_x > 0) ? (uint16_t)title_x : 0;
    uint16_t ty = body_top + 13;
    Paint_DrawString_Display(tx, ty, title_left, &Font16, &font_16_cyrillic, &font_16_ascii, WHITE, BLACK);
    tx += w_left + title_gap_left;
    // Normal separator glyph (no dotted rendering).
    Paint_DrawString_Display(tx, ty, title_sep, &Font16, &font_16_cyrillic, &font_16_ascii, WHITE, BLACK);
    tx += w_sep + title_gap_right;
    Paint_DrawString_Display(tx, ty, title_right, &Font16, &font_16_cyrillic, &font_16_ascii, WHITE, BLACK);

    // Taller header strip for better visual separation.
    const uint16_t qr_h = (left_qr_size > right_qr_size) ? (uint16_t)left_qr_size : (uint16_t)right_qr_size;
    uint16_t top_sep_y = body_top + 44; // fallback
    if (qr_h > 0) {
        // Use asymmetric padding (bigger bottom padding requested).
        top_sep_y = top_qr_y + qr_h + qr_pad_bottom_y;
    }
    // Dotted separator line under the top strip (subheader).
    for (uint16_t x = content_left; x <= content_right; x += 4) {
        uint16_t x1 = x + 1;
        if (x1 > content_right) x1 = content_right;
        Paint_DrawLine(x, top_sep_y, x1, top_sep_y, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    }

    int temp_out_dir  = tempDangerDirection(values.temp_outdoor);
    int temp_in_dir   = tempDangerDirection(values.temp_indoor);
    int hum_out_dir   = humidityDangerDirection(values.hum_outdoor);
    int hum_in_dir    = humidityDangerDirection(values.hum_indoor);
    int press_out_dir = pressureDangerDirection(values.press_outdoor);
    int press_in_dir  = pressureDangerDirection(values.press_indoor);
    int pm_dir        = pmDangerDirection(values.pm10, values.pm25);
    int noise_dir     = noiseDangerDirection(values.noise_max);
    int co2_dir       = co2DangerDirection(values.co2);
    int temp_icon_dir = (temp_out_dir != 0) ? temp_out_dir : temp_in_dir;
    int hum_icon_dir = (hum_out_dir != 0) ? hum_out_dir : hum_in_dir;
    int press_icon_dir = (press_out_dir != 0) ? press_out_dir : press_in_dir;
    int co2_icon_dir = co2_dir;
    int noise_icon_dir = noise_dir;
    int pm_icon_dir = pm_dir;

    char temp_out[16], temp_in[16], hum_out[16], hum_in[16], press_out[16], press_in[16];
    char pm10_str[16], pm25_str[16], noise_avg[12], noise_max[12], co2_str[16];
    const bool footer_test_all_warnings = false; // temporary UI-fit test mode
    formatMetricValue(temp_out, sizeof(temp_out), values.temp_outdoor, 0, true);
    formatMetricValue(temp_in, sizeof(temp_in), values.temp_indoor, 0, true);
    formatMetricValue(hum_out, sizeof(hum_out), values.hum_outdoor, 0, false);
    formatMetricValue(hum_in, sizeof(hum_in), values.hum_indoor, 0, false);
    formatMetricValue(press_out, sizeof(press_out), values.press_outdoor, 0, false);
    formatMetricValue(press_in, sizeof(press_in), values.press_indoor, 0, false);
    formatMetricValue(pm10_str, sizeof(pm10_str), values.pm10, 0, false);
    formatMetricValue(pm25_str, sizeof(pm25_str), values.pm25, 0, false);
    formatMetricValue(noise_avg, sizeof(noise_avg), values.noise_avg, 0, false);
    formatMetricValue(noise_max, sizeof(noise_max), values.noise_max, 0, false);
    formatMetricValue(co2_str, sizeof(co2_str), values.co2, 0, false);

    // UI fit-test: keep real PM strings (no forced PM overrides).

    // Move measurements lower, closer to QR section.
    const uint16_t row_top = top_sep_y + 14;
    const uint16_t row_step = 54;
    const uint16_t left_x = content_left;
    const uint16_t right_col_shift_left = 9;
    const uint16_t right_x = content_left + content_width / 2 - 3 - right_col_shift_left;

    // Left group: shared metrics Urban/Insight.

    Paint_DrawImage(wi_thermometer_cropped_34x32, left_x, row_top + 1, 34, 32);
    String left_temp_label_s = String(INTL_DISP_TEMPERATURE) + " °C";
    const char *left_temp_label = left_temp_label_s.c_str();
    uint16_t left_temp_label_x = left_x + 42;
    Paint_DrawString_Display(left_temp_label_x, row_top, left_temp_label, &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
    uint16_t left_temp_label_w = Paint_GetStringWidth_Display(left_temp_label, &Font12, &font_12_cyrillic, &font_12_ascii);
    drawWarningLevelIcon(left_temp_label_x + left_temp_label_w + 4, row_top - 1, temp_icon_dir);
    drawPairNumbersWithUnits(left_temp_label_x, row_top + 13, temp_out, "", temp_in, "");

    Paint_DrawImage(wi_humidity_cropped_34x34, left_x, row_top + row_step + 1, 34, 34);
    String left_hum_label_s = String(INTL_DISP_HUMIDITY) + " %";
    const char *left_hum_label = left_hum_label_s.c_str();
    uint16_t left_hum_label_x = left_x + 42;
    Paint_DrawString_Display(left_hum_label_x, row_top + row_step, left_hum_label, &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
    uint16_t left_hum_label_w = Paint_GetStringWidth_Display(left_hum_label, &Font12, &font_12_cyrillic, &font_12_ascii);
    drawWarningLevelIcon(left_hum_label_x + left_hum_label_w + 4, row_top + row_step - 1, hum_icon_dir);
    drawPairNumbersWithUnits(left_hum_label_x, row_top + row_step + 13, hum_out, "", hum_in, "");

    Paint_DrawImage(co2_svgrepo_com_32x32, left_x, row_top + 2 * row_step + 1, 32, 32);
    const char *left_co2_label = "CO2 ppm";
    uint16_t left_co2_label_x = left_x + 42;
    Paint_DrawString_Display(left_co2_label_x, row_top + 2 * row_step, left_co2_label, &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
    uint16_t left_co2_label_w = Paint_GetStringWidth_Display(left_co2_label, &Font12, &font_12_cyrillic, &font_12_ascii);
    drawWarningLevelIcon(left_co2_label_x + left_co2_label_w + 4, row_top + 2 * row_step - 1, co2_icon_dir);
    drawNumberWithUnit(left_co2_label_x, row_top + 2 * row_step + 13, co2_str, "");

    // Right group: source-specific metrics.

    Paint_DrawImage(ear_hearing_34x34, right_x, row_top + 1, 34, 34);
    String right_noise_label_s = String(INTL_DISP_NOISE) + " " + String(INTL_DISP_NOISE_AVGMAX_SUFFIX) + " dB";
    const char *right_noise_label = right_noise_label_s.c_str();
    uint16_t right_noise_label_x = right_x + 42;
    // Noise on main screen comes from Urban device/sensors.
    Paint_DrawString_Display(right_noise_label_x, row_top - 10, INTL_DISP_URBAN_ONLY, &Font8, &font_8_cyrillic, &font_8_ascii, WHITE, BLACK);
    Paint_DrawString_Display(right_noise_label_x, row_top, right_noise_label, &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
    uint16_t right_noise_label_w = Paint_GetStringWidth_Display(right_noise_label, &Font12, &font_12_cyrillic, &font_12_ascii);
    drawWarningLevelIcon(right_noise_label_x + right_noise_label_w + 4, row_top - 1, noise_icon_dir);
    drawPairNumbersWithUnits(right_noise_label_x, row_top + 13, noise_avg, "", noise_max, "");

    Paint_DrawImage(dust_34x34, right_x, row_top + row_step + 1, 34, 34);
    const char *right_air_label = "PM10 | PM2.5 ug/m3";
    uint16_t right_air_label_x = right_x + 42;
    // PM on main screen comes from Urban device/sensors.
    Paint_DrawString_Display(right_air_label_x, row_top + row_step - 10, INTL_DISP_URBAN_ONLY, &Font8, &font_8_cyrillic, &font_8_ascii, WHITE, BLACK);
    Paint_DrawString_Display(right_air_label_x, row_top + row_step, right_air_label, &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
    uint16_t right_air_label_w = Paint_GetStringWidth_Display(right_air_label, &Font12, &font_12_cyrillic, &font_12_ascii);
    drawWarningLevelIcon(right_air_label_x + right_air_label_w + 4, row_top + row_step - 1, pm_icon_dir);
    drawPairNumbersWithUnits(right_air_label_x, row_top + row_step + 13, pm10_str, "", pm25_str, "");

    Paint_DrawImage(pressure_32x32, right_x, row_top + 2 * row_step + 1, 32, 32);
    String right_press_label_s = String(INTL_DISP_PRESSURE) + " mmHg";
    const char *right_press_label = right_press_label_s.c_str();
    uint16_t right_press_label_x = right_x + 42;
    Paint_DrawString_Display(right_press_label_x, row_top + 2 * row_step, right_press_label, &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
    uint16_t right_press_label_w = Paint_GetStringWidth_Display(right_press_label, &Font12, &font_12_cyrillic, &font_12_ascii);
    drawWarningLevelIcon(right_press_label_x + right_press_label_w + 4, row_top + 2 * row_step - 1, press_icon_dir);
    drawPairNumbersWithUnits(right_press_label_x, row_top + 2 * row_step + 13, press_out, "", press_in, "");

    // Footer info: grouped by source to improve readability.
    auto levelWord = [](int dir) -> const char* { return (dir > 0) ? INTL_DISP_LEVEL_HIGH : INTL_DISP_LEVEL_LOW; };
    auto makeTooPhrase = [&](const char *measure, int dir) -> String {
        String s = String(measure);
        if (strlen(INTL_DISP_IS_TOO) > 0) {
            s += " ";
            s += INTL_DISP_IS_TOO;
        }
        s += " ";
        s += levelWord(dir);
        return s;
    };
    auto appendIssue = [](String &line, const String &issue) {
        if (issue.length() == 0) return;
        if (line.length() > 0) line += ", ";
        line += issue;
    };

    String urban_issues = "";
    String insight_issues = "";

    if (footer_test_all_warnings) {
        appendIssue(urban_issues, makeTooPhrase("RH", -1));
        appendIssue(urban_issues, makeTooPhrase(INTL_DISP_TEMP_SHORT, 1));
        appendIssue(urban_issues, makeTooPhrase(INTL_DISP_PRESS_SHORT, -1));
        appendIssue(urban_issues, makeTooPhrase("PM", 1));
        appendIssue(urban_issues, makeTooPhrase(INTL_DISP_NOISE, 1));
        appendIssue(urban_issues, String(INTL_DISP_DEW_POINT_IS) + String("12°C"));
        appendIssue(insight_issues, makeTooPhrase("RH", 1));
        appendIssue(insight_issues, makeTooPhrase(INTL_DISP_TEMP_SHORT, 1));
        appendIssue(insight_issues, makeTooPhrase(INTL_DISP_PRESS_SHORT, -1));
        appendIssue(insight_issues, makeTooPhrase("CO2", 1));
    } else {
        if (hum_out_dir != 0) appendIssue(urban_issues, makeTooPhrase("RH", hum_out_dir));
        if (hum_in_dir != 0) appendIssue(insight_issues, makeTooPhrase("RH", hum_in_dir));

        if (temp_out_dir != 0) appendIssue(urban_issues, makeTooPhrase(INTL_DISP_TEMP_SHORT, temp_out_dir));
        if (temp_in_dir != 0) appendIssue(insight_issues, makeTooPhrase(INTL_DISP_TEMP_SHORT, temp_in_dir));

        if (press_out_dir != 0) appendIssue(urban_issues, makeTooPhrase(INTL_DISP_PRESS_SHORT, press_out_dir));
        if (press_in_dir != 0) appendIssue(insight_issues, makeTooPhrase(INTL_DISP_PRESS_SHORT, press_in_dir));

        // Urban-specific sensors on main screen.
        if (pm_dir != 0) appendIssue(urban_issues, makeTooPhrase("PM", pm_dir));
        if (noise_dir != 0) appendIssue(urban_issues, makeTooPhrase(INTL_DISP_NOISE, noise_dir));
        // Insight-specific sensor on main screen.
        if (co2_dir != 0) appendIssue(insight_issues, makeTooPhrase("CO2", co2_dir));
    }

    // Include dew point in Urban line when available.
    float dew_point_c = calculateDewPointC(values.temp_outdoor, values.hum_outdoor);
    char dew_str[16];
    formatMetricValue(dew_str, sizeof(dew_str), dew_point_c, 0, true);
    if (!footer_test_all_warnings && !isTempNoData(dew_point_c)) {
        appendIssue(urban_issues, String(INTL_DISP_DEW_POINT_IS) + String(dew_str) + String("°C"));
    }

    String source_lines[3];
    int source_line_count = 0;
    if (urban_issues.length() > 0) {
        source_lines[source_line_count++] = String("Urban: ") + urban_issues;
    }
    if (insight_issues.length() > 0) {
        source_lines[source_line_count++] = String("Insight: ") + insight_issues;
    }
    if (source_line_count == 0) {
        source_lines[source_line_count++] = INTL_DISP_CHECK_MAP_FULL_DATA;
    }

    // Footer text: use as much width as possible (excluding the right sidebar only).
    uint16_t text_x = content_left;
    // `content_right` is conservative; for footer wrapping we can go up to the last pixel before the sidebar.
    uint16_t text_right = (DISPLAY_WIDTH > nav_sidebar_width + 1) ? (DISPLAY_WIDTH - nav_sidebar_width - 1) : content_right;

    // Wrap into center block between QRs, left-aligned next to info icon.
    const uint16_t info_icon_size = 32;
    uint16_t info_icon_x = text_x;
    uint16_t body_text_x = info_icon_x + info_icon_size + 2;
    uint16_t body_text_w = (text_right > body_text_x) ? (text_right - body_text_x) : 0;
    String wrapped[4];
    int wrapped_count = 0;
    const int max_lines = footer_test_all_warnings ? 4 : 3;

    for (int src = 0; src < source_line_count && wrapped_count < max_lines; src++) {
        String current = "";
        int pos = 0;
        const String &line = source_lines[src];
        while (pos < line.length() && wrapped_count < max_lines) {
            int next_space = line.indexOf(' ', pos);
            if (next_space < 0) next_space = line.length();
            String token = line.substring(pos, next_space);
            if (token.length() == 0) {
                pos = next_space + 1;
                continue;
            }

            String candidate = (current.length() == 0) ? token : (current + " " + token);
            // Measure with the SAME font as the tips, so wrapping uses the full available width.
            uint16_t candidate_w = Paint_GetStringWidth_Display(candidate.c_str(), &Font12, &font_10_cyrillic, &font_10_ascii);
            if (candidate_w <= body_text_w || current.length() == 0) {
                current = candidate;
            } else {
                wrapped[wrapped_count++] = current;
                current = token;
            }
            pos = next_space + 1;
        }
        if (wrapped_count < max_lines && current.length() > 0) {
            wrapped[wrapped_count++] = current;
        }
    }

    // Keep footer text anchored near the lower area even when top QR test mode is active.
    const uint16_t qr_y = DISPLAY_HEIGHT - 44;
    const uint16_t first_line_y = (qr_y > 10) ? (qr_y - 10) : qr_y;
    Paint_DrawImage(info_32x32, info_icon_x, first_line_y - 3, info_icon_size, info_icon_size);
    const uint16_t tipLineH =
#ifdef INTL_RU
        (font_10_cyrillic.line_height ? font_10_cyrillic.line_height : Font12.Height);
#else
        (font_10_ascii.line_height ? font_10_ascii.line_height : Font12.Height);
#endif
    const uint16_t line_step = tipLineH + 2;
    for (int i = 0; i < wrapped_count; i++) {
        Paint_DrawString_Display(body_text_x, first_line_y + i * line_step, wrapped[i].c_str(), &Font12, &font_10_cyrillic, &font_10_ascii, WHITE, BLACK);
    }
}

#endif