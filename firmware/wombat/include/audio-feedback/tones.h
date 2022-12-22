/**
 * @file tones.h
 *
 * @brief Audio control for the onboard piezo buzzer.
 *
 * A GPIO pin is used to drive the gate of a transistor connected to an
 * onboard piezo buzzer. Several tones are provided to enable audio feedback
 * when the device is running.
 *
 * @date December 2022
 */
#ifndef WOMBAT_TONES_H
#define WOMBAT_TONES_H

#include <Arduino.h>

#define PIEZO_BUZZER_PIN GPIO_NUM_17 ///< GPIO pin to control the piezo buzzer.

/**
 * @brief Start tones by setting GPIO pin as an OUTPUT to control the onboard
 * piezo buzzer.
 */
static void init_tones(){
    pinMode(PIEZO_BUZZER_PIN, OUTPUT);
}

/**
 * @brief Used on device startup.
 */
static void startup_tone(){
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

/**
 * @brief Used on device shutdown.
 */
static void shutdown_tone(){
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

/**
 * @brief Command completed tone.
 */
static void completed_tone(){
    tone(PIEZO_BUZZER_PIN, 1319); // E6
    delay(100);
    noTone(PIEZO_BUZZER_PIN);
}

/**
 * @brief BluetoothLE message received tone.
 */
static void message_tone(){
    tone(PIEZO_BUZZER_PIN, 1319); // E6
    delay(100);
    tone(PIEZO_BUZZER_PIN, 1760); // A6
    delay(100);
    noTone(PIEZO_BUZZER_PIN);
}

/**
 * @brief Generic error tone.
 */
static void error_tone(){
    tone(PIEZO_BUZZER_PIN, 294); // D4
    delay(100);
    tone(PIEZO_BUZZER_PIN, 131); // C3
    delay(100);
    noTone(PIEZO_BUZZER_PIN);
}

#endif //WOMBAT_TONES_H
