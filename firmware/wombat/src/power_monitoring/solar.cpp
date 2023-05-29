/**
 * @file solar.cpp
 *
 * @brief Solar voltage and current monitoring using INA219 IC.
 *
 * @date January 2023
 */
#include "power_monitoring/solar.h"

#define TAG "solar"

//! Adafruit_INA219 instance for solar monitoring
static Adafruit_INA219* solar = nullptr;
//! Track if the INA219 IC is setup and available
static bool ina219_ok = false;

/**
 * @brief Setup INA219 IC to monitoring battery voltage and current.
 *
 * As the solar panel has a maximum voltage of 21V a calibration factor is
 * set to a lower resolution.
 */
void SolarMonitor::begin() {
    if (!solar) {
        solar = new Adafruit_INA219(solarAddr);
        if (solar->begin()) {
            ina219_ok = true;

            // Lower resolution due to larger voltage range
            solar->setCalibration_32V_1A();
            ESP_LOGI(TAG, "Solar monitor initialised");
        } else {
            ESP_LOGE(TAG, "Solar monitor initialisation failed");
        }
    }
}

/**
 * @brief Get the voltage of the solar.
 *
 * If the IC is not initialed this function will return -1.0.
 *
 * @return Solar voltage.
 */
float SolarMonitor::get_voltage(){
    if ( ! ina219_ok) {
        ESP_LOGI(TAG, "ina219_ok: %d", ina219_ok);
    }

    if ( ! solar) {
        ESP_LOGI(TAG, "solar is null");
    }

    if (! ina219_ok || ! solar) {
        return -1.0f;
    }

    float bus_volts = solar->getBusVoltage_V();
    float shunt_mv = solar->getShuntVoltage_mV();
    float solar_voltage = bus_volts + (shunt_mv / 1000.0f);

    ESP_LOGI(TAG, "solar bus_volts = %.2f, shunt_mv = %.2f, final value: %.2f", bus_volts, shunt_mv, solar_voltage);

    return solar_voltage;
}

/**
 * @brief Get the current flowing from the solar panel.
 *
 * If the IC is not initialed this function will return -1.0.
 *
 * @return Solar current.
 */
float SolarMonitor::get_current() {
    if (ina219_ok && solar) {
        float mA = solar->getCurrent_mA();
        ESP_LOGI(TAG, "solar bus_mA = %.2f", mA);
        return mA;
    }

    return -1.0f;
}

/**
 * @brief Puts the INA219 IC in a sleep mode.
 *
 * The IC will consume around 6 uA when in sleep mode.
 */
void SolarMonitor::sleep() {
    if (ina219_ok && solar) {
        solar->powerSave(true);
    }
}

/**
 * @brief Wakes up the INA219 IC.
 */
void SolarMonitor::wakeup() {
    if (ina219_ok && solar) {
        solar->powerSave(false);
    }
}
