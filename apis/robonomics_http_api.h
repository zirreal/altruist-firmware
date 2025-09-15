#ifndef __ROBONOMICS_API_H__
#define __ROBONOMICS_API_H__

#include "api.h"
#include "WiFiClient.h"
#include "HTTPClient.h"
#include "../robonomics_servers.h"
#include <Robonomics.h>

static const char URL_ROBONOMICS[] PROGMEM = "/";
#define PORT_ROBONOMICS 65

class RobonomicsHTTPAPI : public API {
public:
  // Constructor with a default timeout value (e.g., 1000 milliseconds)
  void setup() override;

  void setRobonomcis(Robonomics* robonomics) {
    this->robonomics = robonomics;
  }

private:
    WiFiClient* _client;
    String esp_chipid;
    String current_reg;
    String donated_by;
    String rws_owner;
    Robonomics* robonomics;
    void _send(JsonDocument &data) override;
    void POSTRequest(const String& data, const char* host);
    int chooseRobonomicsServer(bool onlyGlobal);
    void formatDataToSend(String &data_to_send, JsonDocument &data);
};

#endif  // __ROBONOMICS_API_H__