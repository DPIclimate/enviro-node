#include <Arduino.h>
#include "SensorTask.h"
#include "DeviceConfig.h"
#include "Utils.h"
#include "globals.h"
#include "ulp.h"
#include "sd-card/interface.h"
#include "power_monitoring/battery.h"
#include "power_monitoring/solar.h"
#include <esp_log.h>

#include <freertos/FreeRTOS.h>

#include <dpiclimate-12.h>
#include <SPIFFS.h>

#define TAG "sensors"

#define SDI12_DATA_PIN  GPIO_NUM_21

SDI12 sdi12(SDI12_DATA_PIN);
DPIClimate12 dpi12(sdi12);

/// The ID information of each sensor found on the SDI-12 bus.
sensor_list sensors;

///
/// \brief Scans the SDI-12 bus and fills in the sensors object.
///
void init_sensors(void) {
    sdi12.begin();
    dpi12.scan_bus(sensors);
    sdi12.end();

    log_to_sdcardf("Found %u sensors", sensors.count);

    ESP_LOGI(TAG, "Found %u sensors", sensors.count);
    for (uint8_t i = 0; i < sensors.count; i++) {
        ESP_LOGI(TAG, "%s", &sensors.sensors[i]);
    }
}


/// \brief Read a single SDI-12 sensor and add its values to timeseries_array.
bool read_sensor(const char addr, JsonArray& timeseries_array) {
    // Will be true if the sensor is read using information from the sensor definitions. If that fails
    // and this is false after the attempt, then a standard measure command will be issued as a fall-back.
    bool have_reading = false;

    // If the sensor definition gives a C command this will be true.
    bool is_concurrent = false;

    // If the 3rd character of the command from a sensor definition is '0' < ch <= '9' then this
    // will be set to true to indicate an 'additional' measure or concurrent command is used.
    bool is_additional = false;

    std::vector<double> plain_values;
    size_t sensor_idx = 0;
    for (; sensor_idx < sensors.count; sensor_idx++) {
        if (addr == sensors.sensors[sensor_idx].address) {
            break;
        }
    }

    if (sensor_idx >= sensors.count) {
        ESP_LOGE(TAG, "SDI-12 sensor %c not found", addr);
        return false;
    }

    JsonObjectConst s = getSensorDefn(sensor_idx, sensors);
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
                double value = dpi12.get_value(value_idx).value;
                char mask = '1'; // Default to including the value.
                if (value_mask != nullptr && strnlen(value_mask, num_values) >= value_idx + 1) {
                    mask = value_mask[value_idx];
                }

                if (mask == '1') {
                    plain_values.push_back(value);
                } else {
                    ESP_LOGI(TAG, "Skipping value %d due to value mask", value_idx);
                }
            }
        }

        JsonArrayConst labels = s["labels"];

        // This loop runs through the kept values, finds or generates a label for them, and
        // appends them to the timeSeries array of the JSON document.
        for (int value_idx = 0; value_idx < plain_values.size(); value_idx++) {
            auto ts_entry = timeseries_array.add<JsonObject>();
            JsonVariantConst label = labels[value_idx];
            if (label) {
                String generated_label(sensors.sensors[sensor_idx].address - '0');
                generated_label += "_";
                generated_label += label.as<String>();
                ts_entry["name"] = generated_label;
            } else {
                snprintf(g_buffer, sizeof(MAX_G_BUFFER), "%c_V%d", sensors.sensors[sensor_idx].address, value_idx);
                ts_entry["name"] = g_buffer;
            }

            ts_entry["value"] = plain_values.at(value_idx);
        }

        have_reading = true;
    }

    if ( ! have_reading) {
        ESP_LOGI(TAG, "No sensor definition found, performing fallback plain measure command.");
        // Setting wait_full_time to true because we don't know what type of sensor we're reading
        // in this code so try to handle those sensors that send a service request but then don't
        // send the data immediately afterwards.
        int res = dpi12.do_measure(sensors.sensors[sensor_idx].address, true);
        if (res > 0) {
            for (uint8_t value_idx = 0; value_idx < res; value_idx++) {
                auto ts_entry = timeseries_array.add<JsonObject>();
                snprintf(g_buffer, MAX_G_BUFFER, "%c_V%u", sensors.sensors[sensor_idx].address, value_idx+1);
                ts_entry["name"] = g_buffer;
                ts_entry["value"] = dpi12.get_value(value_idx).value;
            }
        } else {
            ESP_LOGE(TAG, "Failed to read sensor");
            return false;
        }
    }

    return true;
}


