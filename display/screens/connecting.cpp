#ifdef ALTRUIST_INSIDE

#include "connecting.h"
#include "../paint_driver/GUI_Paint.h"
#include "../utils.h"
#include "../../config_manager/config_helpers.h"

// step goes from 0..100 (progress percentage)
void showConnectingPage(UBYTE *BlackImage, int step) {
    // Clear screen
    Paint_Clear(WHITE);

    // Title
    const char *title = "Wi-Fi Setup";
    int title_x = (DISPLAY_WIDTH - strlen(title) * Font24.Width) / 2;
    int title_y = 40;
    Paint_DrawString_EN(title_x, title_y, title, &Font24, BLACK, WHITE);

    // Status line with animated dots
    const char *base_status = "Connecting";
    char status[32];
    int dots = (step / 10) % 4; // cycle 0..3
    snprintf(status, sizeof(status), "%s%s", base_status,
             dots == 0 ? "" : (dots == 1 ? "." : (dots == 2 ? ".." : "...")));

    int status_x = (DISPLAY_WIDTH - strlen(status) * Font16.Width) / 2;
    int status_y = title_y + Font24.Height + 20;
    Paint_DrawString_EN(status_x, status_y, status, &Font16, BLACK, WHITE);

    // SSID
    const char *ssid = cfg::wlanssid;
    int ssid_x = (DISPLAY_WIDTH - strlen(ssid) * Font24.Width) / 2;
    int ssid_y = status_y + Font16.Height + 10;
    Paint_DrawString_EN(ssid_x, ssid_y, ssid, &Font24, BLACK, WHITE);

    // Progress bar outline
    int bar_width = DISPLAY_WIDTH / 2;
    int bar_height = 12;
    int bar_x = (DISPLAY_WIDTH - bar_width) / 2;
    int bar_y = ssid_y + Font24.Height + 25;
    Paint_DrawRectangle(bar_x, bar_y, bar_x + bar_width, bar_y + bar_height,
                        BLACK, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);

    // Fill bar based on step (0–100%)
    int progress = (bar_width * step) / 100;
    if (progress > 0) {
        Paint_DrawRectangle(bar_x, bar_y, bar_x + progress, bar_y + bar_height,
                            BLACK, DOT_PIXEL_2X2, DRAW_FILL_FULL);
    }
}

#endif
