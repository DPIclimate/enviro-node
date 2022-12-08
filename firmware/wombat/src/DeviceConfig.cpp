#include "DeviceConfig.h"
#include <esp_log.h>

#include <SPIFFS.h>

#include "Utils.h"
#include "cli/FreeRTOS_CLI.h"
#include "cli/device_config/acquisition_intervals.h"
#include "cli/device_config/mqtt_cli.h"

#define TAG "config"

constexpr const char* config_filename = "/config";
constexpr const char* sdi12defn_filename = "/sdi12defn.json";

#define BUF_SIZE 64
static char buf[BUF_SIZE+1];
static char rsp[BUF_SIZE+1];

static RTC_DATA_ATTR uint32_t bootCount = 0;

DynamicJsonDocument sdi12Defns(1024);

DeviceConfig::DeviceConfig() : uplink_interval(60), measure_interval(15), mqttHost(), mqttUser(), mqttPassword() {
    ESP_LOGI(TAG, "Constructing instance");
    bootCount++;
}

void DeviceConfig::reset() {
    ESP_LOGI(TAG, "Resetting values to defaults");
    measure_interval = 15;
    uplink_interval = 60;

    esp_efuse_mac_get_default(mac);
    snprintf(DeviceConfig::node_id, 13, "%02X%02X%02X%02X%02X%02X%02X%02X", DeviceConfig::mac[0], DeviceConfig::mac[1], DeviceConfig::mac[2], DeviceConfig::mac[3], DeviceConfig::mac[4], DeviceConfig::mac[5], DeviceConfig::mac[6], DeviceConfig::mac[7]);
    ESP_LOGI(TAG, "Node Id: %s", node_id);

    mqttHost.clear();
    mqttPort = 1833;
    mqttUser.clear();
    mqttPassword.clear();
}

void DeviceConfig::load() {
    // Ensure any settings not present in the config file have the default value.
    reset();

    if (SPIFFS.begin()) {
        if (SPIFFS.exists(config_filename)) {
            File f = SPIFFS.open(config_filename, FILE_READ);
            size_t len;
            while (f.available() > 0) {
                memset(buf, 0, sizeof(buf));
                f.readBytesUntil('\n', buf, BUF_SIZE);
                len = stripWS(buf);
                if (len < 1) {
                    continue;
                }

                if (buf[0] == '#' || buf[0] == ';') {
                    continue;
                }

                ESP_LOGD(TAG, "config cmd: [%s] [%u]", buf, len);
                BaseType_t rc = pdTRUE;
                while (rc != pdFALSE) {
                    rc = FreeRTOS_CLIProcessCommand(buf, rsp, BUF_SIZE);
                    ESP_LOGD(TAG, "%s", rsp);
                }
            }

            f.close();
        } else {
            ESP_LOGE(TAG, "File not found: %s", config_filename);
        }

        if (SPIFFS.exists(sdi12defn_filename)) {
            File f = SPIFFS.open(sdi12defn_filename, FILE_READ);
            while (f.available() > 0) {
                DeserializationError err = deserializeJson(sdi12Defns, f);
                f.close();
                if (err) {
                    ESP_LOGE(TAG, "Failed to load SDI-12 sensor definitions: %s", err.f_str());
                } else {
                    std::string str;
                    serializeJsonPretty(sdi12Defns, str);
                    ESP_LOGI(TAG, "SDI-12 sensor definitions:\r\n%s\r\n", str.c_str());
                }
            }
        } else {
            ESP_LOGE(TAG, "File not found: %s", sdi12defn_filename);
        }
    } else {
        ESP_LOGE(TAG, "Failed to initialise SPIFFS");
    }

    SPIFFS.end();
}

void DeviceConfig::save() {
    if (SPIFFS.begin()) {
        File f = SPIFFS.open(config_filename, FILE_WRITE);
        dumpConfig(f);
        f.close();
    } else {
        ESP_LOGE(TAG, "Failed to initialise SPIFFS");
    }

    SPIFFS.end();
}

void DeviceConfig::dumpConfig(Stream& stream) {
    CLIConfigIntervals::dump(stream);
    CLIMQTT::dump(stream);
}

uint32_t DeviceConfig::getBootCount(void) {
    // bootCount is incremented when the singleton is created, meaning at power-on it will
    // be 1 by the time any code can ask for it. It is easier to do modulo arithmetic with
    // a 0-based bootCount so adjust it before returning it.
    return bootCount - 1;
}

void DeviceConfig::setMeasureInterval(const uint16_t minutes) {
    measure_interval = minutes;
}

void DeviceConfig::setUplinkInterval(const uint16_t minutes) {
    if (minutes % measure_interval != 0) {
        ESP_LOGE(TAG, "uplink_interval must be a whole multiple of measurement_interval");
        return;
    }

    uplink_interval = minutes;
}

void DeviceConfig::setMeasurementAndUplinkIntervals(const uint16_t measurement_seconds, const uint16_t uplink_seconds) {
    if (uplink_seconds % measurement_seconds != 0) {
        ESP_LOGE(TAG, "uplink_interval must be a whole multiple of measurement_interval");
        return;
    }

    measure_interval = measurement_seconds;
    uplink_interval = uplink_seconds;
}

const JsonDocument& DeviceConfig::getSDI12Defns(void) { return sdi12Defns; }
