/**
 * @file battery.cpp
 *
 * @brief Battery voltage and current monitoring using INA219 IC.
 *
 * @date January 2023
 */
#include "power_monitoring/battery.h"

#define TAG "battery"

//! Adafruit_INA219 battery monitoring instance
static Adafruit_INA219* battery = nullptr;
//! Track if the INA219 is setup and available
static bool ina219_ok = false;

/**
 * @brief Setup INA219 IC to monitoring battery voltage and current.
 *
 * As the battery has a maximum voltage of 4.2V a calibration factor can be
 * used to improve accuracy.
 */
void BatteryMonitor::begin() {
    if (!battery) {
        battery = new Adafruit_INA219(batteryAddr);
        if (battery->begin()) {
            ina219_ok = true;
            // Lower resolution due to larger voltage range
            battery->setCalibration_16V_400mA();
            ESP_LOGI(TAG, "Battery monitor initialised");
        } else {
            ESP_LOGE(TAG, "Battery monitor initialisation failed");
        }
    }
}

/**
 * @brief Get the voltage of the battery.
 *
 * If the IC is not initialed this function will return -1.0.
 *
 * @return Battery voltage.
 */
float BatteryMonitor::get_voltage() {
    if ( ! ina219_ok) {
        ESP_LOGI(TAG, "ina219_ok: %d", ina219_ok);
    }

    if ( ! battery) {
        ESP_LOGI(TAG, "battery is null");
    }

    if ( ! ina219_ok || ! battery) {
        return -1.0f;
    }

    float bus_volts = battery->getBusVoltage_V();
    float shunt_mv = battery->getShuntVoltage_mV();
    float battery_voltage = bus_volts + (shunt_mv / 1000.0f);

    ESP_LOGI(TAG, "battery bus_volts = %.2f, shunt_mv = %.2f, final value: %.2f", bus_volts, shunt_mv, battery_voltage);

    return battery_voltage;
}

/**
 * @brief Get the current flowing from/to the battery.
 *
 * Note that this may not be super accurate as the current consumption will
 * change when different components become active and inactive. Using external
 * power such as that through the debugger will also result inaccurate readings.
 *
 * If the IC is not initialed this function will return -1.0.
 *
 * @return Battery current.
 */
float BatteryMonitor::get_current() {
    if (ina219_ok && battery) {
        float mA = battery->getCurrent_mA();
        ESP_LOGI(TAG, "battery bus_mA = %.2f", mA);
        return mA;
    }

    return -1.0f;
}

/**
 * @brief Puts the INA219 IC in a sleep mode.
 *
 * The IC will consume around 6 uA when in sleep mode.
 */
void BatteryMonitor::sleep() {
    if (ina219_ok && battery) {
        battery->powerSave(true);
    }
}

/**
 * @brief Wakes up the INA219 IC.
 */
void BatteryMonitor::wakeup() {
    if (ina219_ok && battery) {
        battery->powerSave(false);
    }
}
