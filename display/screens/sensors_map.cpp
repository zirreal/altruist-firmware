#ifdef ALTRUIST_INSIDE

#include <qrcode.h>
#include "../../config_manager/config_helpers.h"
#include "main_screen.h"
#include "../driver/EPD.h"
#include "setup.h"
#include "../../utils.h"
#include "display_common.h"
#include "../icons/icons/icons_15x15.h"
#include "../icons/icons/40x40/location_40x40.h" // location icon

QRCode QRSensorMap; 

void showSensorsMapPage(const String& robonomics_address) {
    uint8_t qrcodeData[qrcode_getBufferSize(12)];
    char qr_data[300];
    // Safe defaults
    char lat[32] = "0.0";
    char lon[32] = "0.0";
    const char* addr = robonomics_address.c_str();

    if (robonomics_address.length() == 0) {
        snprintf(qr_data, sizeof(qr_data), "https://sensors.social/");
    } else {
        char lat[32] = "0.0";
        char lon[32] = "0.0";
        bool coords_ok = false;
        if (cfg::coords_gps != nullptr && strlen(cfg::coords_gps) > 0) {
            int parsed = sscanf(cfg::coords_gps, "%31[^,],%31s", lat, lon);
            coords_ok = (parsed == 2);
        }
        int zoom = coords_ok ? 18 : 3;
        // Date in YYYY-MM-DD
        char date[11] = "1970-01-01";
        struct tm timeinfo;
        if (getLocalTime(&timeinfo)) {
            strftime(date, sizeof(date), "%Y-%m-%d", &timeinfo);
        }
        snprintf(qr_data, sizeof(qr_data), "https://sensors.social/?type=noisemax&date=%s&provider=remote&lat=%s&lng=%s&zoom=%d&sensor=%s", date, lat, lon, zoom, addr);
    }
    qrcode_initText(&QRSensorMap, qrcodeData, 12, ECC_LOW, qr_data);

    // Medium-size QR to balance readability and space
    int scale_factor = 3;
    int quiet_zone = 3; 
    int scaled_qr_width = QRSensorMap.size * scale_factor;
    int scaled_qr_height = QRSensorMap.size * scale_factor;
    int total_width = scaled_qr_width + (2 * quiet_zone);
    int total_height = scaled_qr_height + (2 * quiet_zone);

    int qr_bitmap_width_bytes = (total_width + 7) / 8;
    int qr_bitmap_size = total_height * qr_bitmap_width_bytes;
    unsigned char *qr_bitmap_scaled = (unsigned char*)malloc(qr_bitmap_size);
    if (!qr_bitmap_scaled) {
        return;
    }
    memset(qr_bitmap_scaled, 0x00, qr_bitmap_size);

    for (uint8_t qr_y = 0; qr_y < QRSensorMap.size; qr_y++) {
        for (uint8_t qr_x = 0; qr_x < QRSensorMap.size; qr_x++) {
            if (qrcode_getModule(&QRSensorMap, qr_x, qr_y)) {
                for (int sy = 0; sy < scale_factor; sy++) {
                    for (int sx = 0; sx < scale_factor; sx++) {
                        int pixel_x = quiet_zone + (qr_x * scale_factor) + sx;
                        int pixel_y = quiet_zone + (qr_y * scale_factor) + sy;
                        int byte_index = pixel_y * qr_bitmap_width_bytes + (pixel_x / 8);
                        qr_bitmap_scaled[byte_index] |= (0x80 >> (pixel_x % 8));
                    }
                }
            }
        }
    }

    Paint_Clear(WHITE);

    // === HEADER: same style as main screen, but with map icon ===
    struct tm timeinfo;
    const uint16_t header_top_y = 6;
    const uint16_t header_row_height = Font16.Height + 2;
    uint16_t header_bottom_border_y = header_top_y + header_row_height + 2;

    // Left: sensor map icon å
    const uint16_t header_icon_size = 15;
    const uint16_t header_icon_x    = 4;
    const uint16_t header_icon_y    = header_top_y;
    Paint_DrawImage(map_nav_15x15, header_icon_x, header_icon_y, header_icon_size, header_icon_size);

    if (getLocalTime(&timeinfo)) {
        char date_buf[12], time_buf[8];
        strftime(date_buf, sizeof(date_buf), "%m/%d/%Y", &timeinfo);
        strftime(time_buf, sizeof(time_buf), "%H:%M",    &timeinfo);

        // Center: time
        int time_width = strlen(time_buf) * Font16.Width;
        int time_x = (DISPLAY_WIDTH - time_width) / 2;
        int time_y = header_top_y;
        Paint_DrawString_EN(time_x, time_y, time_buf, &Font16, WHITE, BLACK);

        // Right: date
        int date_width = strlen(date_buf) * Font12.Width;
        const int right_margin = 4;
        int date_x = DISPLAY_WIDTH - right_margin - date_width;
        int date_y = header_top_y + 2;
        Paint_DrawString_EN(date_x,     date_y, date_buf, &Font12, WHITE, BLACK);
        Paint_DrawString_EN(date_x + 1, date_y, date_buf, &Font12, WHITE, BLACK);
    }

    // Header bottom border
    Paint_DrawLine(0, header_bottom_border_y, DISPLAY_WIDTH, header_bottom_border_y,
                   BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

    uint16_t content_top_y = header_bottom_border_y + 10;

    // Subtitle just under header
    const char* subtitle_main = "Sensors Map";
    int subtitle_main_x = (DISPLAY_WIDTH - strlen(subtitle_main) * Font16.Width) / 2;
    int subtitle_main_y = content_top_y;
    Paint_DrawString_EN(subtitle_main_x, subtitle_main_y, subtitle_main, &Font16, WHITE, BLACK);

    // Center QR in the remaining vertical space
    int qr_top_after_text = subtitle_main_y + Font16.Height + 8;
    int available_height = DISPLAY_HEIGHT - qr_top_after_text - (Font12.Height + 8);
    if (available_height < (int)total_height) {
        available_height = total_height;
    }
    int qr_area_top = qr_top_after_text;
    int qr_y = qr_area_top + (available_height - total_height) / 2;
    int qr_x = DISPLAY_WIDTH / 2 - total_width / 2;
    Paint_DrawImage(qr_bitmap_scaled, qr_x, qr_y, total_width, total_height);

    // Location icon to the left of QR, vertically centered with it
    int icon_x = qr_x - 40 - 8; // icon width (40) + small gap
    if (icon_x < 0) icon_x = 0;
    int icon_y = qr_y + (total_height - 40) / 2;
    if (icon_y < 0) icon_y = 0;
    Paint_DrawImage(location_40x40, icon_x, icon_y, 40, 40);

    // "Scan to open online" label just above QR
    const char* subtitle_scan = "Scan to open online";
    int subtitle_scan_x = (DISPLAY_WIDTH - strlen(subtitle_scan) * Font16.Width) / 2;
    int subtitle_scan_y = qr_y - Font16.Height + 4;
    Paint_DrawString_EN(subtitle_scan_x, subtitle_scan_y, subtitle_scan, &Font16, BLACK, WHITE);

    // Powered by (bottom)
    const char* powered = "Powered by Robonomics";
    int powered_x = (DISPLAY_WIDTH - strlen(powered) * Font12.Width) / 2;
    int powered_y = DISPLAY_HEIGHT - Font12.Height - 5;
    Paint_DrawString_EN(powered_x, powered_y, powered, &Font12, WHITE, BLACK);

    free(qr_bitmap_scaled);
}

#endif
