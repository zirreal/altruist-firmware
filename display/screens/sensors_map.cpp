#ifdef ALTRUIST_INSIDE

#include <qrcode.h>
#include "../../config_manager/config_helpers.h"
#include "main_screen.h"
#include "../driver/EPD.h"
#include "setup.h"
#include "../utils.h"
#include "../../utils.h"
#include "display_common.h"
#include "../../intl.h"
#include "../paint_driver/fonts/fonts.h"
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

    // QR size: scale 3 (~117px) with strict bottom limit so it stays above "Powered by"
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

        // Center: time (same display font as rest of UI)
        int time_width = (int)Paint_GetStringWidth_Display(time_buf, &Font16, &font_16_cyrillic, &font_16_ascii);
        int time_x = (DISPLAY_WIDTH - time_width) / 2;
        int time_y = header_top_y;
        Paint_DrawString_Display(time_x, time_y, time_buf, &Font16, &font_16_cyrillic, &font_16_ascii, WHITE, BLACK);

        // Right: date (same display font as rest of UI)
        int date_width = (int)Paint_GetStringWidth_Display(date_buf, &Font12, &font_12_cyrillic, &font_12_ascii);
        const int right_margin = 4;
        int date_x = DISPLAY_WIDTH - right_margin - date_width;
        int date_y = header_top_y + 2;
        Paint_DrawString_Display(date_x,     date_y, date_buf, &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
        Paint_DrawString_Display(date_x + 1, date_y, date_buf, &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
    }

    // Header bottom border
    Paint_DrawLine(0, header_bottom_border_y, DISPLAY_WIDTH, header_bottom_border_y,
                   BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

    uint16_t content_top_y = header_bottom_border_y + 6;

    // Subtitle just under header
    const char* subtitle_main = INTL_DISP_SENSORS_MAP;
    int subtitle_main_x = (DISPLAY_WIDTH - (int)Paint_GetStringWidth_Display(subtitle_main, &Font16, &font_16_cyrillic, &font_16_ascii)) / 2;
    int subtitle_main_y = content_top_y;
    Paint_DrawString_Display(subtitle_main_x, subtitle_main_y, subtitle_main, &Font16, &font_16_cyrillic, &font_16_ascii, WHITE, BLACK);

    // Tighter spacing so "Scan to open" and QR sit higher, leaving room for "Powered by" below
    const int gap_after_subtitle = 6;
    const int gap_after_scan = 4;
    const int scan_line_offset = 4;  /* nudge "Scan to open" bar + text slightly lower */
    int subtitle_scan_y = content_top_y + Font16.Height + gap_after_subtitle + scan_line_offset;
    const char* subtitle_scan = INTL_DISP_SCAN_TO_OPEN;
    int subtitle_scan_w = (int)Paint_GetStringWidth_Display(subtitle_scan, &Font16, &font_16_cyrillic, &font_16_ascii);
    int subtitle_scan_x = (DISPLAY_WIDTH - subtitle_scan_w) / 2;
    int scan_bar_pad = 6;
    int scan_bar_left = subtitle_scan_x - scan_bar_pad;
    if (scan_bar_left < 0) scan_bar_left = 0;
    int scan_bar_right = subtitle_scan_x + subtitle_scan_w + scan_bar_pad;
    if (scan_bar_right > (int)DISPLAY_WIDTH) scan_bar_right = DISPLAY_WIDTH;
    Paint_DrawRectangle(scan_bar_left, subtitle_scan_y, scan_bar_right, subtitle_scan_y + Font16.Height + 4, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    Paint_DrawString_Display_OnBlack(subtitle_scan_x, subtitle_scan_y + 2, subtitle_scan, &Font16, &font_16_cyrillic, &font_16_ascii, WHITE, BLACK);

    int qr_area_top = subtitle_scan_y + Font16.Height + gap_after_scan;
    /* "Powered by" position first; QR must end above this with a clear gap */
    const int gap_below_qr = 24;
    int powered_y = DISPLAY_HEIGHT - Font12.Height - 8;
    int qr_max_bottom = powered_y - gap_below_qr;
    int available_height = qr_max_bottom - qr_area_top;
    if (available_height < 32) {
        available_height = 32;
    }
    int qr_y = qr_area_top + (available_height - total_height) / 2;
    if (qr_y + total_height > qr_max_bottom) {
        qr_y = qr_max_bottom - total_height;
    }
    if (qr_y < qr_area_top) {
        qr_y = qr_area_top;
    }
    int qr_x = DISPLAY_WIDTH / 2 - total_width / 2;
    Paint_DrawImage(qr_bitmap_scaled, qr_x, qr_y, total_width, total_height);

    // Location icon to the left of QR, vertically centered with it
    int icon_x = qr_x - 40 - 8; // icon width (40) + small gap
    if (icon_x < 0) icon_x = 0;
    int icon_y = qr_y + (total_height - 40) / 2;
    if (icon_y < 0) icon_y = 0;
    Paint_DrawImage(location_40x40, icon_x, icon_y, 40, 40);

    // Powered by (bottom) — drawn last so it stays on top of the QR
    const char* powered = INTL_DISP_POWERED_BY;
    int powered_x = (DISPLAY_WIDTH - (int)Paint_GetStringWidth_Display(powered, &Font12, &font_12_cyrillic, &font_12_ascii)) / 2;
    Paint_DrawString_Display(powered_x, powered_y, powered, &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);

    free(qr_bitmap_scaled);
}

#endif
