#ifndef ESP32_DEBUGGER_CAT_M1_H
#define ESP32_DEBUGGER_CAT_M1_H

#include <Wire.h>
#include <TCA9534.h>

#define LTE_PWR_SUPPLY_PIN 3
#define LTE_PWR_TOGGLE_PIN 0

#define LTE_Serial Serial1

class CAT_M1 {
    TCA9534* io_expander;

public:
    CAT_M1() = default;
    ~CAT_M1() = default;

    void begin(TCA9534& tca9534);
    void power_supply(bool state);
    void device_on();
    void device_restart();
    void interface();

};

#endif //ESP32_DEBUGGER_CAT_M1_H
