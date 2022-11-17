#include <Arduino.h>
#include "SensorTask.h"

#include <esp_log.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

//#include <esp32-dpi12.h>
#include <dpiclimate-12.h>

#define TAG "SensorTask"

#define SDI12_DATA_PIN  GPIO_NUM_27

SDI12 sdi12(SDI12_DATA_PIN);
DPIClimate12 dpi12(sdi12);

//ESP32_SDI12 dpi12(SDI12_DATA_PIN);
//ESP32_SDI12::Sensors sensors;

sensor_list sensors;

// While the SDI12 library uses uint8_t to express sensor addresses from 0-9, and we don't want
// to use address 0 in normal use, we can have at most 9 sensors.
constexpr size_t MAX_SENSORS = 10;
uint8_t addresses[MAX_SENSORS];

static constexpr uint8_t MAX_VALUES = 32;
static float values[MAX_VALUES];

void sensorTask(void *params) {
    TickType_t xLastWakeTime;
    const TickType_t xFrequency = pdMS_TO_TICKS(60 * 1000 * 10); // 10 mins
    BaseType_t xWasDelayed;

    uint8_t n_values;

    ESP_LOGI(TAG, "sensorTask starting");

    ESP_LOGI(TAG, "Initialising SDI-12");
    sdi12.begin();

    ESP_LOGI(TAG, "Default timeout on sdi12: %lu", sdi12.getTimeout());

    ESP_LOGI(TAG, "Scanning bus.");
    dpi12.scan_bus(sensors);

    ESP_LOGI(TAG, "Found %u sensors", sensors.count);
    for (uint8_t i = 0; i < sensors.count; i++) {
        ESP_LOGI(TAG, "%s", &sensors.sensors[i]);
    }

/*
    ESP32_SDI12::Status res;

    res = dpi12.sensorsOnBus(&sensors);
    if (res == ESP32_SDI12::SDI12_OK) {
        ESP_LOGI(TAG, "Found %d devices.", sensors.count);

        // Convert the ASCII digit addresses from the info response to
        // zero-based uint8_t values for use with the SDI12 library.
        for (size_t i = 0; i < sensors.count; i++) {
            addresses[i] = sensors.sensor[i].address - '0';
        }
    } else {
        sensors.count = 0;
    }
*/
    xLastWakeTime = xTaskGetTickCount ();
    // Tasks generally do not return.
    while (true) {
        ESP_LOGI(TAG, "sensorTask reading sensors");
/*
        for (size_t i = 0; i < sensors.count; i++) {
            ESP32_SDI12::Status res = dpi12.measure(addresses[i], values, MAX_VALUES, &n_values);
            if (res != ESP32_SDI12::SDI12_OK || n_values < 1) {
                Serial.printf("Measure for %u failed.\n", addresses[i]);
            } else {
                ESP_LOGI(TAG, "Values from sensor %u", addresses[i]);
                for (uint8_t v = 0; v < n_values; v++) {
                    ESP_LOGI(TAG, "%.2f", values[v]);
                }
            }
        }
*/
        for (size_t i = 0; i < sensors.count; i++) {
            int res = dpi12.do_measure(sensors.sensors[i].address, true);
            if (res > 0) {
                for (uint8_t v = 0; v < res; v++) {
                    ESP_LOGI(TAG, "Value %u: %.2f", v, dpi12.get_value(v).value);
                }
            } else {
                ESP_LOGE(TAG, "Failed to read sensor");
            }
        }

        ESP_LOGI(TAG, "sensorTask waiting for next period");
        xWasDelayed = xTaskDelayUntil( &xLastWakeTime, xFrequency );
    }
}
