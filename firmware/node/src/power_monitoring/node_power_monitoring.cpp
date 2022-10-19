#include "power_monitoring/node_power_monitoring.h"

static Adafruit_INA219* battery = nullptr;
static Adafruit_INA219* solar = nullptr;

Node_PowerMonitoring::~Node_PowerMonitoring(){
    delete battery;
    delete solar;
}

float Node_PowerMonitoring::getSolarVoltage(){
    #ifdef DEBUG
        log_msg("[DEBUG]: Getting solar voltage.");
    #endif

    float solar_voltage = 0.0f;

    if(solar){
        solar_voltage = solar->getBusVoltage_V() +
                (solar->getShuntVoltage_mV() / 1000.0f);

        #ifdef DEBUG
            Serial.print("[DEBUG]: Solar voltage: ");
            Serial.println(solar_voltage);
        #endif
    } else {
        #ifdef DEBUG
            log_msg("[ERROR]: Solar voltage reading not available. "
                    "Class may not be initialised or a connection is poor.");
        #endif
    }

    return solar_voltage;
}

float Node_PowerMonitoring::getBatteryVoltage(){
    #ifdef DEBUG
        log_msg("[DEBUG]: Getting battery voltage.");
    #endif

    float battery_voltage = 0.0f;

    if(battery){
        battery_voltage = battery->getBusVoltage_V() +
                (battery->getShuntVoltage_mV() / 1000.0f);

        #ifdef DEBUG
            Serial.print("[DEBUG]: Battery voltage: ");
            Serial.println(battery_voltage);
        #endif
    } else {
        #ifdef DEBUG
            log_msg("[ERROR]: Battery voltage reading not available. "
                    "Class may not be initialised or a connection is poor.");
        #endif
    }

    return battery_voltage;
}

float Node_PowerMonitoring::getSolarCurrent(){
    #ifdef DEBUG
        log_msg("[DEBUG]: Getting solar current.");
    #endif

    float solar_current = 0.0f;

    if(solar){
        solar_current = solar->getCurrent_mA();

        #ifdef DEBUG
            Serial.print("[DEBUG]: Solar current (mA): ");
            Serial.println(solar_current);
        #endif
    } else {
        #ifdef DEBUG
            log_msg("[ERROR]: Solar current reading not available. "
                    "Class may not be initialised or a connection is poor.");
        #endif
    }

    return solar_current;
}

float Node_PowerMonitoring::getBatteryCurrent(){
    #ifdef DEBUG
        log_msg("[DEBUG]: Getting battery current.");
    #endif

    float battery_current = 0.0f;

    if(battery){
        battery_current = battery->getCurrent_mA();

        #ifdef DEBUG
            Serial.print("[DEBUG]: Battery current (mA): ");
            Serial.println(battery_current);
        #endif
    } else {
        #ifdef DEBUG
            log_msg("[ERROR]: Battery current reading not available. "
                    "Class may not be initialised or a connection is poor.");
        #endif
    }

    return battery_current;
}

void Node_PowerMonitoring::sleep(){
    #ifdef DEBUG
        log_msg("[DEBUG]: Putting solar and battery monitoring into "
                "sleep mode.");
    #endif
    if(battery) {
        battery->powerSave(true);
    }
    if(solar){
        solar->powerSave(true);
    }
}

void Node_PowerMonitoring::wakeup(){
    #ifdef DEBUG
        log_msg("[DEBUG]: Waking up solar and battery power monitoring.");
    #endif
    if(battery) {
        battery->powerSave(false);
    }
    if(solar){
        solar->powerSave(false);
    }
}

void Node_PowerMonitoring::begin() {
    #ifdef DEBUG
        log_msg("[DEBUG]: Starting solar and battery power monitoring.");
    #endif

    if(!battery){
        battery = new Adafruit_INA219(batteryAddr);
        if(battery->begin()) {
            // Higher resolution at lower voltage range for battery
            battery->setCalibration_16V_400mA();
        #ifdef DEBUG
            log_msg("[DEBUG]: Battery power monitoring initialised.");
        #endif
        } else {
            #ifdef DEBUG
                log_msg("[ERROR]: Unable to initialise battery power "
                        "monitoring.");
            #endif
        }
    }

    if(!solar){
        solar = new Adafruit_INA219(solarAddr);
        if(solar->begin()){
            // Lower resolution due to larger voltage range
            solar->setCalibration_32V_1A();
            #ifdef DEBUG
                log_msg("[DEBUG]: Solar power monitoring initialised.");
            #endif
        } else {
            #ifdef DEBUG
                log_msg("[ERROR]: Unable to initialise solar power monitoring.");
            #endif
        }
    }
}
