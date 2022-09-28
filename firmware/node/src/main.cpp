#include <Arduino.h>

#include "node_config.h"
#ifdef DEBUG
#include "node_tests.h"
#endif

#ifndef DEBUG
#include "node_power_monitoring.h"
Node_PowerMonitoring pwr;
#endif


void setup() {
    Serial.begin(115200);

    pinMode(LED_BUILTIN, OUTPUT); // Onboard LED
    pinMode(POWER_12V_EN, OUTPUT); // 12V and 5V power pin

    // Wait until serial console is open before debugging
    #ifdef DEBUG
        while(!Serial) yield();
    #endif

    // Must be called after pin definitions have been set
    #ifdef DEBUG
        Node_UnitTests ut;
    #endif
}

void loop() {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(1000);
    digitalWrite(LED_BUILTIN, LOW);
    delay(1000);
}