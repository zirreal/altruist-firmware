#ifdef ALTRUIST_INSIDE

#include "logo.h"
#include "../icons/icons/icons_200x200.h"
#include "../paint_driver/GUI_Paint.h"
#include "../driver/EPD.h"
#include <string.h>

// Logo bitmap is 200x158 (not 200x200).
#define LOGO_SRC_WIDTH  200
#define LOGO_SRC_HEIGHT 158
#define LOGO_DRAWN_WIDTH  LOGO_SRC_WIDTH
#define LOGO_DRAWN_HEIGHT LOGO_SRC_HEIGHT

static void drawLogoFullNoLine(uint16_t xStart, uint16_t yStart) {
    const unsigned char *buf = robo_hw_logo_black_200x200;
    const uint16_t w = LOGO_SRC_WIDTH;
    const uint16_t h = LOGO_SRC_HEIGHT;
    UWORD byte_width = (w % 8) ? (w / 8 + 1) : (w / 8);
    for (uint16_t y = 0; y < h; y++) {
        for (uint16_t x = 0; x < w; x++) {
            UWORD byte_index = (y * byte_width) + (x / 8);
            UBYTE bit = 0x80 >> (x % 8);
            UWORD color = (buf[byte_index] & bit) ? WHITE : BLACK;
            Paint_SetPixel(xStart + x, yStart + y, color);
        }
    }
}

void showLogoPage() {
    Paint_Clear(WHITE);

    const uint16_t top_margin = 16;

    // Product name above logo
    const char *product = "Altruist Insight";
    uint16_t product_width = strlen(product) * Font12.Width;
    uint16_t product_x = (DISPLAY_WIDTH - product_width) / 2;
    uint16_t product_y = top_margin;
    Paint_DrawString_EN(product_x, product_y, product, &Font12, WHITE, BLACK);

    // Full logo 200x158
    uint16_t logo_x = (DISPLAY_WIDTH - LOGO_DRAWN_WIDTH) / 2;
    uint16_t logo_y = product_y + Font12.Height + 8;
    drawLogoFullNoLine(logo_x, logo_y);

    // Status block below logo
    const uint16_t text_block_y = logo_y + LOGO_DRAWN_HEIGHT + 12;
    const char *msg1 = "WiFi credentials cleared";
    const char *msg2 = "Restarting device...";
    uint16_t msg1_width = strlen(msg1) * Font16.Width;
    uint16_t msg2_width = strlen(msg2) * Font12.Width;
    uint16_t msg1_x = (DISPLAY_WIDTH - msg1_width) / 2;
    uint16_t msg2_x = (DISPLAY_WIDTH - msg2_width) / 2;
    uint16_t msg1_y = text_block_y;
    uint16_t msg2_y = msg1_y + Font16.Height + 10;
    Paint_DrawString_EN(msg1_x, msg1_y, msg1, &Font16, WHITE, BLACK);
    Paint_DrawString_EN(msg2_x, msg2_y, msg2, &Font12, WHITE, BLACK);
}

#endif