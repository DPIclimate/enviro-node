#ifndef NODE_POWER_MONITORING_H
#define NODE_POWER_MONITORING_H

#include "Adafruit_INA219.h"
#include "node_config.h"

class Node_PowerMonitoring {
private:
    Adafruit_INA219* battery = nullptr;
    Adafruit_INA219* solar = nullptr;

    static const uint8_t batteryAddr = 0x45;
    static const uint8_t solarAddr = 0x40;

public:
    Node_PowerMonitoring();
    ~Node_PowerMonitoring();

    float getSolarVoltage();
    float getBatteryVoltage();

    float getSolarCurrent();
    float getBatteryCurrent();

    void sleep();
    void wakeup();
};


#endif //NODE_POWER_MONITORING_H
