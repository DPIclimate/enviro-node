#include "CAT_M1.h"
#include "SparkFun_u-blox_SARA-R5_Arduino_Library.h"
#include "Utils.h"
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

bool CAT_M1::make_ready() {
    static bool already_called = false;

    if (already_called) {
        ESP_LOGI(TAG, "already_called = true, early return");
        return true;
    }

    if ( ! is_powered()) {
        ESP_LOGI(TAG, "Enabling R5 VCC");
        power_supply(true);
    }

    ESP_LOGI(TAG, "Looking for response to AT command");
    if ( ! wait_for_at()) {
        already_called = false;
        restart();
        if ( ! wait_for_at()) {
            ESP_LOGE(TAG, "Cannot talk to SARA R5");
            return false;
        }
    }

    r5.invertPowerPin(true);
    r5.autoTimeZoneForBegin(true);

    r5.enableAtDebugging();
    r5.enableDebugging();

    // This is relatively benign - it enables the network indicator GPIO pin, set error message format, etc.
    // It does close all open sockets, but there should not be any open sockets at this point so that is ok.
    r5_ok = r5.begin(LTE_Serial, 115200);
    if ( ! r5_ok) {
        ESP_LOGE(TAG, "SARA-R5 begin failed");
        return false;
    }

    already_called = true;
    return true;
}
