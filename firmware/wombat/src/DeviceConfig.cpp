/**
 * @file DeviceConfig.h
 *
 * @brief Get and set device configuration values.
 *
 * @date December 2022
 */
#include "DeviceConfig.h"

#include <SPIFFS.h>

#include "Utils.h"
#include "cli/FreeRTOS_CLI.h"
#include "cli/device_config/acquisition_intervals.h"
#include "cli/device_config/mqtt_cli.h"
#include "cli/device_config/ftp_cli.h"
#include "globals.h"

//! ESP32 debug output tag
#define TAG "config"

//! Default configuration path
constexpr const char* config_filename = "/config";

//! Size of buffer for reading and writing
#define BUF_SIZE 64
//! Buffer for sending commands
static char buf[BUF_SIZE+1];
//! Buffer for reading responses
static char rsp[BUF_SIZE+1];

//! Counter on the number of times the ESP32 has been re-booted
static RTC_DATA_ATTR uint32_t bootCount = 0;

//! JSON document instance to handle SDI-12 device definitions
JsonDocument sdi12Defns;

/**
 * @brief Device configuration constructor with default values.
 *
 * @todo Several values here are not initialised
 */
DeviceConfig::DeviceConfig() : uplink_interval(3600), measure_interval(900),
mqttHost(), mqttUser(), mqttPassword() {
    ESP_LOGI(TAG, "Constructing instance");
    bootCount++;
}

/**
 * @brief Reset the DeviceConfig values to their default states.
 *
 * This function resets the measure interval, uplink interval, MAC address,
 * and MQTT connection parameters (host, port, user, and password) to their
 * default values.
 *
 * @see measure_interval
 * @see uplink_interval
 * @see mac
 * @see node_id
 * @see mqttHost
 * @see mqttPort
 * @see mqttUser
 * @see mqttPassword
 */
void DeviceConfig::reset() {
    ESP_LOGI(TAG, "Resetting values to defaults");
    measure_interval = 900;
    uplink_interval = 3600;

    esp_efuse_mac_get_default(mac);
    snprintf(DeviceConfig::node_id, 13, "%02X%02X%02X%02X%02X%02X%02X%02X", DeviceConfig::mac[0], DeviceConfig::mac[1], DeviceConfig::mac[2], DeviceConfig::mac[3], DeviceConfig::mac[4], DeviceConfig::mac[5], DeviceConfig::mac[6], DeviceConfig::mac[7]);
    ESP_LOGI(TAG, "Node Id: %s", node_id);

    mqttHost.clear();
    mqttPort = 1833;
    mqttUser.clear();
    mqttPassword.clear();
}

/**
 * @brief Load the DeviceConfig values from a file on the file system.
 *
 * This function loads the DeviceConfig values from a file on the file system
 * and sets any missing values to their default states. It also loads the SDI-12
 * sensor definitions from a file on the file system.
 *
 * @see reset
 * @see config_filename
 * @see sdi12defn_spiffs
 * @see sdi12Defns
 */
void DeviceConfig::load() {
    // Ensure any settings not present in the config file have the default value.
    reset();

    if (spiffs_ok) {
        if (SPIFFS.exists(config_filename)) {
            File f = SPIFFS.open(config_filename, FILE_READ);
            size_t len;
            while (f.available() > 0) {
                memset(buf, 0, sizeof(buf));
                f.readBytesUntil('\n', buf, BUF_SIZE);
                len = wombat::stripWS(buf);
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

        if (SPIFFS.exists(sdi12defn_spiffs)) {
            File f = SPIFFS.open(sdi12defn_spiffs, FILE_READ);
            while (f.available() > 0) {
                DeserializationError err = deserializeJson(sdi12Defns, f);
                f.close();
                if (err) {
                    ESP_LOGE(TAG, "Failed to load SDI-12 sensor definitions: %s", err.f_str());
                } else {
//                    std::string str;
//                    serializeJsonPretty(sdi12Defns, str);
//                    ESP_LOGI(TAG, "SDI-12 sensor definitions:\r\n%s\r\n", str.c_str());
                }
            }
        } else {
            ESP_LOGE(TAG, "File not found: %s", sdi12defn_spiffs);
        }
    } else {
        ESP_LOGE(TAG, "Failed to initialise SPIFFS");
    }
}

/**
 * @brief Save device configuration to SPIFFS storage.
 */
void DeviceConfig::save() {
    if (spiffs_ok) {
        File f = SPIFFS.open(config_filename, FILE_WRITE);
        dumpConfig(f);
        f.close();
    } else {
        ESP_LOGE(TAG, "Failed to initialise SPIFFS");
    }
}

/**
 * @brief Print out device configuration to a stream.
 *
 * @param stream Output stream.
 */
void DeviceConfig::dumpConfig(Stream& stream) {
    CLIConfigIntervals::dump(stream);
    CLIMQTT::dump(stream);
    CLIFTP::dump(stream);
}

/**
 * @brief Method to get the number of times the ESP32 has rebooted.
 * bootCount is incremented when the singleton is created, meaning at power-on
 * it will be 1 by the time any code can ask for it. It is easier to do modulo
 * arithmetic with a 0-based bootCount so adjust it before returning it.
 *
 * @return Boot counter.
 */
uint32_t DeviceConfig::getBootCount(void) {
    return bootCount - 1;
}

/**
 * @brief Set the measurement interval.
 *
 * @param seconds Time in seconds.
 */
void DeviceConfig::setMeasureInterval(const uint16_t seconds) {
    measure_interval = seconds;
}

/**
 * @brief Set the uplink interval.
 *
 * @param seconds Time in seconds.
 */
void DeviceConfig::setUplinkInterval(const uint16_t seconds) {
    if (seconds % measure_interval != 0) {
        ESP_LOGE(TAG, "uplink_interval must be a whole multiple of measurement_interval");
        return;
    }

    uplink_interval = seconds;
}

/**
 * @brief Set both the measurement and uplink intervals.
 *
 * @warning The measurement interval should be divisible by the uplink interval
 * with no remainder.
 *
 * @param measurement_seconds Measurement interval in seconds.
 * @param uplink_seconds Uplink interval in seconds.
 */
void DeviceConfig::setMeasurementAndUplinkIntervals(const uint16_t measurement_seconds, const uint16_t uplink_seconds) {
    if (uplink_seconds % measurement_seconds != 0) {
        ESP_LOGE(TAG, "uplink_interval must be a whole multiple of measurement_interval");
        return;
    }

    measure_interval = measurement_seconds;
    uplink_interval = uplink_seconds;
}

/**
 * @brief Get the SDI-12 device definitions from SPIFFS storage.
 *
 * @return SDI-12 device definitions as a JSONDocument.
 */
const JsonDocument& DeviceConfig::getSDI12Defns(void) { return sdi12Defns; }
