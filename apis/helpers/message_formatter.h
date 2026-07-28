#ifndef __MSG_FORMATTER_H__
#define __MSG_FORMATTER_H__

#include <ArduinoJson.h>
#include <Robonomics.h>

/** Map / connectivity: per-field CPS encrypt where configured. */
void formatRobonomicsString(JsonDocument &data, String &datalog_data);

/**
 * On-chain datalog: plain CSV, then one CPS blob if any encrypt flag is on
 * (fits Robonomics 512-byte record limit). Map/connectivity unchanged.
 */
void formatRobonomicsDatalogString(JsonDocument &data, String &datalog_data);

void addTimeAndSign(const String &data, String &signature, Robonomics *robonomics);

#endif // __MSG_FORMATTER_H__