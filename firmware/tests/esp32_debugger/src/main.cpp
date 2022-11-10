#include <Arduino.h>
#include "audio_feedback.h"

void setup() {
    init_tones();
    startup_tone();
}

void loop() {
    delay(60000);
}