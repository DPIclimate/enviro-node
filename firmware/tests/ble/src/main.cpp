#include <Arduino.h>

#include "audio_feedback.h"
#include "ble.h"

Node_BluetoothLE bt;

void setup() {
    init_tones();

    Serial.begin(115200);
    while(!Serial) delay(1);

    //startup_tone();
    bt.begin();
    bt.read_write_blocking();
}

void loop() {
    delay(1);
}