#include "CAT_M1.h"

CAT_M1 cat_m1;

void CAT_M1::begin(TCA9534& io_ex){

    // Setup TCA9534
    io_expander = &io_ex;
    io_expander->config(LTE_PWR_SUPPLY_PIN, TCA9534::Config::OUT);
    io_expander->config(LTE_PWR_TOGGLE_PIN, TCA9534::Config::OUT);

    power_supply(true);
    device_on();

}

void CAT_M1::power_supply(bool state){
    if(state){
        io_expander->output(LTE_PWR_SUPPLY_PIN, TCA9534::Level::H);
    } else {
        io_expander->output(LTE_PWR_SUPPLY_PIN, TCA9534::Level::L);
    }
    delay(10); // May not be needed
}

void CAT_M1::device_on(){
    io_expander->output(LTE_PWR_TOGGLE_PIN, TCA9534::Level::H);
    delay(200);
    io_expander->output(LTE_PWR_TOGGLE_PIN, TCA9534::Level::L);
}

void CAT_M1::device_restart(){
    io_expander->output(LTE_PWR_TOGGLE_PIN, TCA9534::Level::H);
    delay(10000);
    io_expander->output(LTE_PWR_TOGGLE_PIN, TCA9534::Level::L);
}

void CAT_M1::interface(){
    if(Serial.available()){
        Serial.println("Sending msg...");
        while(Serial.available()){
            LTE_Serial.write(Serial.read());
        }
        Serial.println("Waiting for response...");
    }
    while(LTE_Serial.available()) {
        Serial.write(LTE_Serial.read());
    }
}
