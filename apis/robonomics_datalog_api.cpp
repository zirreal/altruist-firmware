#include "robonomics_datalog_api.h"
#include "../config_manager/config_helpers.h"
#include "helpers/message_formatter.h"
#include "../utils.h"

void RobonomicsDatalogAPI::setup() {
    api_name = "Robonomics Datalog";
    timeout = getConfigUintValue("datalog_sending_intervall_ms");
    rws_owner = getConfigStringValue("rws_owner");
    private_key = getConfigStringValue("private_key");
    robonomics_public_node = getConfigStringValue("robonomics_public_node");
    if (strcmp(private_key.c_str(), "Not Set") == 0) {
		robonomics->generateAndSetPrivateKey();
		saveRobonomicsPrivateKey(private_key.c_str());
	} else {
		robonomics->setPrivateKey(private_key.c_str());
	}
    robonomics->setup(robonomics_public_node);
	debug_outln_info(F("Robonomics private key: "), String(robonomics->getPrivateKey()));
    debug_outln_info(F("Robonomics datalog API is ready with sending interval (sec): "), String(timeout/1000));
}

void RobonomicsDatalogAPI::_send(JsonDocument &data) {
    String datalog_data;
    formatRobonomicsString(data, datalog_data);
    debug_outln_info(F("Start sending datalog: "), datalog_data);
    const char* res = robonomics->sendRWSDatalogRecord(datalog_data.c_str(), rws_owner.c_str());
	debug_outln_info(F("Datalog result: "), res);
    is_ok = (strcmp(res, "error") != 0);
}
