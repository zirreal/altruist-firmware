#ifdef ALTRUIST_INSIDE

#include "connecting.h"
#include "../paint_driver/GUI_Paint.h"
#include "../../config_manager/config_helpers.h"
#include "../icons/icons/40x40/robo_hw_logo_black_40x40.h"  // team logo

// step goes from 0..100 (progress percentage)
void showConnectingPage(UBYTE *BlackImage, int step) {
    // Clear screen
    Paint_Clear(WHITE);

    // Create a more modern layout with better spacing
    int content_start_y = 30;

    // --- Header with logo and title together ---
    int logo_x = (DISPLAY_WIDTH - 40) / 2;
    int logo_y = content_start_y;
    Paint_DrawImage(robo_hw_logo_black_40x40, logo_x, logo_y, 40, 40);

    // Title directly below logo with better spacing
    const char *title = "ALTRUIST INSIGHT";
    int title_width = strlen(title) * Font20.Width;
    int title_x = (DISPLAY_WIDTH - title_width) / 2;
    int title_y = logo_y + 50;
    Paint_DrawString_EN(title_x, title_y, title, &Font20, WHITE, BLACK);

    // --- Connection status section ---
    int status_section_y = title_y + Font20.Height + 25;
    
    // Connection status
    const char *status = "Connecting to Wi-Fi";
    int status_width = strlen(status) * Font16.Width;
    int status_x = (DISPLAY_WIDTH - status_width) / 2;
    Paint_DrawString_EN(status_x, status_section_y, status, &Font16, WHITE, BLACK);

    // Network name with better formatting
    char network_display[64];
    snprintf(network_display, sizeof(network_display), "\"%s\"", cfg::wlanssid);
    int network_width = strlen(network_display) * Font16.Width;
    int network_x = (DISPLAY_WIDTH - network_width) / 2;
    int network_y = status_section_y + Font16.Height + 8;
    Paint_DrawString_EN(network_x, network_y, network_display, &Font16, WHITE, BLACK);

    // --- Simple static dots ---
    int dots_y = network_y + Font16.Height + 25;
    int dot_spacing = 12;
    int num_dots = 5;
    int total_dots_width = num_dots * 6 + (num_dots - 1) * dot_spacing;
    int dots_start_x = (DISPLAY_WIDTH - total_dots_width) / 2;
    
    for (int i = 0; i < num_dots; i++) {
        int dot_x = dots_start_x + 3 + i * (6 + dot_spacing);
        Paint_DrawCircle(dot_x, dots_y, 3, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    }

    // --- Help text ---
    const char* help_text = "Please wait...";
    int help_width = strlen(help_text) * Font12.Width;
    int help_x = (DISPLAY_WIDTH - help_width) / 2;
    int help_y = dots_y + 25;
    Paint_DrawString_EN(help_x, help_y, help_text, &Font12, WHITE, BLACK);
}

#endif
