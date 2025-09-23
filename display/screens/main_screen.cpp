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


// Draw a value with icon, label, and units
void drawValue(const char *label, float value, uint8_t precision,
               const unsigned char *image, const char *units,
               uint16_t image_size, uint16_t x_start, uint16_t y_start,
               uint16_t image_offset = 0) {
    Paint_DrawImage(image, x_start, y_start, image_size, image_size);
    Paint_DrawString_EN(x_start + image_size + image_offset, y_start, label, &Font12, WHITE, BLACK);

    if (value < 0) {
        Paint_DrawString_EN(x_start + image_size + image_offset,
                            y_start + Font12.Height + 5,
                            "No data", &Font16, WHITE, BLACK);
    } else {
        char value_str[10];
        stringFromFloat(value_str, value, precision);
        Paint_DrawString_EN(x_start + image_size + image_offset,
                            y_start + Font12.Height + 5,
                            value_str, &Font20, WHITE, BLACK);
        Paint_DrawString_EN(x_start + image_size + Font20.Width * strlen(value_str) + image_offset,
                            y_start + Font12.Height + 5 + Font20.Height / 4,
                            units, &Font12, WHITE, BLACK);
    }
}

// Parse JSON data into struct
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
        if (urban.containsKey("SDS_P1")) values.pm10 = urban["SDS_P1"]["value"].as<float>();
        if (urban.containsKey("SDS_P2")) values.pm25 = urban["SDS_P2"]["value"].as<float>();
        if (urban.containsKey("BME280_humidity")) values.hum_outdoor = urban["BME280_humidity"]["value"].as<float>();
        if (urban.containsKey("BME280_temperature")) values.temp_outdoor = urban["BME280_temperature"]["value"].as<float>();
        if (urban.containsKey("BME280_pressure")) values.press_outdoor = urban["BME280_pressure"]["value"].as<float>() * 0.0075;
        if (urban.containsKey("PCBA_noiseMax")) values.noise_max = urban["PCBA_noiseMax"]["value"].as<float>();
        if (urban.containsKey("PCBA_noiseAvg")) values.noise_avg = urban["PCBA_noiseAvg"]["value"].as<float>();
    }
    if (data.containsKey("SCD4x")) values.co2 = data["SCD4x"]["co2"]["value"].as<float>();
    if (data.containsKey("BME680")) {
        auto bme = data["BME680"];
        values.temp_indoor = bme["temperature"]["value"].as<float>();
        values.hum_indoor = bme["humidity"]["value"].as<float>();
        values.press_indoor = bme["pressure"]["value"].as<float>() * 0.0075;
    }
}

// Draw the full main screen
void drawMainScreen(UBYTE *BlackImage, const String &jsonString, const String &device_ip) {
    main_screen_values_t values;
    _parseJsonToStruct(jsonString, values);

    uint16_t top_bar_height = Font16.Height + Font12.Height + 8;
    uint16_t column_width = DISPLAY_WIDTH / 3;

    // Top black bar
    Paint_DrawRectangle(0, 0, DISPLAY_WIDTH, top_bar_height, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);

    // Urban Section
    Paint_DrawString_EN(5, 5, "Urban", &Font16, BLACK, WHITE);
    Paint_DrawString_EN(5, Font16.Height + 5, values.ip_address.c_str(), &Font12, BLACK, WHITE);

    // Insight Section
    Paint_DrawString_EN(2 * column_width + 5, 5, "Insight", &Font16, BLACK, WHITE);
    Paint_DrawString_EN(2 * column_width + 5, Font16.Height + 5, device_ip.c_str(), &Font12, BLACK, WHITE);

    // Centered date/time
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
        char buf[18];
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", &timeinfo);
        int x_center = DISPLAY_WIDTH / 2 - strlen(buf) * Font12.Width / 2;
        Paint_DrawString_EN(x_center, 5, buf, &Font12, BLACK, WHITE);
    }

    // Vertical separators
    Paint_DrawLine(column_width, 0, column_width, top_bar_height, WHITE, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    Paint_DrawLine(2 * column_width, 0, 2 * column_width, top_bar_height, WHITE, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

    // Values layout
    uint16_t y_start = top_bar_height + 10;

    // Column 1 - Urban sensor readings
    drawValue("PM10", values.pm10, 2, air_filter_35x35, "ppm", 35, 5, y_start);
    drawValue("PM2.5", values.pm25, 2, air_filter_35x35, "ppm", 35, 5, y_start + 50);
    drawValue("Noise Max", values.noise_max, 0, ear_hearing_35x35, "db", 35, 5, y_start + 100);
    drawValue("Noise Avg", values.noise_avg, 0, ear_hearing_35x35, "db", 35, 5, y_start + 150);

    // Column 2 - Outdoor environment
    drawValue("Temp", values.temp_outdoor, 1, wi_thermometer_cropped_35x35, "C", 35, column_width + 5, y_start);
    drawValue("Humidity", values.hum_outdoor, 1, wi_humidity_cropped_35x35, "%", 35, column_width + 5, y_start + 50);
    drawValue("Pressure", values.press_outdoor, 0, pressure_40x40, "mm/Hg", 40, column_width + 5, y_start + 100);

    // Column 3 - Indoor environment
    drawValue("Temp", values.temp_indoor, 1, house_thermometer_40x40, "C", 40, 2 * column_width + 5, y_start);
    drawValue("Humidity", values.hum_indoor, 1, house_humidity_40x40, "%", 40, 2 * column_width + 5, y_start + 50);
    drawValue("Pressure", values.press_indoor, 0, pressure_40x40, "mm/Hg", 40, 2 * column_width + 5, y_start + 100);
    drawValue("CO2", values.co2, 1, co2_svgrepo_com_35x35, "ppm", 35, 2 * column_width + 5, y_start + 150);
}

#endif