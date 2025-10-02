#ifdef ALTRUIST_INSIDE

#ifndef DISPLAY_COMMON_H
#define DISPLAY_COMMON_H

#include "../driver/EPD.h"

void initAndClearScreen();
void createNewImage(UBYTE *&BlackImage);
void showImageFast(UBYTE *&BlackImage);
void showImageLong(UBYTE *&BlackImage);

#ifdef DISPLAY_4IN2
#define EPD_DisplayFull(img)   EPD_4IN2_V2_Display(img)
#define EPD_DisplayPartial(img,xs,ys,xe,ye)  EPD_4IN2_V2_PartialDisplay(img,xs,ys,xe,ye)
#endif

#endif
#endif