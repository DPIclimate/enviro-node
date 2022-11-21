#ifndef WOMBAT_PERIPHERALS_H
#define WOMBAT_PERIPHERALS_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>

//==============================================
// Programmable button ISR with debounce.
//==============================================
#define PROG_BTN GPIO_NUM_34

volatile bool progBtnPressed = false;
static unsigned long lastTriggered = 0;

void IRAM_ATTR progBtnISR(void) {
    unsigned long now = millis();
    if (lastTriggered != 0) {
        if (now - lastTriggered < 250) {
            return;
        }
    }

    lastTriggered = now;
    progBtnPressed = true;
}



#endif //WOMBAT_PERIPHERALS_H
