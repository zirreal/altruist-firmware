#ifdef ALTRUIST_INSIDE

#include "loading.h"
#include "../paint_driver/GUI_Paint.h"
#include "../utils.h"
#include "../driver/EPD.h"
// #include "../icons/icons/loading_image.h"

void showLoadingPage(UBYTE *BlackImage) {
    Paint_DrawString_EN_Center("Loading...", &Font24, WHITE, BLACK);
    // Draw loading image (scaled to fit 400x300 display)
    // No need to clear - image covers entire screen
    // Paint_DrawImage(loading_image, 0, 0, 400, 300);
}

#endif