#include <Arduino.h>
#include <Wire.h>

#include "audio_feedback.h"
#include "TCA9534.h"
#include "CAT_M1.h"

#include <SD.h>

static TCA9534 io_expander;

CAT_M1 cat_m1;

void setup() {
    //init_tones();

    Serial.begin(115200);
    while(!Serial) delay(1);

    //startup_tone();

    Wire.begin(GPIO_NUM_25, GPIO_NUM_23);
    io_expander.attach(Wire);
    io_expander.setDeviceAddress(0x20);

    io_expander.config(TCA9534::Config::OUT);
    io_expander.config(4, TCA9534::Config::IN);

    //SD.begin(4);

    cat_m1.begin(io_expander);
    LTE_Serial.begin(115200);
    while(!LTE_Serial) delay(1);
    Serial.println("Started CAT-M1");
}

void loop() {
    cat_m1.interface();
    delay(1);
}

//#ifdef __cplusplus
//extern "C" {
//#endif
//extern void digitalWrite(uint8_t pin, uint8_t val) {
//    if(pin == 4) {
//        ESP_LOGI(TAG, "digitalWrite(%u, %u", pin, val);
//        if(val == HIGH){
//            io_expander.output(2, TCA9534::H);
//        } else {
//            io_expander.output(2, TCA9534::L);
//        }
//        return;
//    }
//    gpio_set_level((gpio_num_t)pin, val);
//}
//#ifdef __cplusplus
//}
//#endif
