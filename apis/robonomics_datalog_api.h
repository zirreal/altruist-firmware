#ifndef __DATALOG_API_H__
#define __DATALOG_API_H__

#include "api.h"
#include "WiFiClient.h"
#include "HTTPClient.h"
#include "../robonomics_servers.h"
#include <Robonomics.h>

class RobonomicsDatalogAPI : public API {
public:

  void setRobonomcis(Robonomics* robonomics) {
    this->robonomics = robonomics;
  }

  void setup() override;

private:
    String rws_owner;
    String private_key;
    String robonomics_public_node;
    Robonomics* robonomics;
    void _send(JsonDocument &data) override;
};

#endif  // __DATALOG_API_H__