#ifndef API_H
#define API_H

#include "Arduino.h"
#include <ArduinoJson.h>

class API {
protected:
  unsigned long sending_timeout;  // Private variable for sending timeout
  unsigned long timeout;  // Private variable for API timeout
  unsigned long last_send_time;

  void updateSendTime() {
    last_send_time = millis();
  }

  void send(JsonDocument &data) {
    _send(data);
    updateSendTime();
  };

public:
  // Constructor with a default timeout value (e.g., 1000 milliseconds)
  API(unsigned long sending_timeout = 1000UL) : sending_timeout(sending_timeout) {}

  virtual ~API() {}

  const char* api_name;

  virtual void _send(JsonDocument &data) = 0;

  // Optional getter for the timeout.
  bool isTimeToSend() const {
    return (millis() - last_send_time > timeout);
  }
};

#endif  // API_H