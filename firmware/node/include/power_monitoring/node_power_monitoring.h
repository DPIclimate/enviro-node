#ifndef NODE_POWER_MONITORING_H
#define NODE_POWER_MONITORING_H

#include "Adafruit_INA219.h"
#include "node_config.h"

class Node_PowerMonitoring {
private:
    static const uint8_t batteryAddr = 0x45;
    static const uint8_t solarAddr = 0x40;

public:
    Node_PowerMonitoring() = default;
    ~Node_PowerMonitoring();
    static void begin();

    static float getSolarVoltage();
    static float getBatteryVoltage();

    static float getSolarCurrent();
    static float getBatteryCurrent();

    static void sleep();
    static void wakeup();
};

#endif //NODE_POWER_MONITORING_H
