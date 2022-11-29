#include <Arduino.h>
#include <esp_log.h>

#include <freertos/FreeRTOS.h>

#include <TCA9534.h>
#include "SensorTask.h"
#include <CAT_M1.h>
#include "DeviceConfig.h"
#include "cli/CLI.h"
#include "peripherals.h"

#include "audio-feedback/tones.h"

#define TAG "wombat"

TCA9534 io_expander;
CAT_M1 cat_m1;

// Used by OpenOCD.
static volatile int uxTopUsedPriority;

void setup() {
    pinMode(PROG_BTN, INPUT);

    Serial.begin(115200);
    while ( ! Serial) {
        delay(1);
    }

    // Initialise audio feedback
    init_tones();

    if (digitalRead(PROG_BTN) == HIGH) {
        progBtnPressed = true;
    }

    // Not working.
    esp_log_level_set("sensors", ESP_LOG_WARN);
    esp_log_level_set("DPI12", ESP_LOG_WARN);

    // Try to avoid it getting optimized out.
    uxTopUsedPriority = configMAX_PRIORITIES - 1;

    // ==== CAT-M1 Setup START ====
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
    // ==== CAT-M1 Setup END ====

    DeviceConfig& config = DeviceConfig::get();
    config.load();

    BluetoothServer::begin();

    CLI::init();

    if (progBtnPressed) {
        progBtnPressed = false;
        ESP_LOGI(TAG, "Programmable button pressed while booting, dropping into REPL");
        CLI::repl(Serial);
        ESP_LOGI(TAG, "Continuing");
    }

    uint16_t mi = config.getMeasureInterval();
    uint16_t ui = config.getUplinkInterval();

    if (ui < mi) {
        ESP_LOGE(TAG, "Measurement interval (%u) must be <= uplink interval "
                      "(%u). Resetting config to default values.", mi, ui);
        config.reset();
    }

    if (ui % mi != 0) {
        ESP_LOGE(TAG, "Measurement interval (%u) must be a factor of uplink "
                      "interval (%u). Resetting config to default "
                      "values.", mi, ui);
        config.reset();
    }

    // Assuming the measure interval is a factor of the uplink interval,
    // uimi is the number of boots between uplinks.
    uint16_t uimi = ui / mi;

    // If the current boot cycle is a multiple of uimi, then this boot cycle
    // should do an uplink.
    bool uplinkCycle = config.getBootCount() % uimi == 0;

    ESP_LOGI(TAG, "Boot count: %lu, measurement interval: %u, "
                  "uplink interval: %u, ui/mi: %u, uplink this cycle: %d",
                  config.getBootCount(), mi, ui, uimi, uplinkCycle);

    // Not sure how useful this is, or how it will work. We don't want to
    // interrupt sensor reading or uplink processing and loop() will likely
    // never run if we do the usual ESP32 setup going to deep sleep mode.
    attachInterrupt(PROG_BTN, progBtnISR, RISING);

    //initSensors();

    sensorTask();

    // Flush log messages before sleeping;
    Serial.flush();

}

void loop() {
    if (progBtnPressed) {
        progBtnPressed = false;
        ESP_LOGI(TAG, "Button");
        CLI::repl(Serial);
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
