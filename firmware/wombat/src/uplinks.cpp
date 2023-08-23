#include <SPIFFS.h>

#include "globals.h"

#include "DeviceConfig.h"
#include "uplinks.h"
#include "mqtt_stack.h"
#include "Utils.h"

#define TAG "uplinks"

enum mqtt_status_t {
    MQTT_UNINITIALISED,
    MQTT_LOGIN_OK,
    MQTT_LOGIN_FAILED
};

static volatile mqtt_status_t mqtt_status = MQTT_UNINITIALISED;

static String topic("wombat");

static constexpr size_t MAX_MSG_LEN = 4096;
static char msg_buf[MAX_MSG_LEN + 1];

/**
 * Sends the given file via MQTT.
 *
 * @param filename the file to send, including the leading '/' required by SPIFFS.
 *
 * @return true if the message was sent ok, otherwise false.
 */
static bool process_file(const String& filename) {
    ESP_LOGI(TAG, "Processing message file [%s]", filename.c_str());

    File file = SPIFFS.open(filename);
    // Just in case a directory shows up with a match on the filename pattern. May as well say it
    // was processed ok.
    if (file.isDirectory()) {
        file.close();
        return false;
    }

    size_t len = file.available();
    if (len < MAX_MSG_LEN) {
        // Read and close the message file now in case there is a config script coming
        // that wants to use SPIFFS.
        file.readBytes(msg_buf, len);
        msg_buf[len] = 0;
        file.close();

        if (mqtt_status == MQTT_UNINITIALISED) {
            connect_to_internet();
            mqtt_status = mqtt_login() ? MQTT_LOGIN_OK : MQTT_LOGIN_FAILED;
            if (mqtt_status == MQTT_LOGIN_FAILED) {
                ESP_LOGE(TAG, "Not processing file, no MQTT connection");
                return false;
            }
        }

        // This is not always true - if this function is called after a failed login then
        // we want to skip publishing the message.
        if (mqtt_status == MQTT_LOGIN_OK) {
            if (mqtt_publish(topic, msg_buf, len)) {
                SPIFFS.remove(filename);
                return true;
            }
        }
    } else {
        ESP_LOGE(TAG, "Message too long");
        file.close();
        SPIFFS.remove(filename);
    }

    return false;
}

void send_messages(void) {
    if (spiffs_ok) {
        File root = SPIFFS.open("/");
        if ( ! root) {
            ESP_LOGE(TAG, "Failed to open root directory of SPIFFS");
            return;
        }

        const char *msg_file_prefix = DeviceConfig::getMsgFilePrefix();
        const size_t msg_file_prefix_len = strlen(msg_file_prefix);

        uint16_t file_count = 0;
        uint16_t upload_errors = 0;
        String filename = root.getNextFileName();
        while (filename.length() > 0) {
            ESP_LOGD(TAG, "Found file %s", filename.c_str());
            if ( ! strncmp(&filename.c_str()[1], msg_file_prefix, msg_file_prefix_len)) {
                if (file_count > 0) {
                    delay(250);
                }

                if (!process_file(filename)) {
                    upload_errors++;
                }
                file_count++;
            }

            if (mqtt_status == MQTT_LOGIN_FAILED) {
                ESP_LOGW(TAG, "MQTT login failed, skipping any further messages");
                break;
            }

            filename = root.getNextFileName();
        }

        root.close();

        ESP_LOGI(TAG, "Processed %u files with %u upload errors", file_count, upload_errors);
    }

    if (mqtt_status == MQTT_UNINITIALISED) {
        ESP_LOGI(TAG, "No files caused an MQTT connection, trying now to look for waiting config scripts");
        connect_to_internet();
        mqtt_status = mqtt_login() ? MQTT_LOGIN_OK : MQTT_LOGIN_FAILED;
        if (mqtt_status == MQTT_LOGIN_FAILED) {
            ESP_LOGE(TAG, "MQTT connection failed");
        }
    }

    if (mqtt_status == MQTT_LOGIN_OK) {
        mqtt_logout();
    }

    mqtt_status = MQTT_UNINITIALISED;
}
