#ifndef __OTA_UPDATE_H__
#define __OTA_UPDATE_H__

#include "utils.h"

bool downloadAndUpdate(const char* url, const String& expectedMD5, device_status_t &deviceStatus);
void twoStageOTAUpdate(device_status_t &deviceStatus, bool manual = false);

#endif // __OTA_UPDATE_H__