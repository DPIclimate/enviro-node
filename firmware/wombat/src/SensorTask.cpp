#include <Arduino.h>
#include "SensorTask.h"
#include "DeviceConfig.h"
#include "Utils.h"
#include "mqtt_stack.h"
#include <esp_log.h>

#include <freertos/FreeRTOS.h>

#include <dpiclimate-12.h>

#define TAG "sensors"

#define SDI12_DATA_PIN  GPIO_NUM_21

SDI12 sdi12(SDI12_DATA_PIN);
DPIClimate12 dpi12(sdi12);

sensor_list sensors;

inline static double round2(double value) {
    return (int)(value * 100 + 0.5) / 100.0;
}

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

    DynamicJsonDocument msg(2048);

    msg["timestamp"] = iso8601();
    JsonObject source_ids = msg.createNestedObject();
    source_ids["serial_no"] = DeviceConfig::get().node_id;
    JsonArray timeseries_array = msg.createNestedArray("timeseries");

    char unknown_label[] = {'V', 0, 0};

    for (size_t sensor_idx = 0; sensor_idx < sensors.count; sensor_idx++) {
        // Will be true if the sensor is read using information from the sensor definitions. If that fails
        // and this is false after the attempt, then a standard measure command will be issued as a fall-back.
        bool have_reading = false;

        // If the sensor definition gives a C command this will be true.
        bool is_concurrent = false;

        // If the 3rd character of the command from a sensor definition is '0' < ch <= '9' then this
        // will be set to true to indicate an 'additional' measure or concurrent command is used.
        bool is_additional = false;

        std::vector<float> plain_values;

        JsonObjectConst s = getSensorDefn(sensors.sensors[sensor_idx]);
        if (s) {
            JsonArrayConst read_cmds = s["read_cmds"];
            const char* value_mask = s["value_mask"];

            // This loop issues all the read and data commands for the sensor, and gathers the values
            // that pass the value mask into the values vector.
            for (size_t cmd_idx = 0; cmd_idx < read_cmds.size(); cmd_idx++) {
                const char *crc = read_cmds[cmd_idx];

                is_concurrent = crc[1] == 'C';
                is_additional = crc[2] > '0' && crc[2] <= '9';

                int num_values = 0;

                if (is_concurrent) {
                    if (is_additional) {
                        ESP_LOGI(TAG, "Concurrent (additional)");
                        num_values = dpi12.do_additional_concurrent(sensors.sensors[sensor_idx].address, crc[2]);
                    } else {
                        ESP_LOGI(TAG, "Concurrent");
                        num_values = dpi12.do_concurrent(sensors.sensors[sensor_idx].address);
                    }
                } else {
                    bool wait_full_time = s["ignore_sr"];
                    if (is_additional) {
                        ESP_LOGI(TAG, "Measure (additional), wait_full_time: %d", wait_full_time);
                        num_values = dpi12.do_additional_measure(sensors.sensors[sensor_idx].address, crc[2]);
                    } else {
                        ESP_LOGI(TAG, "Measure, wait_full_time: %d", wait_full_time);
                        num_values = dpi12.do_measure(sensors.sensors[sensor_idx].address, wait_full_time);
                    }
                }

                for (int value_idx = 0; value_idx < num_values; value_idx++) {
                    float value = dpi12.get_value(value_idx).value;
                    char mask = '1'; // Default to including the value.
                    if (value_mask != nullptr && strnlen(value_mask, num_values) >= value_idx + 1) {
                        mask = value_mask[value_idx];
                    }

                    if (mask == '1') {
                        // FIXME: This is an attempt to stop the floats in the JSON having so many
                        // FIXME: digits after the decimal point. It is not working yet.
                        plain_values.push_back(round2(value));
                    } else {
                        ESP_LOGI(TAG, "Skipping value %d due to value mask", value_idx);
                    }
                }
            }

            JsonArrayConst labels = s["labels"];

            // This loop runs through the kept values, finds or generates a label for them, and
            // appends them to the timeSeries array of the JSON document.
            for (int value_idx = 0; value_idx < plain_values.size(); value_idx++) {
                JsonObject ts_entry = timeseries_array.createNestedObject();
                JsonVariantConst label = labels[value_idx];
                if (label) {
                    String generated_label(sensors.sensors[sensor_idx].address - '0');
                    generated_label += "_";
                    generated_label += label.as<String>();
                    ts_entry["name"] = generated_label;
                } else {
                    unknown_label[1] = '0' + value_idx;
                    ts_entry["name"] = unknown_label;
                }

                ts_entry["value"] = plain_values.at(value_idx);
            }

            have_reading = true;
        }

        if ( ! have_reading) {
            ESP_LOGI(TAG, "No sensor definition found, performing fallback plain measure command.");
            int res = dpi12.do_measure(sensors.sensors[sensor_idx].address);
            if (res > 0) {
                for (uint8_t value_idx = 0; value_idx < res; value_idx++) {
                    JsonObject ts_entry = timeseries_array.createNestedObject();
                    unknown_label[1] = '0' + value_idx;
                    ts_entry["name"] = unknown_label;
                    ts_entry["value"] = dpi12.get_value(value_idx).value;
                }
            } else {
                ESP_LOGE(TAG, "Failed to read sensor");
            }
        }

        // FIXME: This delay is here to try and avoid noise on the SDI-12 line of the current
        // FIXME: revision of the board. It was not necessary for the previous revision.
        delay(1000);
    }

    String str;
    serializeJson(msg, str);
    ESP_LOGI(TAG, "Msg:\r\n%s\r\n", str.c_str());

    bool mqtt_ok = mqtt_login();
    String mqtt_topic("wombat");

    if (mqtt_ok) {
        mqtt_publish(mqtt_topic, str);
        mqtt_logout();
    }

    sdi12.end();
    disable12V();
}
