#include "Config.h"
#include <esp_log.h>

#include <SPIFFS.h>

#include "Utils.h"

#define TAG "config"

constexpr const char* config_filename = "/config";

#define BUF_SIZE 64
static char buf[BUF_SIZE+1];

Config::Config() : uplink_interval(60), measure_interval(15) {
    ESP_LOGI(TAG, "Constructing instance");
}

void Config::reset() {
    ESP_LOGI(TAG, "Resetting values to defaults");
    measure_interval = 15;
    uplink_interval = 60;

    esp_efuse_mac_get_default(mac);
    snprintf(Config::node_id, 13, "%02X%02X%02X%02X%02X%02X%02X%02X", Config::mac[0], Config::mac[1], Config::mac[2], Config::mac[3], Config::mac[4], Config::mac[5], Config::mac[6], Config::mac[7]);
    ESP_LOGI(TAG, "Node Id: %s", node_id);
}

void Config::load() {
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
            ESP_LOGE(TAG, "Config file not found");
        }
    } else {
        ESP_LOGE(TAG, "Failed to initialise SPIFFS");
    }

    SPIFFS.end();
}

void Config::save() {
    if (SPIFFS.begin()) {
        File f = SPIFFS.open(config_filename, FILE_WRITE);
        dumpConfig(f);
        f.close();
    } else {
        ESP_LOGE(TAG, "Failed to initialise SPIFFS");
    }

    SPIFFS.end();
}

void Config::dumpConfig(Stream& stream) {
    stream.print("interval measure "); stream.println(measure_interval);
    stream.print("interval uplink "); stream.println(uplink_interval);
}

void Config::setMeasureInterval(const uint16_t minutes) {
    measure_interval = minutes;
}

void Config::setUplinkInterval(const uint16_t minutes) {
    if (minutes % measure_interval != 0) {
        ESP_LOGE(TAG, "uplink_interval must be a whole multiple of measurement_interval");
        return;
    }

    uplink_interval = minutes;
}

void Config::setMeasurementAndUplinkIntervals(const uint16_t measurement_seconds, const uint16_t uplink_seconds) {
    if (uplink_seconds % measurement_seconds != 0) {
        ESP_LOGE(TAG, "uplink_interval must be a whole multiple of measurement_interval");
        return;
    }

    measure_interval = measurement_seconds;
    uplink_interval = uplink_seconds;
}
