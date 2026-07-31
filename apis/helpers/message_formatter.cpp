#include "message_formatter.h"
#include "value_crypto.h"
#include "../../utils.h"
#include "../../config_manager/config_defaults.h"
#include "../../sensors/sensor_names.h"

#include <Ed25519.h>
#include <string.h>

#define SCD4X_WARMUP_SEC 360

// Connectivity Keypair.verify() checks the raw message. ESPRobonomicsClient::signMessage
// goes through doSign(), which Blake2b-hashes payloads >256 bytes (Substrate extrinsic
// rule). CPS e.* values push the signed string over that limit → signature mismatch →
// HTTP 500. Map/custom HTTP must always sign the raw bytes.
static bool parseHex32Key(const char *hex, uint8_t out[32]) {
	if (!hex) {
		return false;
	}
	size_t n = strlen(hex);
	if (n >= 2 && hex[0] == '0' && (hex[1] == 'x' || hex[1] == 'X')) {
		hex += 2;
		n -= 2;
	}
	if (n != 64) {
		return false;
	}
	for (size_t i = 0; i < 32; i++) {
		auto nib = [](char c) -> int {
			if (c >= '0' && c <= '9') return c - '0';
			if (c >= 'a' && c <= 'f') return c - 'a' + 10;
			if (c >= 'A' && c <= 'F') return c - 'A' + 10;
			return -1;
		};
		const int hi = nib(hex[i * 2]);
		const int lo = nib(hex[i * 2 + 1]);
		if (hi < 0 || lo < 0) {
			return false;
		}
		out[i] = static_cast<uint8_t>((hi << 4) | lo);
	}
	return true;
}

static bool signMessageRaw(const String &message, String &signature, Robonomics *robonomics) {
	if (!robonomics) {
		return false;
	}
	uint8_t sk[32];
	uint8_t pk[32];
	if (!parseHex32Key(robonomics->getPrivateKey(), sk)) {
		debug_outln_error(F("[Map] Cannot parse device private key for raw sign"));
		return false;
	}
	Ed25519::derivePublicKey(pk, sk);

	uint8_t sig[64];
	Ed25519::sign(sig, sk, pk, reinterpret_cast<const uint8_t *>(message.c_str()), message.length());

	signature = "";
	signature.reserve(128);
	for (size_t i = 0; i < sizeof(sig); i++) {
		if (sig[i] < 0x10) {
			signature += '0';
		}
		signature += String(sig[i], HEX);
	}
	return true;
}

static String maybeEncryptValue(const String &value, bool should_encrypt) {
	if (!should_encrypt || value.isEmpty()) {
		return value;
	}
	return valueCryptoEncryptValue(value);
}

/** Robonomics parachain datalog pallet hard limit (see MaximumMessageSize). */
static constexpr size_t DATALOG_CHAIN_MAX_BYTES = 512;
static constexpr size_t DATALOG_CHAIN_SAFE_BYTES = 480;

static bool anyMetricEncryptionEnabled() {
	return cfg::encrypt_pm || cfg::encrypt_noise || cfg::encrypt_temperature || cfg::encrypt_humidity ||
	       cfg::encrypt_pressure || cfg::encrypt_co2 || cfg::encrypt_co || cfg::encrypt_radiation ||
	       cfg::encrypt_o3 || cfg::encrypt_no2 || cfg::encrypt_fast_aqi || cfg::encrypt_epa_aqi;
}

