#include <Arduino.h>
#include <esp_log.h>

#include <freertos/FreeRTOS.h>

#include <TCA9534.h>
#include "SensorTask.h"
#include <CAT_M1.h>
#include <dpiclimate-12.h>
#include "Config.h"
#include "CLI.h"

#define TAG "wombat"

TCA9534 io_expander;

CAT_M1 cat_m1;

// Used by OpenOCD.
static volatile int uxTopUsedPriority;


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

void setup() {
    pinMode(PROG_BTN, INPUT);

    Serial.begin(115200);
    while ( ! Serial) {
        delay(1);
    }

    if (digitalRead(PROG_BTN) == HIGH) {
        progBtnPressed = true;
    }

    // Not working.
    esp_log_level_set("sensors", ESP_LOG_WARN);
    esp_log_level_set("DPI12", ESP_LOG_WARN);

    // Try to avoid it getting optimized out.
    uxTopUsedPriority = configMAX_PRIORITIES - 1;

    Wire.begin(GPIO_NUM_25, GPIO_NUM_23);
    io_expander.attach(Wire);
    io_expander.setDeviceAddress(0x20);

    // Set all pins to output mode (reg 3, value 0).
    io_expander.config(TCA9534::Config::OUT);

    cat_m1.begin(io_expander);
    LTE_Serial.begin(115200);
    while(!LTE_Serial) {
        delay(1);
    }

    Config& config = Config::get();
    config.load();

    cliInitialise();

    if (progBtnPressed) {
        progBtnPressed = false;
        ESP_LOGI(TAG, "Programmable button pressed while booting, dropping into REPL");
        repl(Serial);
        ESP_LOGI(TAG, "Continuing");
    }

    uint16_t mi = config.getMeasureInterval();
    uint16_t ui = config.getUplinkInterval();

    if (ui < mi) {
        ESP_LOGE(TAG, "Measurement interval (%u) must be <= uplink interval (%u). Resetting config to default values.", mi, ui);
        config.reset();
    }

    if (ui % mi != 0) {
        ESP_LOGE(TAG, "Measurement interval (%u) must be a factor of uplink interval (%u). Resetting config to default values.", mi, ui);
        config.reset();
    }

    // Assuming the measure interval is a factor of the uplink interval,
    // uimi is the number of boots between uplinks.
    uint16_t uimi = ui / mi;

    // If the current boot cycle is a multiple of uimi, then this boot cycle
    // should do an uplink.
    bool uplinkCycle = config.getBootCount() % uimi == 0;

    ESP_LOGI(TAG, "Boot count: %lu, measurement interval: %u, uplink interval: %u, ui/mi: %u, uplink this cycle: %d", config.getBootCount(), mi, ui, uimi, uplinkCycle);

    // Not sure how useful this is, or how it will work. We don't want to interrupt sensor reading or uplink
    // processing and loop() will likely never run if we do the usual ESP32 setup going to deep sleep mode.
    attachInterrupt(PROG_BTN, progBtnISR, RISING);

    initSensors();

    sensorTask();

    // Flush log messages before sleeping;
    Serial.flush();

//    unsigned long setupEnd = millis();
//    uint64_t sleepTime = (mi * 1000000) - (setupEnd * 1000);
//    esp_sleep_enable_timer_wakeup(sleepTime);
//    esp_deep_sleep_start();
}

void loop() {
    if (progBtnPressed) {
        progBtnPressed = false;
        ESP_LOGI(TAG, "Button");
        repl(Serial);
    }
}

#ifdef __cplusplus
extern "C" {
#endif
extern void digitalWrite(uint8_t pin, uint8_t val) {
    if(pin & 0x80) {
        io_expander.output(pin & 0x7F, val == HIGH ? TCA9534::H : TCA9534::L);
    } else {
        gpio_set_level((gpio_num_t) pin, val);
    }
}
#ifdef __cplusplus
}
#endif
