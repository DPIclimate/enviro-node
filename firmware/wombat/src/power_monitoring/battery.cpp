#include "power_monitoring/battery.h"

#define TAG "battery"

static Adafruit_INA219* battery = nullptr;
static bool ina219_ok = false;

void BatteryMonitor::begin() {
    if (!battery) {
        battery = new Adafruit_INA219(batteryAddr);
        if (battery->begin()) {
            ina219_ok = true;
            // Lower resolution due to larger voltage range
            battery->setCalibration_16V_400mA();
            ESP_LOGI(TAG, "BatteryMonitor initialised");
        } else {
            ESP_LOGE(TAG, "BatteryMonitor initialisation failed");
        }
    }
}

float BatteryMonitor::get_voltage() {
    if ( ! ina219_ok) {
        return -1.0f;
    }

    float battery_voltage = 0.0f;

    if (battery) {
        float bus_volts = battery->getBusVoltage_V();
        float shunt_mv = battery->getShuntVoltage_mV();
        battery_voltage = bus_volts + (shunt_mv / 1000.0f);

        ESP_LOGI(TAG, "bus_volts = %.2f, shunt_mv = %.2f", bus_volts, shunt_mv);
    }

    return battery_voltage;
}

float BatteryMonitor::get_current() {
    if ( ! ina219_ok) {
        return -1.0f;
    }

    float battery_current = 0.0f;

    if (battery) {
        battery_current = battery->getCurrent_mA();
        ESP_LOGI(TAG, "battery_current = %.2f", battery_current);
    }

    return battery_current;
}


void BatteryMonitor::sleep() {
    if (ina219_ok && battery) {
        battery->powerSave(true);
    }
}

void BatteryMonitor::wakeup() {
    if (ina219_ok && battery) {
        battery->powerSave(false);
    }
}
