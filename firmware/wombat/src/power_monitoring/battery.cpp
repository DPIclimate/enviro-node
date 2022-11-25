#include "power_monitoring/battery.h"

static Adafruit_INA219* battery = nullptr;

void BatteryPowerMonitoring::begin() {
    if(!battery){
        battery = new Adafruit_INA219(batteryAddr);
        if(battery->begin()){
            // Lower resolution due to larger voltage range
            battery->setCalibration_16V_400mA();
        } else {
        }
    }
}

float BatteryPowerMonitoring::get_voltage(){
    float battery_voltage = 0.0f;

    if(battery){
        battery_voltage = battery->getBusVoltage_V() +
                        (battery->getShuntVoltage_mV() / 1000.0f);
    }

    return battery_voltage;
}

float BatteryPowerMonitoring::get_current(){
    float battery_current = 0.0f;

    if(battery) {
        battery_current = battery->getCurrent_mA();
    }

    return battery_current;
}


void BatteryPowerMonitoring::sleep() {
    if(battery){
        battery->powerSave(true);
    }
}

void BatteryPowerMonitoring::wakeup() {
    if(battery){
        battery->powerSave(false);
    }
}