/// \brief Read all sensors, put the readings in a JSON message and save the JSON to a file on SPIFFS to be uplinked later.
void sensor_task(void) {
    sdi12.begin();

    JsonDocument msg;

    const char *timestamp = iso8601();
    msg["timestamp"] = timestamp;

    auto source_ids = msg["source_ids"].to<JsonObject>();
    source_ids["serial_no"] = DeviceConfig::get().node_id;

    constexpr size_t buffer_sz = 48;
    char buffer[buffer_sz + 1];
    memset(buffer, 0, sizeof(buffer));
    snprintf(buffer, buffer_sz, "%d.%d.%d %s %s %s", ver_major, ver_minor, ver_update, repo_branch, commit_id, repo_status);
    source_ids["firmware"] = buffer;
    JsonArray timeseries_array = msg["timeseries"].to<JsonArray>();

    //
    // Node sensors
    //
    auto battery_v = timeseries_array.add<JsonObject>();
    battery_v["name"] = "battery (v)";
    battery_v["value"] = BatteryMonitor::get_voltage();

    auto solar_v = timeseries_array.add<JsonObject>();
    solar_v["name"] = "solar (v)";
    solar_v["value"] = SolarMonitor::get_voltage();

    if (r5_ok) {
        signal_quality sq;
        SARA_R5_error_t r5_err = r5.getExtSignalQuality(sq);

        if (!r5_err) {
            JsonObject rsrq = timeseries_array.add<JsonObject>();
            rsrq["name"] = "rsrq";
            rsrq["value"] = sq.rsrq;

            JsonObject rsrp = timeseries_array.add<JsonObject>();
            rsrp["name"] = "rsrp";
            rsrp["value"] = sq.rsrp;
        }

        String ccid = r5.getCCID();
        if (ccid.length() > 0) {
            source_ids["ccid"] = ccid;
        }
    }
    //
    // Pulse counter
    //
    uint32_t pc = get_pulse_count();
    uint32_t sp = get_shortest_pulse();

    auto pulse_count = timeseries_array.add<JsonObject>();
    pulse_count["name"] = "pulse_count";
    pulse_count["value"] = pc;

    auto shortest_pulse = timeseries_array.add<JsonObject>();
    shortest_pulse["name"] = "shortest_pulse";
    shortest_pulse["value"] = sp;

    //
    // SDI-12 sensors
    //

    // NOTE: This may make the message too long to send directly via MQTT on the
    // SMP nodes because the 6 SDI-12 ID strings add about 200 bytes to the message.
    auto sdi12_ids = source_ids["sdi-12"].to<JsonArray>();
    for (size_t sensor_idx = 0; sensor_idx < sensors.count; sensor_idx++) {
        read_sensor(sensors.sensors[sensor_idx].address, timeseries_array);
        sdi12_ids.add((char*)&sensors.sensors[sensor_idx]);
    }

    sdi12.end();

    String str;
    serializeJson(msg, str);
    ESP_LOGI(TAG, "Msg:\r\n%s\r\n", str.c_str());

    if (spiffs_ok) {
        // Write the message to a file so it can be sent on the next uplink cycle.
        static const size_t MAX_FNAME = 32;
        static char filename[MAX_FNAME + 1];

        snprintf(filename, MAX_FNAME, "/%s%s.json", DeviceConfig::getMsgFilePrefix(), timestamp);
        ESP_LOGI(TAG, "Creating unsent msg file [%s]", filename);
        File f = SPIFFS.open(filename, FILE_WRITE);
        serializeJson(msg, f);
        f.close();
    } else {
        log_to_sdcard("[E] spiffs_ok is false, no message stored");
    }

    // Append the message to a file on the SD card.
    if (SDCardInterface::is_ready()) {
        snprintf(g_buffer, MAX_G_BUFFER, "%s,\n", str.c_str());
        SDCardInterface::append_to_file(sd_card_datafile_name, g_buffer);
    }
}