static void appendRobonomicsFields(JsonDocument &data, String &out, bool per_field_encrypt) {
	out = "";

	bool has_scd4x = !data[SCD4X_SENSOR_NAME].isNull();
	bool use_bme680_for_temp_hum = !has_scd4x || (millis() / 1000 < SCD4X_WARMUP_SEC);

	for (JsonPair sensor : data.as<JsonObject>()) {
        String sensor_name = sensor.key().c_str();

        JsonObject sensorData = sensor.value().as<JsonObject>();

        for (JsonPair measurement : sensorData) {
            String type = measurement.key().c_str();
            JsonObject measurementData = measurement.value().as<JsonObject>();
            String value;
			if (measurementData["value"].is<uint8_t>()) {
				value = String(measurementData["value"].as<uint8_t>());
			}else if (measurementData["value"].is<float>()) {
				value = String(measurementData["value"].as<float>(), 2);
			} else {
				value = measurementData["value"].as<String>();
			}

            bool is_bme680 = (sensor_name == BME680_SENSOR_NAME);
            bool is_scd4x = (sensor_name == SCD4X_SENSOR_NAME);
            bool skip_temp_hum = (is_bme680 && !use_bme680_for_temp_hum)
                              || (is_scd4x && use_bme680_for_temp_hum);

			const bool encrypt_climate = cfg::encrypt_temperature || cfg::encrypt_humidity;

			if (type == "P1" && cfg::share_pm) {
				out += "p1:" + maybeEncryptValue(value, per_field_encrypt && cfg::encrypt_pm) + ",";
			} else if (type == "P2" && cfg::share_pm) {
				out += "p2:" + maybeEncryptValue(value, per_field_encrypt && cfg::encrypt_pm) + ",";
			} else if (type == "noiseMax" && cfg::share_noise) {
				out += "nm:" + maybeEncryptValue(value, per_field_encrypt && cfg::encrypt_noise) + ",";
			} else if (type == "noiseAvg" && cfg::share_noise) {
				out += "na:" + maybeEncryptValue(value, per_field_encrypt && cfg::encrypt_noise) + ",";
			} else if (type == "temperature" && cfg::share_temperature && out.indexOf("t:") == -1 &&
				   !skip_temp_hum) {
				out += "t:" + maybeEncryptValue(value, per_field_encrypt && encrypt_climate) + ",";
			} else if (type == "pressure" && cfg::share_pressure) {
				out += "p:" + maybeEncryptValue(value, per_field_encrypt && cfg::encrypt_pressure) + ",";
			} else if (type == "humidity" && cfg::share_humidity && out.indexOf("h:") == -1 && !skip_temp_hum) {
				out += "h:" + maybeEncryptValue(value, per_field_encrypt && encrypt_climate) + ",";
			} else if (type == "radiation") {
				out += "gc:" + maybeEncryptValue(value, per_field_encrypt && cfg::encrypt_radiation) + ",";
			} else if (type == "co2" && cfg::share_co2) {
				out += "co2:" + maybeEncryptValue(value, per_field_encrypt && cfg::encrypt_co2) + ",";
			} else if (type == "co" && cfg::share_co) {
				out += "co:" + maybeEncryptValue(value, per_field_encrypt && cfg::encrypt_co) + ",";
			} else if (type == "o3" && cfg::share_o3) {
				out += "o3:" + maybeEncryptValue(value, per_field_encrypt && cfg::encrypt_o3) + ",";
			} else if (type == "no2" && cfg::share_no2) {
				out += "no2:" + maybeEncryptValue(value, per_field_encrypt && cfg::encrypt_no2) + ",";
			} else if (type == "fast_aqi" && cfg::share_fast_aqi) {
				out += "fa:" + maybeEncryptValue(value, per_field_encrypt && cfg::encrypt_fast_aqi) + ",";
			} else if (type == "epa_aqi" && cfg::share_epa_aqi) {
				out += "ea:" + maybeEncryptValue(value, per_field_encrypt && cfg::encrypt_epa_aqi) + ",";
			}
		}
	}
	if (out.length() > 0) {
		out.remove(out.length() - 1);
	}
}

void formatRobonomicsString(JsonDocument &data, String &datalog_data) {
	appendRobonomicsFields(data, datalog_data, true);
	debug_outln_info(F("Map sensor data: "), datalog_data);
}

void formatRobonomicsDatalogString(JsonDocument &data, String &datalog_data) {
	String plain;
	appendRobonomicsFields(data, plain, false);
	if (plain.isEmpty()) {
		datalog_data = plain;
		debug_outln_info(F("[Datalog] Plain record (empty): "), datalog_data);
		return;
	}

	if (!anyMetricEncryptionEnabled()) {
		datalog_data = plain;
		debug_outln_info(F("[Datalog] Plain record: "), datalog_data);
		return;
	}

	const String encrypted = valueCryptoEncryptValue(plain);
	if (encrypted.startsWith(VALUE_CRYPTO_CPS_PREFIX) &&
	    encrypted.length() <= DATALOG_CHAIN_SAFE_BYTES) {
		datalog_data = encrypted;
		debug_outln_info(String(F("[Datalog] Bulk encrypted record (")) + String(datalog_data.length()) +
					 F(" bytes): ") + datalog_data);
		return;
	}

	if (encrypted.startsWith(VALUE_CRYPTO_CPS_PREFIX) && encrypted.length() > DATALOG_CHAIN_SAFE_BYTES) {
		debug_outln_error(F("[Datalog] Bulk encrypted record too large for chain limit"));
		debug_outln_verbose(String(F("[Datalog] Record size: ")) + String(encrypted.length()) + F(" bytes (max ") +
				    String(DATALOG_CHAIN_MAX_BYTES) + F(")"));
	} else {
		debug_outln_error(F("[Datalog] Bulk encrypt failed; not sending plaintext fallback"));
	}
	datalog_data = "";
}

void addTimeAndSign(const String &data, String &signature, Robonomics *robonomics) {
  // Get the local time.
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    debug_outln_verbose(F("Failed to obtain time"));
    return;
  }
  debug_outln_info(F("Local time: "), timeinfo.tm_hour);
  
  // Convert local time to a Unix timestamp.
  time_t timestamp = mktime(&timeinfo);
  String timestampStr = String(timestamp);
  
  // Remove the last two digits from the timestamp string.
  if (timestampStr.length() > 2) {
    timestampStr = timestampStr.substring(0, timestampStr.length() - 2);
  }
  
  debug_outln_info(F("Modified Timestamp: "), timestampStr);

  String messageWithTimestamp = data + ",time:" + timestampStr;

  debug_outln_info(F("Message to sign: "), messageWithTimestamp);
  debug_outln_info(F("Message length: "), String(messageWithTimestamp.length()));

  if (!signMessageRaw(messageWithTimestamp, signature, robonomics)) {
    signature = "";
    debug_outln_error(F("[Map] Raw Ed25519 sign failed"));
    return;
  }

  debug_outln_info(F("Signature: "), signature);
}
