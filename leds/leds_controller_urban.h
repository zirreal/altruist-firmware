#ifdef ALTRUIST_URBAN

#ifndef __LEDS_CONTROLLER_URBAN_H__
#define __LEDS_CONTROLLER_URBAN_H__

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

#define LED_COUNT 2
#define MAX_BLINK_COUNT 1

enum class LedMode {
    NONE,
    BLINK_RED,
    BLUE,
    PROVISIONING,
    RESETTING,
    BLINK_GREEN,
    GREEN
};

class LedControllerUrban {
    public:
        LedControllerUrban();
        void init();
        void setMode(LedMode mode);
        void process();

    private:
        LedMode current_mode = LedMode::NONE;
        bool mode_changed = false;
        Adafruit_NeoPixel pixels;
        uint32_t connection_color = 0;
        bool connection_color_set = false;

        void _setAllPixels(uint32_t color);
};

#endif

#endif