#ifndef WOMBAT_POWER_MONITORING_SOLAR_H
#define WOMBAT_POWER_MONITORING_SOLAR_H

#include <Arduino.h>
#include "Adafruit_INA219.h"

class SolarPowerMonitoring{
private:
    static const uint8_t solarAddr = 0x40;

public:
    SolarPowerMonitoring() = default;
    ~SolarPowerMonitoring() = default;
    static void begin();

    static float get_voltage();
    static float get_current();

    static void sleep();
    static void wakeup();
};

#endif //WOMBAT_POWER_MONITORING_SOLAR_H