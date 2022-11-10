#include <Arduino.h>
#include <Wire.h>

#include "audio_feedback.h"
#include "TCA9534.h"

TCA9534 io_expander;

void setup() {
    init_tones();

    Serial.begin(115200);
    while(!Serial) delay(1);

    //startup_tone();

    Wire.begin(GPIO_NUM_25, GPIO_NUM_23);
    io_expander.attach(Wire);
    io_expander.setDeviceAddress(0x20);

    io_expander.config(TCA9534::Config::OUT);
    io_expander.config(4, TCA9534::Config::IN);
    io_expander.output(1, TCA9534::Level::H);
}

void loop() {
    delay(60000);
}