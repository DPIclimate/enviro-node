/**
 * @file solar.h
 *
 * @brief Solar voltage and current monitoring using INA219 IC.
 *
 * @date January 2023
 */
#ifndef WOMBAT_POWER_MONITORING_SOLAR_H
#define WOMBAT_POWER_MONITORING_SOLAR_H

#include <Arduino.h>
#include "Adafruit_INA219.h"

/**
 * @brief Solar power monitoring handler.
 *
 * A INA219 IC is used to monitor solar voltage and current. When using the
 * Adafruit_INA219 library this IC requires a 1 ohm resistor across its positive
 * and negative inputs.
 *
 * @see Library: https://github.com/adafruit/Adafruit_INA219
 * @see Datasheet: https://www.ti.com/lit/ds/symlink/ina219.pdf?ts=1673141206831
 */
class SolarMonitor {
private:
    //! Default I2C solar address (cannot be changed without modifying PCB)
    static const uint8_t solarAddr = 0x40;

public:
    static void begin();

    static float get_voltage();
    static float get_current();

    static void sleep();
    static void wakeup();
};

#endif //WOMBAT_POWER_MONITORING_SOLAR_H
