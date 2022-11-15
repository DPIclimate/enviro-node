#include <Arduino.h>
#include <Wire.h>

#include "audio_feedback.h"
#include "TCA9534.h"
#include "CAT_M1.h"


TCA9534 io_expander;
CAT_M1 cat_m1;

void setup() {
    init_tones();

    Serial.begin(115200);
    while(!Serial) delay(1);

    startup_tone();

    Wire.begin(GPIO_NUM_25, GPIO_NUM_23);
    io_expander.attach(Wire);
    io_expander.setDeviceAddress(0x20);

    io_expander.config(TCA9534::Config::OUT);
    io_expander.config(4, TCA9534::Config::IN);

    cat_m1.begin(io_expander);

    LTE_Serial.begin(115200);
    while(!LTE_Serial) delay(1);

    Serial.println("Started CAT-M1");
}

void loop() {
    cat_m1.interface();
    delay(1);
}