#include "CAT_M1.h"
#include "SparkFun_u-blox_SARA-R5_Arduino_Library.h"
#include "globals.h"

#define TAG "CAT_M1"

CAT_M1 cat_m1;

void CAT_M1::begin(TCA9534& io_ex){

    // Setup TCA9534
    io_expander = &io_ex;
    // The & 0x7F is to mask out the MSB that tells our version of digitalWrite that the
    // pin is on the IO expander.
    io_expander->config(ARDUINO_TO_IO(LTE_VCC), TCA9534::Config::OUT);
    io_expander->config(ARDUINO_TO_IO(LTE_PWR_ON), TCA9534::Config::OUT);

    // Switches off VCC
    power_supply(false);

    // Turns Q4 off, allowing PWR_ON to be controlled by the R5 internal pull-up.
    digitalWrite(LTE_PWR_ON, LOW);
}

void CAT_M1::power_supply(bool state) {
    ESP_LOGI(TAG, "state = %d", state);
    digitalWrite(LTE_VCC, state ? HIGH : LOW);
    power_on = state;
    delay(10); // May not be needed
}

void CAT_M1::device_on() {
    ESP_LOGI(TAG, "Toggle PWR_ON (pin %X) for 200ms", LTE_PWR_ON);
    digitalWrite(LTE_PWR_ON, HIGH);
    delay(200);
    digitalWrite(LTE_PWR_ON, LOW);
}

void CAT_M1::device_off() {
    ESP_LOGI(TAG, "Toggle PWR_ON (pin %X) for 10s", LTE_PWR_ON);
    digitalWrite(LTE_PWR_ON, HIGH);
    delay(10000);
    digitalWrite(LTE_PWR_ON, LOW);
}

bool CAT_M1::restart() {
    constexpr size_t buf_size = 64;
    char buffer[buf_size+1];

    device_off();
    delay(1000);
    device_on();
    delay(5000);

    LTE_Serial.print("ATI\r");
    delay(20);
    memset(buffer, 0, sizeof(buffer));
    size_t i = 0;
    while (LTE_Serial.available() && i < buf_size) {
        buffer[i++] = (char)(LTE_Serial.read() & 0xFF);
        buffer[i] = 0;
    }

    ESP_LOGI(TAG, "Test command response: [%s]", buffer);
    return (buffer[0] == 'O' && buffer[1] == 'K');
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
