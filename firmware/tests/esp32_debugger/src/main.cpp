#include <Arduino.h>
#include "audio_feedback.h"

void setup() {
    pinMode(GPIO_NUM_17, OUTPUT);
}

void loop() {
    startup_tone();
    delay(60000);
}