#ifdef ALTRUIST_INSIDE

#include "display_common.h"
#include "../paint_driver/GUI_Paint.h"
#include "../display_manager.h"
#include "../icons/icons/15x15/display_frame_15x15.h"
#include "../icons/icons/15x15/gear_15x15.h"
#include "../icons/icons/15x15/line_graph_15x15.h"
#include "../icons/icons/15x15/map_15x15.h"

static bool epd_initialized = false;

void initAndClearScreen() {
#ifdef DISPLAY_3IN52
    if (!epd_initialized) {
        EPD_3IN52_Init();
        epd_initialized = true;
    }
    EPD_3IN52_lut_GC();
#endif
#ifdef DISPLAY_4IN2
    if (!epd_initialized) {
        EPD_4IN2_V2_Init();
        // One-time clear on first init only
        EPD_4IN2_V2_Clear();
        DEV_Delay_ms(100);
        epd_initialized = true;
    }
    // No repeated clear; keep updates fast
#endif
}

void createNewImage(UBYTE *&BlackImage) {
#ifdef DISPLAY_4IN2
    UWORD Imagesize = ((EPD_4IN2_V2_WIDTH % 8 == 0)? (EPD_4IN2_V2_WIDTH / 8 ): (EPD_4IN2_V2_WIDTH / 8 + 1)) * EPD_4IN2_V2_HEIGHT;
    if (BlackImage) free(BlackImage);
    BlackImage = (UBYTE *)malloc(Imagesize);
    if (BlackImage == NULL) {
        return;
    }
    Paint_NewImage(BlackImage, EPD_4IN2_V2_WIDTH, EPD_4IN2_V2_HEIGHT, ROTATE_0, WHITE);
    Paint_Clear(WHITE);
#endif
}

void showImageFast(UBYTE *&BlackImage) {
#ifdef DISPLAY_4IN2
    // Fast update without sleep
    EPD_4IN2_V2_Display(BlackImage);
#endif
}

void showImageLong(UBYTE *&BlackImage) {
#ifdef DISPLAY_3IN52
    EPD_3IN52_SendCommand(0x50);
    EPD_3IN52_SendData(0x17);

    EPD_3IN52_display(BlackImage);
    EPD_3IN52_lut_GC();
    EPD_3IN52_refresh();
    DEV_Delay_ms(200);
    EPD_3IN52_sleep();
#endif
#ifdef DISPLAY_4IN2
    EPD_4IN2_V2_Init();
    EPD_4IN2_V2_Display(BlackImage);
    DEV_Delay_ms(100);
    // EPD_4IN2_V2_Sleep(); // avoid sleeping on every render
#endif
}

// Draw screen indicator icons as a vertical stack on the right side
void drawScreenIndicator(ScreenPage currentScreen) {
    bool isNavigable = (currentScreen == ScreenPage::MAIN || 
                       currentScreen == ScreenPage::GRAPHS || 
                       currentScreen == ScreenPage::SETTINGS || 
                       currentScreen == ScreenPage::SENSOR_MAP);
    if (!isNavigable) {
        return;
    }

    struct ScreenIcon {
        ScreenPage screen;
        const unsigned char* icon;
    };

    ScreenIcon screens[4] = {
        {ScreenPage::MAIN, display_frame_15x15},
        {ScreenPage::GRAPHS, line_graph_15x15},
        {ScreenPage::SETTINGS, gear_15x15},
        {ScreenPage::SENSOR_MAP, map_15x15}
    };

    const int icon_size = 15;
    const int icon_spacing = 8; // vertical spacing
    const int icon_count = 4;
    const int margin = 8; // side margin
    // Place below headers
    const int top_margin = 30;

    // Right-aligned vertical stack at the top
    int start_x = DISPLAY_WIDTH - icon_size - margin;
    // Position at top, below headers
    int start_y = top_margin;

    for (int i = 0; i < icon_count; i++) {
        int icon_x = start_x;
        int icon_y = start_y + i * (icon_size + icon_spacing);

        // Clear the icon area + a little margin
        Paint_ClearWindows(icon_x - 20, icon_y - 2, icon_x + icon_size + 2, icon_y + icon_size + 6, WHITE);

        bool is_active = (screens[i].screen == currentScreen);

        // Draw icon
        Paint_DrawImage(screens[i].icon, icon_x, icon_y, icon_size, icon_size);

        // Draw '>' marker to the left for active icon
        if (is_active) {
            const char *marker = ">";
            int marker_x = icon_x - 10; // a bit to the left
            int marker_y = icon_y + (icon_size - Font12.Height) / 2;
            Paint_DrawString_EN(marker_x, marker_y, marker, &Font12, WHITE, BLACK);
        }
    }
}

#endif