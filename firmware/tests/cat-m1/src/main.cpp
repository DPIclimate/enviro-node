#include <Arduino.h>

#define LTE_ENABLE_PIN      G3
#define LTE_POWER_PIN       G4

#define LTESerial Serial1

void setup() {
    // Enable LTE
    pinMode(LTE_ENABLE_PIN, OUTPUT);
    digitalWrite(LTE_ENABLE_PIN, HIGH);

    delay(1000);

    // Switch on LTE
    pinMode(LTE_POWER_PIN, OUTPUT);
    digitalWrite(LTE_POWER_PIN, HIGH);
    delay(1000);
    pinMode(LTE_POWER_PIN, LOW); // Rely on SARA module pull-up

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    Serial.begin(115200);
    while(!Serial){
        digitalWrite(LED_BUILTIN, HIGH);
        delay(100);
        digitalWrite(LED_BUILTIN, HIGH);
        delay(100);
    }

    Serial.println("Starting CAT-M1");
    LTESerial.begin(115200);
    while(!LTESerial) delay(1);

    Serial.println("Started CAT-M1");
}

void loop() {
    if(Serial.available()){
        Serial.println("Sending msg...");
        while(Serial.available()){
            LTESerial.write(Serial.read());
        }
        Serial.println("Waiting for response...");
    }
    while(LTESerial.available()) {
        digitalWrite(LED_BUILTIN, HIGH);
        Serial.write(LTESerial.read());
    }
    digitalWrite(LED_BUILTIN, LOW);
    delay(1);
}