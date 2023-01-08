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
    if(!solar){
        solar = new Adafruit_INA219(solarAddr);
        if(solar->begin()){
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
    if (! ina219_ok || ! solar ){
        return -1.0f;
    }

    float solar_voltage = solar->getBusVoltage_V() +
                    (solar->getShuntVoltage_mV() / 1000.0f);

    return solar_voltage;
}

/**
 * @brief Get the current flowing from the solar panel.
 *
 * If the IC is not initialed this function will return -1.0.
 *
 * @return Solar current.
 */
float SolarMonitor::get_current(){
    if (! ina219_ok){
        return -1.0f;
    }

    float solar_current = -1.0f;

    if(solar) {
        solar_current = solar->getCurrent_mA();
    }

    return solar_current;
}

/**
 * @brief Puts the INA219 IC in a sleep mode.
 *
 * The IC will consume around 6 uA when in sleep mode.
 */
void SolarMonitor::sleep() {
    if(solar){
        solar->powerSave(true);
    }
}

/**
 * @brief Wakes up the INA219 IC.
 */
void SolarMonitor::wakeup() {
    if(solar){
        solar->powerSave(false);
    }
}
