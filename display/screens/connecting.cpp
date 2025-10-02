#ifdef ALTRUIST_INSIDE

#include "connecting.h"
#include "../paint_driver/GUI_Paint.h"
#include "../utils.h"
#include "../../config_manager/config_helpers.h"
#include "../icons/icons/40x40/robo_hw_logo_black_40x40.h"  // team logo

// step goes from 0..100 (progress percentage)
void showConnectingPage(UBYTE *BlackImage, int step) {
    // Clear screen
    Paint_Clear(WHITE);

    // Position content higher on screen (not perfectly centered)
    int start_y = 20; // Start higher up instead of centering

    // --- Logo (centered) ---
    int logo_x = (DISPLAY_WIDTH - 40) / 2;
    int logo_y = start_y;
    Paint_DrawImage(robo_hw_logo_black_40x40, logo_x, logo_y, 40, 40);

    // --- Title (larger font, centered) ---
    const char *title = "Wi-Fi Setup";
    int title_width = strlen(title) * Font24.Width;
    int title_x = (DISPLAY_WIDTH - title_width) / 2;
    int title_y = logo_y + 50;
    Paint_DrawString_EN(title_x, title_y, title, &Font24, BLACK, WHITE);

    // --- Status (larger font, simple static text for e-ink) ---
    const char *status = "Connecting...";
    int status_width = strlen(status) * Font20.Width;
    int status_x = (DISPLAY_WIDTH - status_width) / 2;
    int status_y = title_y + Font24.Height + 15;
    Paint_DrawString_EN(status_x, status_y, status, &Font20, BLACK, WHITE);

    // --- SSID (larger font, centered) ---
    const char *ssid = cfg::wlanssid;
    int ssid_width = strlen(ssid) * Font16.Width;
    int ssid_x = (DISPLAY_WIDTH - ssid_width) / 2;
    int ssid_y = status_y + Font20.Height + 10;
    Paint_DrawString_EN(ssid_x, ssid_y, ssid, &Font16, BLACK, WHITE);

    // --- Simple visual separator line instead of progress bar ---
    int line_width = DISPLAY_WIDTH / 3; // Simple decorative line
    int line_x = (DISPLAY_WIDTH - line_width) / 2;
    int line_y = ssid_y + Font16.Height + 25;
    Paint_DrawLine(line_x, line_y, line_x + line_width, line_y, BLACK, DOT_PIXEL_2X2, LINE_STYLE_SOLID);

    // --- Powered by (bottom, centered) ---
    const char* powered = "Powered by Robonomics";
    int powered_width = strlen(powered) * Font12.Width;
    int powered_x = (DISPLAY_WIDTH - powered_width) / 2;
    int powered_y = DISPLAY_HEIGHT - Font12.Height - 10;
    Paint_DrawString_EN(powered_x, powered_y, powered, &Font12, BLACK, WHITE);
}

#endif
