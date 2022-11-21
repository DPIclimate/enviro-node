#include "DeviceConfig.h"
#include <esp_log.h>

#include <SPIFFS.h>

#include "Utils.h"

#define TAG "config"

constexpr const char* config_filename = "/config";

#define BUF_SIZE 64
static char buf[BUF_SIZE+1];

static RTC_DATA_ATTR uint32_t bootCount = 0;

DeviceConfig::DeviceConfig() : uplink_interval(60), measure_interval(15) {
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
            }

            f.close();
        } else {
            ESP_LOGE(TAG, "DeviceConfig file not found");
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
    stream.print("interval measure "); stream.println(measure_interval);
    stream.print("interval uplink "); stream.println(uplink_interval);
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
