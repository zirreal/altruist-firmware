#include <Arduino.h>

#ifdef ESP32

#define CYCLETIME 120
#define MIC_OFFSET 1.5

bool initI2sSound();
void fetchSensorI2sSound(uint8_t *max_noise, float *mean_noise);

#endif // ESP32