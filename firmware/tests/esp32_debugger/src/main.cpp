#include <Arduino.h>

void setup() {
    pinMode(GPIO_NUM_17, OUTPUT);
}

void startup_tone(){
    tone(GPIO_NUM_17, 494); // B4
    delay(100);
    tone(GPIO_NUM_17, 740); // F5#
    delay(100);
    tone(GPIO_NUM_17, 784); // G5
    delay(100);
    tone(GPIO_NUM_17, 1175); // D6
    delay(400);
    noTone(GPIO_NUM_17);
}

void shutdown_tone(){
    tone(GPIO_NUM_17, 1175); // D6
    delay(100);
    tone(GPIO_NUM_17, 784); // G5
    delay(100);
    tone(GPIO_NUM_17, 740); // F5#
    delay(100);
    tone(GPIO_NUM_17, 494); // B4
    delay(400);
    noTone(GPIO_NUM_17);
}

void command_tone(){
    tone(GPIO_NUM_17, 1319); // E6
    delay(200);
    noTone(GPIO_NUM_17);
}

void loop() {
    //startup_tone();
    delay(60000);
}