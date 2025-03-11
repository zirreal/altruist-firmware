#ifndef __RADSENS_H__
#define __RADSENS_H__

#include "sensor.h"
#include "drivers/ClimateGuard_RadSens/src/CG_RadSens.h"

class RadSensSensor : public Sensor
{

public:
    RadSensSensor(unsigned long sending_timeout = 1000UL);

    bool begin() override;

private:
    void _fetch(JsonDocument &data) override;
    CG_RadSens radSens;
    float last_value_gc = 0.0;
};

#endif // __RADSENS_H__