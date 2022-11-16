#include <Arduino.h>
#include <esp_log.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <StreamString.h>

#include "FreeRTOS_CLI.h"

#include <TCA9534.h>
#include "SensorTask.h"
#include <CAT_M1.h>
#include <dpiclimate-12.h>
#include "Config.h"
#include "CLI.h"

#define TAG "wombat"

TCA9534 io_expander;

CAT_M1 cat_m1;

void enable12V(void) {
    Wire.begin(GPIO_NUM_25, GPIO_NUM_23);
    io_expander.attach(Wire);
    io_expander.setDeviceAddress(0x20);

    io_expander.config(TCA9534::Config::OUT);
    io_expander.config(4, TCA9534::Config::IN);
    io_expander.output(1, TCA9534::Level::H);
}

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

#define MAX_MSG 256
static char msg[MAX_MSG+1];

static void executeCmd(const char *cmd) {
    if (cmd == nullptr || *cmd == 0) {
        return;
    }

    BaseType_t rc = pdTRUE;
    while (rc != pdFALSE) {
        memset(msg, 0, sizeof(msg));
        rc = FreeRTOS_CLIProcessCommand(cmd, msg, MAX_MSG);
        Serial.print(msg);
    }
}

void verbose_print_reset_reason(esp_reset_reason_t reason) {
    switch (reason) {
        case ESP_RST_UNKNOWN:    //!< Reset reason can not be determined
            ESP_LOGI(TAG, "%s", "ESP_RST_UNKNOWN");
            break;
        case ESP_RST_POWERON:    //!< Reset due to power-on event
            ESP_LOGI(TAG, "%s", "ESP_RST_POWERON");
            break;
        case ESP_RST_EXT:        //!< Reset by external pin (not applicable for ESP32)
            ESP_LOGI(TAG, "%s", "ESP_RST_EXT");
            break;
        case ESP_RST_SW:         //!< Software reset via esp_restart
            ESP_LOGI(TAG, "%s", "ESP_RST_SW");
            break;
        case ESP_RST_PANIC:      //!< Software reset due to exception/panic
            ESP_LOGI(TAG, "%s", "ESP_RST_PANIC");
            break;
        case ESP_RST_INT_WDT:    //!< Reset (software or hardware) due to interrupt watchdog
            ESP_LOGI(TAG, "%s", "ESP_RST_INT_WDT");
            break;
        case ESP_RST_TASK_WDT:   //!< Reset due to task watchdog
            ESP_LOGI(TAG, "%s", "ESP_RST_TASK_WDT");
            break;
        case ESP_RST_WDT:        //!< Reset due to other watchdogs
            ESP_LOGI(TAG, "%s", "ESP_RST_WDT");
            break;
        case ESP_RST_DEEPSLEEP:  //!< Reset after exiting deep sleep mode
            ESP_LOGI(TAG, "%s", "ESP_RST_DEEPSLEEP");
            break;
        case ESP_RST_BROWNOUT:   //!< Brownout reset (software or hardware)
            ESP_LOGI(TAG, "%s", "ESP_RST_BROWNOUT");
            break;
        case ESP_RST_SDIO:       //!< Reset over SDIO
            ESP_LOGI(TAG, "%s", "ESP_RST_SDIO");
            break;
        default:
            break;
    }
}

static RTC_DATA_ATTR uint32_t bootCount;

void setup() {
    Serial.begin(115200);
    while ( ! Serial) {
        delay(1);
    }

    esp_reset_reason_t resetReason = esp_reset_reason();
    verbose_print_reset_reason(resetReason);

    // Try to avoid it getting optimized out.
    uxTopUsedPriority = configMAX_PRIORITIES - 1;

    //enable12V();

//    cat_m1.begin(io_expander);
//    LTE_Serial.begin(115200);
//    while(!LTE_Serial) {
//        delay(1);
//    }
//
//    delay(12 * 1000);
//
//    ESP_LOGI(TAG, "Sending ATI");
//
//    LTE_Serial.print("ATI\r\n");
//    ESP_LOGI(TAG, "Waiting for response");
//    while (true) {
//        if (LTE_Serial.available()) {
//            while (LTE_Serial.available()) {
//                Serial.write(LTE_Serial.read());
//            }
//        }
//
//        delay(10);
//    }

    Config& config = Config::get();
    config.load();

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
    bool uplinkCycle = bootCount % uimi == 0;

    ESP_LOGI(TAG, "Boot count: %lu, measurement interval: %u, uplink interval: %u, ui/mi: %u, uplink this cycle: %d", bootCount, mi, ui, uimi, uplinkCycle);

    pinMode(PROG_BTN, INPUT);
    attachInterrupt(PROG_BTN, progBtnISR, RISING);

    cliInitialise();

    // Flush log messages;
    Serial.flush();

    unsigned long setupEnd = millis();
    uint64_t sleepTime = (mi * 1000000) - (setupEnd * 1000);
    esp_sleep_enable_timer_wakeup(sleepTime);
    esp_deep_sleep_start();
}

extern sensor_list sensors;

void waitForChar(void) {
    while ( ! Serial.available()) {
        yield();
    }

    while (Serial.available()) {
        Serial.read();
    }
}

void loop() {
    if (progBtnPressed) {
        progBtnPressed = false;
        ESP_LOGI(TAG, "Button");
    }
}
