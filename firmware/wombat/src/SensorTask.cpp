#include <Arduino.h>
#include "SensorTask.h"
#include "Config.h"
#include "TCA9534.h"
#include <esp_log.h>

#include <freertos/FreeRTOS.h>

#include <dpiclimate-12.h>

#define TAG "sensors"

#define SDI12_DATA_PIN  GPIO_NUM_27

extern TCA9534 io_expander;

SDI12 sdi12(SDI12_DATA_PIN);
DPIClimate12 dpi12(sdi12);

sensor_list sensors;

static void enable12V(void) {
    io_expander.output(1, TCA9534::Level::H);
}

static void disable12V(void) {
    io_expander.output(1, TCA9534::Level::L);
}

// While the SDI12 library uses uint8_t to express sensor addresses from 0-9, and we don't want
// to use address 0 in normal use, we can have at most 9 sensors.
constexpr size_t MAX_SENSORS = 10;
uint8_t addresses[MAX_SENSORS];

static constexpr uint8_t MAX_VALUES = 32;
static float values[MAX_VALUES];

void initSensors(void) {
    enable12V();
    delay(500);
    sdi12.begin();

    if (Config::get().getBootCount() == 0) {
        ESP_LOGI(TAG, "Scanning SDI-12 bus");
        dpi12.scan_bus(sensors);
        ESP_LOGI(TAG, "Found %u sensors", sensors.count);
        for (uint8_t i = 0; i < sensors.count; i++) {
            ESP_LOGI(TAG, "%s", &sensors.sensors[i]);
        }
    }

    sdi12.end();
    disable12V();
}

void sensorTask(void) {
    enable12V();
    sdi12.begin();
    delay(500);

    for (size_t i = 0; i < sensors.count; i++) {
        int res = dpi12.do_measure(sensors.sensors[i].address);
        if (res > 0) {
            for (uint8_t v = 0; v < res; v++) {
                ESP_LOGI(TAG, "Value %u: %.2f", v, dpi12.get_value(v).value);
            }
        } else {
            ESP_LOGE(TAG, "Failed to read sensor");
        }
    }

    sdi12.end();
    disable12V();
}
