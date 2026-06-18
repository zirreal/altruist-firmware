#include "rws_devices_registry.h"
#include "../config_manager/config_helpers.h"
#include "../config_manager/config_defaults.h"
#include "../utils.h"
#include "../wifi_manager.h"
#include <Robonomics.h>
#include <vector>

namespace {

constexpr size_t kMaxRwsDevices = 32;
constexpr unsigned long kRetryIntervalMs = 30UL * 60UL * 1000UL;

bool isSs58Like(const String& address) {
	if (address.length() < 47 || address.length() > 50 || address[0] != '4') {
		return false;
	}
	const char* base58 = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
	for (unsigned i = 0; i < address.length(); ++i) {
		if (!strchr(base58, address[i])) {
			return false;
		}
	}
	return true;
}

bool rwsOwnerIsUnset(const char* owner) {
	if (owner == nullptr || owner[0] == '\0') {
		return true;
	}
	String normalized(owner);
	normalized.trim();
	if (normalized.length() == 0) {
		return true;
	}
	normalized.toLowerCase();
	return normalized == "not set";
}

void appendUnique(std::vector<std::string>& devices, const String& address) {
	if (!isSs58Like(address)) {
		return;
	}
	const std::string normalized = address.c_str();
	for (const auto& existing : devices) {
		if (existing == normalized) {
			return;
		}
	}
	if (devices.size() >= kMaxRwsDevices) {
		return;
	}
	devices.push_back(normalized);
}

void appendExtrasFromConfig(std::vector<std::string>& devices) {
	String extras = String(cfg::rws_devices_extra);
	extras.trim();
	if (extras.length() == 0) {
		return;
	}

	int start = 0;
	while (start < extras.length()) {
		int end = start;
		while (end < extras.length()) {
			const char ch = extras[end];
			if (ch == '\n' || ch == '\r' || ch == ',' || ch == ';') {
				break;
			}
			++end;
		}
		String token = extras.substring(start, end);
		token.trim();
		if (token.length() > 0) {
			appendUnique(devices, token);
		}
		start = end + 1;
	}
}

String buildDeviceListFingerprint(const std::vector<std::string>& devices) {
	String fingerprint;
	for (size_t i = 0; i < devices.size(); ++i) {
		if (i > 0) {
			fingerprint += '\n';
		}
		fingerprint += devices[i].c_str();
	}
	return fingerprint;
}

bool shouldRegisterDevices() {
#if defined(ALTRUIST_INSIDE)
	if (!cfg::standalone) {
		return false;
	}
#endif
	return true;
}

bool buildDeviceList(Robonomics* robonomics, std::vector<std::string>& devices) {
	devices.clear();
	if (robonomics == nullptr) {
		return false;
	}

	const String self_address = String(robonomics->getSs58Address());
	if (!isSs58Like(self_address)) {
		return false;
	}
	appendUnique(devices, self_address);

#if defined(ALTRUIST_URBAN)
	appendExtrasFromConfig(devices);
#endif

	return !devices.empty();
}

bool isLegacyExternalOwner(Robonomics* robonomics) {
	if (rwsOwnerIsUnset(cfg::rws_owner)) {
		return false;
	}
	const String self_address = String(robonomics->getSs58Address());
	const String owner = String(cfg::rws_owner);
	return isSs58Like(owner) &&
	       isSs58Like(self_address) &&
	       owner != self_address;
}

/** Update cfg::rws_owner in RAM; returns true if value changed. */
bool updateRwsOwnerInCfg(const char* owner_value) {
	if (owner_value == nullptr || owner_value[0] == '\0') {
		return false;
	}
	if (!isSs58Like(String(owner_value))) {
		return false;
	}
	if (strcmp(cfg::rws_owner, owner_value) == 0) {
		return false;
	}
	strncpy(cfg::rws_owner, owner_value, LEN_RWS_OWNER - 1);
	cfg::rws_owner[LEN_RWS_OWNER - 1] = '\0';
	return true;
}

bool persistRwsOwnerValue(const char* owner_value) {
	if (!updateRwsOwnerInCfg(owner_value)) {
		return false;
	}
	if (!writeConfig()) {
		debug_outln_error(F("[RWS] failed to save rws_owner to config"));
		return false;
	}
	debug_outln_info(F("[RWS] rws_owner set to: "), String(cfg::rws_owner));
	return true;
}

bool trySyncOwnerFromDevices(Robonomics* robonomics, const std::vector<std::string>* devices) {
	if (isLegacyExternalOwner(robonomics)) {
		return false;
	}
	if (devices != nullptr && !devices->empty()) {
		if (persistRwsOwnerValue(devices->front().c_str())) {
			return true;
		}
	}
	if (robonomics != nullptr && persistRwsOwnerValue(robonomics->getSs58Address())) {
		return true;
	}
	String hash = String(cfg::rws_devices_registered_hash);
	hash.trim();
	if (hash.length() == 0) {
		return false;
	}
	const int newline = hash.indexOf('\n');
	String first_device = (newline >= 0) ? hash.substring(0, newline) : hash;
	first_device.trim();
	return persistRwsOwnerValue(first_device.c_str());
}

enum class SetDevicesSkipReason : uint8_t {
	Send2RobonomicsDisabled,
	RwsAutoRegisterDisabled,
	WaitingForWifi,
	WaitingForSntp,
	Ss58NotReady,
	AlreadyRegistered,
	LegacyExternalOwner,
	BundleInsight,
};

void logSetDevicesSkipOnce(SetDevicesSkipReason reason) {
	static bool logged[8] = {};
	const uint8_t idx = static_cast<uint8_t>(reason);
	if (idx >= sizeof(logged) / sizeof(logged[0]) || logged[idx]) {
		return;
	}
	logged[idx] = true;

	const __FlashStringHelper* message = F("unknown");
	switch (reason) {
	case SetDevicesSkipReason::Send2RobonomicsDisabled:
		message = F("send2robonomics disabled");
		break;
	case SetDevicesSkipReason::RwsAutoRegisterDisabled:
		message = F("rws_auto_register disabled");
		break;
	case SetDevicesSkipReason::WaitingForWifi:
		message = F("waiting for WiFi");
		break;
	case SetDevicesSkipReason::WaitingForSntp:
		message = F("waiting for SNTP time");
		break;
	case SetDevicesSkipReason::Ss58NotReady:
		message = F("device SS58 not ready");
		break;
	case SetDevicesSkipReason::AlreadyRegistered:
		message = F("already registered");
		break;
	case SetDevicesSkipReason::LegacyExternalOwner:
		message = F("legacy external owner");
		break;
	case SetDevicesSkipReason::BundleInsight:
		message = F("bundle Insight (Urban registers)");
		break;
	}
	debug_outln_info(String(F("[RWS] set_devices skipped: ")) + String(message));
}

void logLegacyOwnerModeOnce(Robonomics* robonomics) {
	static bool logged = false;
	if (logged || !isLegacyExternalOwner(robonomics)) {
		return;
	}
	logged = true;
	debug_outln_info(F("[RWS] Legacy external owner configured (datalog uses owner; set_devices skipped)"));
	debug_outln_verbose(F("[RWS] owner: "), String(cfg::rws_owner));
	debug_outln_verbose(F("[RWS] self : "), String(robonomics->getSs58Address()));
}

bool extrinsicAccepted(Robonomics* robonomics, const char* result) {
	if (robonomics->lastExtrinsicOk()) {
		return true;
	}
	const String result_s = String(result ? result : "");
	if (result_s.length() == 0 || result_s == F("error")) {
		return false;
	}
	if (result_s.startsWith("{") &&
	    (result_s.indexOf("\"code\"") >= 0 || result_s.indexOf("\"message\"") >= 0)) {
		return false;
	}
	return true;
}

void saveRwsRegistrationSuccess(Robonomics* robonomics, const String& fingerprint) {
	bool owner_changed = false;
	if (robonomics != nullptr) {
		owner_changed = updateRwsOwnerInCfg(robonomics->getSs58Address());
	}
	bool hash_changed = false;
	if (strcmp(cfg::rws_devices_registered_hash, fingerprint.c_str()) != 0) {
		strncpy(cfg::rws_devices_registered_hash, fingerprint.c_str(), LEN_RWS_DEVICES_REGISTERED_HASH - 1);
		cfg::rws_devices_registered_hash[LEN_RWS_DEVICES_REGISTERED_HASH - 1] = '\0';
		hash_changed = true;
	}
	if (!owner_changed && !hash_changed) {
		return;
	}
	if (!writeConfig()) {
		debug_outln_error(F("[RWS] failed to save registration result to config"));
		return;
	}
	if (owner_changed) {
		debug_outln_info(F("[RWS] rws_owner set to: "), String(cfg::rws_owner));
	}
}

void handleAlreadyRegistered(Robonomics* robonomics, const std::vector<std::string>& devices) {
	if (!trySyncOwnerFromDevices(robonomics, &devices) && rwsOwnerIsUnset(cfg::rws_owner)) {
		debug_outln_error(F("[RWS] owner still unset; check config save"));
	}
	logSetDevicesSkipOnce(SetDevicesSkipReason::AlreadyRegistered);
}

void syncOwnerIfNeeded(Robonomics* robonomics) {
	if (!shouldRegisterDevices() || robonomics == nullptr) {
		return;
	}
	if (isLegacyExternalOwner(robonomics)) {
		logLegacyOwnerModeOnce(robonomics);
		return;
	}
	if (trySyncOwnerFromDevices(robonomics, nullptr)) {
		return;
	}
	if (!rwsOwnerIsUnset(cfg::rws_owner)) {
		return;
	}
	static bool logged_ss58_wait = false;
	if (!logged_ss58_wait) {
		logged_ss58_wait = true;
		debug_outln_info(F("[RWS] rws_owner unset; waiting for device SS58"));
	}
}

void runEnsureRwsDevicesRegistered(Robonomics* robonomics) {
	static unsigned long last_attempt_ms = 0;
	static String throttled_fingerprint;

	syncOwnerIfNeeded(robonomics);

	if (!shouldRegisterDevices()) {
		logSetDevicesSkipOnce(SetDevicesSkipReason::BundleInsight);
		return;
	}
	if (!cfg::send2robonomics) {
		logSetDevicesSkipOnce(SetDevicesSkipReason::Send2RobonomicsDisabled);
		return;
	}
	if (!cfg::rws_auto_register) {
		logSetDevicesSkipOnce(SetDevicesSkipReason::RwsAutoRegisterDisabled);
		return;
	}
	if (!wifiStaLinkReady() || WiFi.localIP()[0] == 0) {
		logSetDevicesSkipOnce(SetDevicesSkipReason::WaitingForWifi);
		return;
	}
	struct tm timeinfo;
	if (!getLocalTime(&timeinfo)) {
		logSetDevicesSkipOnce(SetDevicesSkipReason::WaitingForSntp);
		return;
	}
	if (isLegacyExternalOwner(robonomics)) {
		logLegacyOwnerModeOnce(robonomics);
		logSetDevicesSkipOnce(SetDevicesSkipReason::LegacyExternalOwner);
		return;
	}

	std::vector<std::string> devices;
	if (!buildDeviceList(robonomics, devices)) {
		logSetDevicesSkipOnce(SetDevicesSkipReason::Ss58NotReady);
		return;
	}

	const String fingerprint = buildDeviceListFingerprint(devices);
	if (fingerprint.length() == 0) {
		return;
	}
	if (fingerprint == String(cfg::rws_devices_registered_hash)) {
		handleAlreadyRegistered(robonomics, devices);
		return;
	}

	const unsigned long now_ms = millis();
	if (last_attempt_ms != 0 &&
	    msSince(last_attempt_ms) < kRetryIntervalMs &&
	    fingerprint == throttled_fingerprint) {
		return;
	}
	last_attempt_ms = now_ms;

	debug_outln_info(String(F("[RWS] set_devices submitting ")) + String(devices.size()) + F(" device(s)"));
	for (size_t i = 0; i < devices.size(); ++i) {
		debug_outln_verbose(F("[RWS] device: "), String(devices[i].c_str()));
	}

	const char* result = robonomics->sendRWSSetDevices(devices);
	if (extrinsicAccepted(robonomics, result)) {
		saveRwsRegistrationSuccess(robonomics, fingerprint);
		throttled_fingerprint = "";
		debug_outln_info(F("[RWS] set_devices OK"));
		debug_outln_verbose(F("[RWS] result: "), String(result ? result : ""));
		return;
	}

	throttled_fingerprint = fingerprint;
	debug_outln_error(F("[RWS] set_devices FAILED"));
	if (!robonomics->lastExtrinsicOk()) {
		debug_outln_verbose(F("[RWS] error: "), String(robonomics->lastExtrinsicErrorMessage()));
	} else {
		debug_outln_verbose(F("[RWS] result: "), String(result ? result : ""));
	}
}

}  // namespace

void repairInconsistentRwsRegistrationState() {
#if defined(ALTRUIST_RWS_RESET_REGISTRATION)
	if (cfg::rws_devices_registered_hash[0] != '\0') {
		debug_outln_info(F("[RWS] ALTRUIST_RWS_RESET_REGISTRATION: clearing registration hash"));
		cfg::rws_devices_registered_hash[0] = '\0';
		writeConfig();
	}
	return;
#endif

	if (cfg::rws_devices_registered_hash[0] == '\0') {
		return;
	}
	String owner = String(cfg::rws_owner);
	owner.trim();
	if (owner.length() > 0 && !owner.equalsIgnoreCase(F("not set"))) {
		return;
	}
	debug_outln_info(F("[RWS] owner unset but registration hash present; clearing hash for re-registration"));
	cfg::rws_devices_registered_hash[0] = '\0';
	writeConfig();
}

void ensureRwsDevicesRegistered(Robonomics* robonomics) {
	runEnsureRwsDevicesRegistered(robonomics);
}
