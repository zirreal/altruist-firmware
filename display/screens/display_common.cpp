#ifdef ALTRUIST_INSIDE

#include "display_common.h"
#include "../paint_driver/GUI_Paint.h"
#include "../display_manager.h"
// Icon sets
#include "../icons/icons/icons_10x10.h"
#include "../icons/icons/icons_15x15.h"

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

// Helper function to draw an icon with inverted colors (white icon on black background)
static void Paint_DrawImageInverted(const unsigned char *image_buffer, UWORD xStart, UWORD yStart, UWORD W_Image, UWORD H_Image) {
    UWORD x, y;
    UWORD byte_width = (W_Image % 8) ? (W_Image / 8 + 1) : (W_Image / 8);

    for (y = 0; y < H_Image; y++) {
        for (x = 0; x < W_Image; x++) {
            UWORD byte_index = (y * byte_width) + (x / 8);
            UBYTE byte = image_buffer[byte_index];
            UBYTE bit = 0x80 >> (x % 8);  // MSB first

            // Invert: if bit is set (would be WHITE), draw BLACK; if bit is clear (would be BLACK), draw WHITE
            UWORD color = (byte & bit) ? BLACK : WHITE;

            Paint_SetPixel(xStart + x, yStart + y, color);
        }
    }
}

// Helper function to draw a rounded rectangle
static void Paint_DrawRoundedRectangle(UWORD xStart, UWORD yStart, UWORD xEnd, UWORD yEnd, 
                                        UWORD color, UWORD radius, DOT_PIXEL line_width, DRAW_FILL draw_fill) {
    if (xStart > Paint.Width || yStart > Paint.Height ||
        xEnd > Paint.Width || yEnd > Paint.Height) {
        return;
    }

    if (draw_fill == DRAW_FILL_FULL) {
        // Fill the main rectangular area
        Paint_DrawRectangle(xStart + radius, yStart, xEnd - radius, yEnd, color, line_width, DRAW_FILL_FULL);
        Paint_DrawRectangle(xStart, yStart + radius, xEnd, yEnd - radius, color, line_width, DRAW_FILL_FULL);
        
        // Fill the corner areas with filled circles (only the parts that are inside the rectangle)
        // Top-left
        for (int y = 0; y < radius; y++) {
            for (int x = 0; x < radius; x++) {
                int dx = x - radius;
                int dy = y - radius;
                if (dx*dx + dy*dy <= radius*radius) {
                    Paint_SetPixel(xStart + x, yStart + y, color);
                }
            }
        }
        // Top-right
        for (int y = 0; y < radius; y++) {
            for (int x = 0; x < radius; x++) {
                int dx = x;
                int dy = y - radius;
                if (dx*dx + dy*dy <= radius*radius) {
                    Paint_SetPixel(xEnd - radius + x, yStart + y, color);
                }
            }
        }
        // Bottom-left
        for (int y = 0; y < radius; y++) {
            for (int x = 0; x < radius; x++) {
                int dx = x - radius;
                int dy = y;
                if (dx*dx + dy*dy <= radius*radius) {
                    Paint_SetPixel(xStart + x, yEnd - radius + y, color);
                }
            }
        }
        // Bottom-right
        for (int y = 0; y < radius; y++) {
            for (int x = 0; x < radius; x++) {
                int dx = x;
                int dy = y;
                if (dx*dx + dy*dy <= radius*radius) {
                    Paint_SetPixel(xEnd - radius + x, yEnd - radius + y, color);
                }
            }
        }
    } else {
        // Draw outline - for small radius, use simple rectangle with rounded corners
        // Top and bottom horizontal lines
        Paint_DrawLine(xStart + radius, yStart, xEnd - radius, yStart, color, line_width, LINE_STYLE_SOLID);
        Paint_DrawLine(xStart + radius, yEnd, xEnd - radius, yEnd, color, line_width, LINE_STYLE_SOLID);
        // Left and right vertical lines
        Paint_DrawLine(xStart, yStart + radius, xStart, yEnd - radius, color, line_width, LINE_STYLE_SOLID);
        Paint_DrawLine(xEnd, yStart + radius, xEnd, yEnd - radius, color, line_width, LINE_STYLE_SOLID);
        
        // Draw corner arcs using pixel-by-pixel approach
        // Top-left corner
        for (int y = 0; y < radius; y++) {
            for (int x = 0; x < radius; x++) {
                int dx = x - radius;
                int dy = y - radius;
                int dist_sq = dx*dx + dy*dy;
                if (dist_sq >= (radius-1)*(radius-1) && dist_sq <= radius*radius) {
                    Paint_SetPixel(xStart + x, yStart + y, color);
                }
            }
        }
        // Top-right corner
        for (int y = 0; y < radius; y++) {
            for (int x = 0; x < radius; x++) {
                int dx = x;
                int dy = y - radius;
                int dist_sq = dx*dx + dy*dy;
                if (dist_sq >= (radius-1)*(radius-1) && dist_sq <= radius*radius) {
                    Paint_SetPixel(xEnd - radius + x, yStart + y, color);
                }
            }
        }
        // Bottom-left corner
        for (int y = 0; y < radius; y++) {
            for (int x = 0; x < radius; x++) {
                int dx = x - radius;
                int dy = y;
                int dist_sq = dx*dx + dy*dy;
                if (dist_sq >= (radius-1)*(radius-1) && dist_sq <= radius*radius) {
                    Paint_SetPixel(xStart + x, yEnd - radius + y, color);
                }
            }
        }
        // Bottom-right corner
        for (int y = 0; y < radius; y++) {
            for (int x = 0; x < radius; x++) {
                int dx = x;
                int dy = y;
                int dist_sq = dx*dx + dy*dy;
                if (dist_sq >= (radius-1)*(radius-1) && dist_sq <= radius*radius) {
                    Paint_SetPixel(xEnd - radius + x, yEnd - radius + y, color);
                }
            }
        }
    }
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

    struct NavIcon {
        const unsigned char* icon;
        bool isPageIcon;
        ScreenPage screen; // Only used for page icons
    };


    // Navigation items
    NavIcon navItems[5] = {
        {home_nav_15x15,    true,  ScreenPage::MAIN},
        {chart_15x15,       true,  ScreenPage::GRAPHS},      // chart icon for graphs
        {map_nav_15x15,     true,  ScreenPage::SENSOR_MAP},
        {settings_15x15,    true,  ScreenPage::SETTINGS},    // settings icon for settings page
        {new_switch_15x15,  false, ScreenPage::MAIN}         // bottom switch icon
    };

    const int icon_size = 10; // small icon size
    const int large_icon_size = 15; // large icon size for page icons
    const int icon_spacing = 4; // vertical spacing between icons
    const int nav_item_count = 5;
    const int sidebar_width = 28; // width of the navigation sidebar
    const int button_height = icon_size + 6; // height of each button (icon + padding)
    const int border_radius = 0; 
    const int padding = 4; // padding inside the rounded rectangle
    const int margin = 1; // margin from right and bottom edges of screen 
    
    // Reserve space at the top for the main header (icon/time/date) + its bottom border
    const int header_reserved_height = 27;

    // Calculate sidebar position (right side of screen, starting below header)
    int sidebar_x = DISPLAY_WIDTH - sidebar_width - margin;
    int sidebar_y = header_reserved_height;
    int sidebar_height = DISPLAY_HEIGHT - header_reserved_height - margin;


    Paint_DrawRectangle(sidebar_x, sidebar_y, 
                        sidebar_x + sidebar_width, sidebar_y + sidebar_height - 1,
                        WHITE, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    Paint_DrawRectangle(sidebar_x, sidebar_y, 
                        sidebar_x + sidebar_width - 1, sidebar_y + sidebar_height - 1,
                        BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);

    // Calculate icon positions
    int small_icon_x = sidebar_x + (sidebar_width - icon_size) / 2;
    int large_icon_x = sidebar_x + (sidebar_width - large_icon_size) / 2;
    const int page_icon_gap = 12; // gap between page icons 
    

    int current_y = sidebar_y + 4;
    
    for (int i = 0; i <= 3; i++) {
        bool is_active = (navItems[i].screen == currentScreen);
        
        // Page icons are all 15x15
        bool is_large_icon = true;
        int current_icon_size = is_large_icon ? large_icon_size : icon_size;
        int current_icon_x = is_large_icon ? large_icon_x : small_icon_x;
        
        // Draw black button background for active page
        if (is_active) {
            const int active_padding = 4; // extra top and bottom padding for active icon
            int button_x = sidebar_x + 1; // no padding from border 
            int button_y = current_y - active_padding; // more top padding
            int button_w = sidebar_width - 2; // no padding from border 
            int button_h = button_height + (active_padding * 2); // more bottom padding
            Paint_DrawRectangle(button_x, button_y, 
                               button_x + button_w - 1, button_y + button_h - 1,
                               BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
            // Draw white (inverted) icon on black background
            Paint_DrawImageInverted(navItems[i].icon, current_icon_x, current_y, current_icon_size, current_icon_size);
        } else {
            // Draw black icon on white background
            Paint_DrawImage(navItems[i].icon, current_icon_x, current_y, current_icon_size, current_icon_size);
        }
        
        // Move to next icon position
        current_y += button_height + page_icon_gap;
    }
    
    // Bottom: switch icon 
    int bottom_icon_y = sidebar_y + sidebar_height - padding - button_height;
    Paint_DrawImage(navItems[4].icon, large_icon_x, bottom_icon_y, large_icon_size, large_icon_size);
}

#endif