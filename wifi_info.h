// wifi_info.h
#ifndef __WIFI_INFO_H__
#define __WIFI_INFO_H__

#include <Arduino.h>

struct struct_wifiInfo {
    char ssid[32];
    uint8_t encryptionType;
    int32_t RSSI;
    uint8_t channel;
    bool isHidden;
};

#endif // __WIFI_INFO_H__