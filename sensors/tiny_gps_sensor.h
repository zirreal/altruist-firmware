#ifndef __TINY_GPS_H__
#define __TINY_GPS_H__

#include "sensor.h"
#include <TinyGPS++.h>
#include <HardwareSerial.h>


class GPSSensor : public Sensor
{

public:
    GPSSensor(unsigned long sending_timeout = 1000UL);

    bool begin() override;

private:
    void _fetch(JsonDocument &data) override;

    TinyGPSPlus gps;
    void displayInfo();
};

#endif // __TINY_GPS_H__