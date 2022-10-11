#include <Arduino.h>

#include <SDI12.h>
#include <dpiclimate-12.h>

#include "node_config.h"
#ifdef DEBUG
#include "tests/node_tests.h"
#endif

#ifndef DEBUG
#include "node_power_monitoring.h"
Node_PowerMonitoring pwr;
#endif

#define SDI12_PIN D0         /*!< The pin of the SDI-12 data bus */
SDI12 mySDI12(SDI12_PIN);
DPIClimate12 dpi12(mySDI12);

sensor_list sl;

void setup() {
    Serial.begin(115200);

    pinMode(LED_BUILTIN, OUTPUT); // Onboard LED
    pinMode(POWER_12V_EN, OUTPUT); // 12V and 5V power pin
    digitalWrite(POWER_12V_EN, HIGH); // Turn on SDI-12 devices

    // Wait until serial console is open before debugging
    #ifdef DEBUG
        while(!Serial) yield();
    #endif

    // Must be called after pin definitions have been set
    #ifdef DEBUG
        //Node_UnitTests ut;
    #endif

    mySDI12.begin();

}

void loop() {

    dpi12.scan_bus(sl);

    for(int i = 0; i < sl.count; i++){
        for(unsigned char x : sl.sensors->info){
            Serial.print(x);
        }
        Serial.println();
    }

    digitalWrite(LED_BUILTIN, HIGH);
    delay(1000);
    digitalWrite(LED_BUILTIN, LOW);
    delay(1000);
}