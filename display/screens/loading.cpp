#ifdef ALTRUIST_INSIDE

#include "loading.h"
#include "../paint_driver/GUI_Paint.h"
#include "../utils.h"
#include "../driver/EPD.h"
#include "../icons/icons/loading_image_2.h"

void showLoadingPage(UBYTE *BlackImage) {
    // Clear screen
    Paint_Clear(WHITE);
    
    // Draw loading image (scaled to fit 400x300 display)
    Paint_DrawImage(loading_image_2, 0, 0, 400, 300);
}

#endif