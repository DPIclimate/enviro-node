#include <Arduino.h>
#include "SensorTask.h"
#include "DeviceConfig.h"
#include "TCA9534.h"
#include "Utils.h"
#include <esp_log.h>

#include <freertos/FreeRTOS.h>

#include <dpiclimate-12.h>


#define TAG "sensors"

#define SDI12_DATA_PIN  GPIO_NUM_27

SDI12 sdi12(SDI12_DATA_PIN);
DPIClimate12 dpi12(sdi12);

sensor_list sensors;

void initSensors(void) {
    enable12V();
    sdi12.begin();

    if (DeviceConfig::get().getBootCount() == 0) {
        ESP_LOGI(TAG, "Scanning SDI-12 bus");
        dpi12.scan_bus(sensors);
    }

    sdi12.end();
    disable12V();

    ESP_LOGI(TAG, "Found %u sensors", sensors.count);
    for (uint8_t i = 0; i < sensors.count; i++) {
        ESP_LOGI(TAG, "%s", &sensors.sensors[i]);
    }
}

void sensorTask(void) {
    enable12V();
    sdi12.begin();

    const JsonDocument& sdi12Defns = DeviceConfig::get().getSDI12Defns();

    for (size_t i = 0; i < sensors.count; i++) {
        // Continuous and custom commands are not supported yet. CRCs are not supported yet.

        // Will be true if the sensor is read using information from the sensor definitions. If that fails
        // and this is false after the attempt, then a standard measure command will be issued as a fall-back.
        bool haveReading = false;

        // If the sensor definition gives a C command this will be true.
        bool is_concurrent = false;

        // If the 3rd character of the command from a sensor definition is '0' < ch <= '9' then this
        // will be set to true to indicate an 'additional' measure or concurrent command is used.
        bool is_additional = false;

        std::vector<float> values;
        JsonObjectConst s = getSensorDefn(sensors.sensors[i]);
        if (s) {
            JsonArrayConst read_cmds = s["read_cmds"];
            const char* value_mask = s["value_mask"];
            if (value_mask != nullptr) {
                ESP_LOGI(TAG, "value_mask: %s", value_mask);
            } else {
                ESP_LOGI(TAG, "No value mask");
            }

            JsonArrayConst labels = s["labels"];

            for (size_t a = 0; a < read_cmds.size(); a++) {
                const char *crc = read_cmds[a];

                is_concurrent = crc[1] == 'C';
                is_additional = crc[2] > '0' && crc[2] <= '9';

                int num_values = 0;

                if (is_concurrent) {
                    if (is_additional) {
                        ESP_LOGI(TAG, "Concurrent (additional)");
                        num_values = dpi12.do_additional_concurrent(sensors.sensors[i].address, crc[2]);
                    } else {
                        ESP_LOGI(TAG, "Concurrent");
                        num_values = dpi12.do_concurrent(sensors.sensors[i].address);
                    }
                } else {
                    bool wait_full_time = s["ignore_sr"];
                    if (is_additional) {
                        ESP_LOGI(TAG, "Measure (additional), wait_full_time: %d", wait_full_time);
                        num_values = dpi12.do_additional_measure(sensors.sensors[i].address, crc[2]);
                    } else {
                        ESP_LOGI(TAG, "Measure, wait_full_time: %d", wait_full_time);
                        num_values = dpi12.do_measure(sensors.sensors[i].address, wait_full_time);
                    }
                }

                for (int v = 0; v < num_values; v++) {
                    float value = dpi12.get_value(v).value;
                    char mask = '1'; // Default to including the value.
                    if (value_mask != nullptr && strnlen(value_mask, num_values) >= v+1) {
                        mask = value_mask[v];
                    }

                    if (mask == '1') {
                        values.push_back(value);
                    } else {
                        ESP_LOGI(TAG, "Skipping value %d due to value mask", v);
                    }
                }
            }

            for (int v = 0; v < values.size(); v++) {
                JsonVariantConst label = labels[v];
                if (label) {
                    ESP_LOGI(TAG, "%d %s: %.02f", v, label.as<String>().c_str(), values.at(v));
                } else {
                    ESP_LOGI(TAG, "%d: %.02f", v, values.at(v));
                }
            }

            haveReading = true;
        }

        if ( ! haveReading) {
            ESP_LOGI(TAG, "No sensor definition found, performing fallback plain measure command.");
            int res = dpi12.do_measure(sensors.sensors[i].address);
            if (res > 0) {
                for (uint8_t v = 0; v < res; v++) {
                    ESP_LOGI(TAG, "Value %u: %.2f", v, dpi12.get_value(v).value);
                }
            } else {
                ESP_LOGE(TAG, "Failed to read sensor");
            }
        }
    }

    sdi12.end();
    disable12V();
}
