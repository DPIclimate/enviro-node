#ifndef ESP32_WOMBAT_AUDIO_FEEDBACK_H
#define ESP32_WOMBAT_AUDIO_FEEDBACK_H

#include <Arduino.h>

#define PIEZO_BUZZER_PIN GPIO_NUM_17

void init_tones(){
    pinMode(PIEZO_BUZZER_PIN, OUTPUT);
}

void startup_tone(){
    tone(PIEZO_BUZZER_PIN, 494); // B4
    delay(100);
    tone(PIEZO_BUZZER_PIN, 740); // F5#
    delay(100);
    tone(PIEZO_BUZZER_PIN, 784); // G5
    delay(100);
    tone(PIEZO_BUZZER_PIN, 1175); // D6
    delay(400);
    noTone(PIEZO_BUZZER_PIN);
}

void shutdown_tone(){
    tone(PIEZO_BUZZER_PIN, 1175); // D6
    delay(100);
    tone(PIEZO_BUZZER_PIN, 784); // G5
    delay(100);
    tone(PIEZO_BUZZER_PIN, 740); // F5#
    delay(100);
    tone(PIEZO_BUZZER_PIN, 494); // B4
    delay(400);
    noTone(PIEZO_BUZZER_PIN);
}

void completed_tone(){
    tone(PIEZO_BUZZER_PIN, 1319); // E6
    delay(100);
    noTone(PIEZO_BUZZER_PIN);
}

void message_tone(){
    tone(PIEZO_BUZZER_PIN, 1319); // E6
    delay(100);
    tone(PIEZO_BUZZER_PIN, 1760); // A6
    delay(100);
    noTone(PIEZO_BUZZER_PIN);
}

void error_tone(){
    tone(PIEZO_BUZZER_PIN, 294); // D4
    delay(100);
    tone(PIEZO_BUZZER_PIN, 131); // C3
    delay(100);
    noTone(PIEZO_BUZZER_PIN);
}

#endif //ESP32_WOMBAT_AUDIO_FEEDBACK_H
