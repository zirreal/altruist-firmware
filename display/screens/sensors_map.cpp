#ifdef ALTRUIST_INSIDE

#include <qrcode.h>
#include "../../config_manager/config_helpers.h"
#include "main_screen.h"
#include "../driver/EPD.h"
#include "setup.h"
#include "../../utils.h"
#include "display_common.h"
#include "../icons/icons/40x40/location_40x40.h" // location icon

QRCode QRSensorMap; 

void showSensorsMapPage(const String& robonomics_address) {
    uint8_t qrcodeData[qrcode_getBufferSize(12)];
    char qr_data[300];
    char lat[32], lon[32];
    const char* addr = robonomics_address.c_str();
    sscanf(cfg::coords_gps, "%31[^,],%31s", lat, lon);
    sprintf(qr_data, "https://sensors.social/#/remote/%s/17/%s/%s/%s", addr, lat, lon, addr);
    qrcode_initText(&QRSensorMap, qrcodeData, 12, ECC_LOW, qr_data);

    // scale factor
    int scale_factor = 3;
    int scaled_qr_width = QRSensorMap.size * scale_factor;
    int scaled_qr_height = QRSensorMap.size * scale_factor;
    int quiet_zone = 3;
    int total_width = scaled_qr_width + (2 * quiet_zone);
    int total_height = scaled_qr_height + (2 * quiet_zone);

    int qr_bitmap_width_bytes = (total_width + 7) / 8;
    int qr_bitmap_size = total_height * qr_bitmap_width_bytes;
    unsigned char *qr_bitmap_scaled = (unsigned char*)malloc(qr_bitmap_size);
    memset(qr_bitmap_scaled, 0x00, qr_bitmap_size);

    for (uint8_t qr_y = 0; qr_y < QRSensorMap.size; qr_y++) {
        for (uint8_t qr_x = 0; qr_x < QRSensorMap.size; qr_x++) {
            if (qrcode_getModule(&QRSensorMap, qr_x, qr_y)) {
                for (int sy = 0; sy < scale_factor; sy++) {
                    for (int sx = 0; sx < scale_factor; sx++) {
                        int pixel_x = quiet_zone + (qr_x * scale_factor) + sx;
                        int pixel_y = quiet_zone + (qr_y * scale_factor) + sy;
                        qr_bitmap_scaled[pixel_y * qr_bitmap_width_bytes + (pixel_x / 8)] |= (0x80 >> (pixel_x % 8));
                    }
                }
            }
        }
    }

    Paint_Clear(WHITE);

    // Title with background
    const char* title = "Sensors Map";
    int title_x = (DISPLAY_WIDTH - strlen(title) * Font24.Width) / 2;
    Paint_DrawString_EN(title_x, 2, title, &Font24, WHITE, BLACK);

    // Subtitle
    const char* subtitle = "Scan to open online";
    int subtitle_x = (DISPLAY_WIDTH - strlen(subtitle) * Font16.Width) / 2;
    Paint_DrawString_EN(subtitle_x, Font24.Height + 10, subtitle, &Font16, BLACK, WHITE);

    // Draw location icon next to subtitle
    int icon_x = subtitle_x - 45;
    int icon_y = Font24.Height + 5;
    Paint_DrawImage(location_40x40, icon_x, icon_y, 40, 40);

    // QR code centered
    int qr_x = DISPLAY_WIDTH / 2 - total_width / 2;
    int qr_y = DISPLAY_HEIGHT / 2 - total_height / 2;
    Paint_DrawImage(qr_bitmap_scaled, qr_x, qr_y, total_width, total_height);

    // Powered by
    const char* powered = "Powered by Robonomics";
    int powered_x = (DISPLAY_WIDTH - strlen(powered) * Font12.Width) / 2;
    int powered_y = DISPLAY_HEIGHT - Font12.Height - 5;
    Paint_DrawString_EN(powered_x, powered_y, powered, &Font12, WHITE, BLACK);

    free(qr_bitmap_scaled);
}

#endif
