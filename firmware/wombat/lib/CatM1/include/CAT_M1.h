#ifndef ESP32_DEBUGGER_CAT_M1_H
#define ESP32_DEBUGGER_CAT_M1_H

#include <Wire.h>
#include <TCA9534.h>

#define LTE_PWR_SUPPLY_PIN 0x83
#define LTE_PWR_TOGGLE_PIN 0x80

#define LTE_Serial Serial1

class CAT_M1 {
    TCA9534* io_expander;

public:
    CAT_M1() = default;
    ~CAT_M1() = default;

    void begin(TCA9534& tca9534);
    void power_supply(bool state);
    void device_on();
    void device_off();
    bool restart();
    void interface();

    bool is_powered(void) { return power_on; }

private:
    bool power_on;
};

extern CAT_M1 cat_m1;

#endif //ESP32_DEBUGGER_CAT_M1_H
