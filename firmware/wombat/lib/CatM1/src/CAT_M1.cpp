#include "CAT_M1.h"

CAT_M1 cat_m1;

void CAT_M1::begin(TCA9534& io_ex){

    // Setup TCA9534
    io_expander = &io_ex;
    // The & 0x7F is to mask out the MSB that tells our version of digitalWrite that the
    // pin is on the IO expander.
    io_expander->config(LTE_PWR_SUPPLY_PIN & 0x7F, TCA9534::Config::OUT);
    io_expander->config(LTE_PWR_TOGGLE_PIN & 0x7F, TCA9534::Config::OUT);

    power_supply(true);
    device_on();
}

void CAT_M1::power_supply(bool state) {
    digitalWrite(LTE_PWR_SUPPLY_PIN, state ? HIGH : LOW);
    delay(10); // May not be needed
}

void CAT_M1::device_on(){
    digitalWrite(LTE_PWR_TOGGLE_PIN, HIGH);
    delay(200);
    digitalWrite(LTE_PWR_TOGGLE_PIN, LOW);
}

void CAT_M1::device_restart(){
    digitalWrite(LTE_PWR_TOGGLE_PIN, HIGH);
    delay(10000);
    digitalWrite(LTE_PWR_TOGGLE_PIN, LOW);
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
