#ifndef WOMBAT_POWER_MONITORING_BATTERY_H
#define WOMBAT_POWER_MONITORING_BATTERY_H

#include <Arduino.h>
#include "Adafruit_INA219.h"

class BatteryPowerMonitoring {
private:
    static const uint8_t batteryAddr = 0x40;

public:
    BatteryPowerMonitoring() = default;
    ~BatteryPowerMonitoring() = default;
    static void begin();

    static float get_voltage();
    static float get_current();

    static void sleep();
    static void wakeup();
};

#endif //WOMBAT_POWER_MONITORING_BATTERY_H
