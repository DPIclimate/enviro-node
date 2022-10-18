#include <Arduino.h>

#include "node_config.h"
#ifdef DEBUG
#include "tests/node_tests.h"
#endif

#include "sdi12/node_sdi12.h"

#ifndef DEBUG
#include "node_power_monitoring.h"
Node_PowerMonitoring pwr;
#endif

int8_t payload[] = {1, 2, 3, 4, 5};

void setup() {
    Serial.begin(115200);

    pinMode(LED_BUILTIN, OUTPUT);       // Onboard LED
    pinMode(POWER_12V_EN, OUTPUT);      // 12V and 5V power pin
    digitalWrite(POWER_12V_EN, HIGH);   // Turn on SDI-12 devices

    // Wait until serial console is open before debugging
    #ifdef DEBUG
        while(!Serial) yield();
    #endif

    Node_SDI12 sdi12;
    Node_UnitTests();
}

void loop() {
    delay(60000);
}