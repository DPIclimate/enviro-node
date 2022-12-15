#include "power_monitoring/solar.h"

static Adafruit_INA219* solar = nullptr;

void SolarMonitor::begin() {
    if(!solar){
        solar = new Adafruit_INA219(solarAddr);
        if(solar->begin()){
            // Lower resolution due to larger voltage range
            solar->setCalibration_32V_1A();
        } else {
        }
    }
}

float SolarMonitor::get_voltage(){
    float solar_voltage = 0.0f;

    if(solar){
        solar_voltage = solar->getBusVoltage_V() +
                        (solar->getShuntVoltage_mV() / 1000.0f);
    }

    return solar_voltage;
}

float SolarMonitor::get_current(){
    float solar_current = 0.0f;

    if(solar) {
        solar_current = solar->getCurrent_mA();
    }

    return solar_current;
}


void SolarMonitor::sleep() {
    if(solar){
        solar->powerSave(true);
    }
}

void SolarMonitor::wakeup() {
    if(solar){
        solar->powerSave(false);
    }
}
