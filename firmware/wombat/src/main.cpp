#include <Arduino.h>
#include <esp_log.h>

#include <freertos/FreeRTOS.h>

#include <TCA9534.h>
#include <driver/rtc_io.h>

#define ALLOCATE_GLOBALS
#include "globals.h"
#undef ALLOCATE_GLOBALS

#include "SensorTask.h"
#include "DeviceConfig.h"
#include "cli/CLI.h"
#include "peripherals.h"

#include "audio-feedback/tones.h"
#include "uplinks.h"
#include "SparkFun_u-blox_SARA-R5_Arduino_Library.h"
#include "ulp.h"

#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

#define TAG "wombat"

TCA9534 io_expander;

// Used by OpenOCD.
static volatile int uxTopUsedPriority;

extern bool getNTPTime(SARA_R5 &r5);

void time_check(void);

void setup() {

    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

    pinMode(PROG_BTN, INPUT);
    progBtnPressed = digitalRead(PROG_BTN);

    Serial.begin(115200);
    while ( ! Serial) {
        delay(1);
    }

    ESP_LOGI(TAG, "Wake up time: %s", iso8601());
    ESP_LOGI(TAG, "CPU MHz: %lu", getCpuFrequencyMhz());

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
    io_expander.output(4, TCA9534::Level::L); // Turn off LED

    cat_m1.begin(io_expander);

    delay(1000);

    LTE_Serial.begin(115200);
    while(!LTE_Serial) {
        delay(1);
    }

    // ==== CAT-M1 Setup END ====

    // This must be done before the config is loaded because the config file is
    // a list of commands.
    CLI::init();

    // BluetoothServer::begin();

    DeviceConfig& config = DeviceConfig::get();
    config.load();

    config.setMeasurementAndUplinkIntervals(60, 3600);
    config.dumpConfig(Serial);

    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 1);

    if (progBtnPressed) {
        // Turn on bluetooth if entering CLI
        BluetoothServer::begin();
        progBtnPressed = false;
        ESP_LOGI(TAG, "Programmable button pressed while booting, dropping into REPL");
        CLI::repl(Serial);
        ESP_LOGI(TAG, "Continuing");
    }

    if (config.getBootCount() == 0) {
        initULP();
    }

    time_check();

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
    // It is useful while developing because the node isn't going to sleep.
//    attachInterrupt(PROG_BTN, progBtnISR, RISING);

    battery_monitor.begin();
    solar_monitor.begin();

    initSensors();
    sensorTask();

    if (uplinkCycle) {
        send_messages();
    }

    ESP_LOGI(TAG, "Shutting down");

    if (r5_ok) {
        r5.modulePowerOff();
    }

    cat_m1.power_supply(false);
    delay(20);
    battery_monitor.sleep();
    solar_monitor.sleep();

    unsigned long setupEnd = millis();
    unsigned long setup_in_secs = setupEnd / 1000;
    uint64_t mi_in_secs = (uint64_t)mi;
    uint64_t sleep_time = 0;
    if (setup_in_secs > mi) {
        ESP_LOGW(TAG, "Processing took longer than the measurement interval, skip some intervals");
        uint64_t remainder = setup_in_secs - mi_in_secs;
        while (remainder > mi_in_secs) {
            remainder = setup_in_secs - mi_in_secs;
            setup_in_secs = setup_in_secs - mi_in_secs;
        }

        ESP_LOGW(TAG, "mi_in_secs = %lu, setup_in_secs = %lu, remainder = %lu", mi_in_secs, setup_in_secs, remainder);
        sleep_time = (mi_in_secs - remainder) * 1000000;
    } else {
        sleep_time = (mi * 1000000) - (setupEnd * 1000);
    }

    float f_s_time = (float)sleep_time / 1000000.0f;
    ESP_LOGI(TAG, "Going to sleep at: %s, for %.2f s", iso8601(), f_s_time);
    Serial.flush();

    esp_sleep_enable_timer_wakeup(sleep_time);
    esp_deep_sleep_start();
}

void loop() {
}

void time_check(void) {
    if (time_ok()) {
        return;
    }

    connect_to_internet();

    bool success = getNTPTime(r5);
    if (!success) {
        ESP_LOGE(TAG, "getNTPTime failed");
        return;
    }
}

#ifdef __cplusplus
extern "C" {
#endif
/*
 * This function overrides the default digitalWrite provided by the Arduino core so
 * we can use it with pins on the IO expander IC. If the pin number > 0x80 then the
 * bits > 0x80 are masked off and the request is sent to the IO expander library.
 *
 * For example, passing in pin 0x82 will ask the IO expander library to write to its
 * pin 2.
 *
 * @param pin the GPIO to write to; values > 0x80 will be handled by the IO expander library
 * after masking off the bits > 0x80.
 * @val HIGH or LOW
 */
extern void digitalWrite(uint8_t pin, uint8_t val) {
    if (pin & 0x80) {
        uint8_t p = pin & 0x7F;
        io_expander.output(p, val == HIGH ? TCA9534::H : TCA9534::L);
    } else {
        gpio_set_level((gpio_num_t) pin, val);
    }
}
#ifdef __cplusplus
}
#endif
