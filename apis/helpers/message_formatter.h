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
enum DatalogFormatStatus {
	DATALOG_FORMAT_PLAIN,
	DATALOG_FORMAT_CPS,
	DATALOG_FORMAT_PAYLOAD_EMPTY,
	DATALOG_FORMAT_ENCRYPTION_FAILED,
	DATALOG_FORMAT_PAYLOAD_TOO_LARGE,
};

DatalogFormatStatus formatRobonomicsDatalogString(JsonDocument &data, String &datalog_data);

void addTimeAndSign(const String &data, String &signature, Robonomics *robonomics);

#endif // __MSG_FORMATTER_H__
