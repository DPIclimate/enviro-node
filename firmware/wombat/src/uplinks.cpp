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

static mqtt_status_t mqtt_status = MQTT_UNINITIALISED;

static String topic("wombat");

static constexpr size_t MAX_MSG_LEN = 1024;
static char msg_buf[MAX_MSG_LEN + 1];

static constexpr size_t MAX_FILENAME_LEN = 64;
static char msg_filename[MAX_FILENAME_LEN + 1];

static bool process_file(File& file) {
    ESP_LOGI(TAG, "Processing message file [%s]", file.name());

    size_t len = file.available();
    if (len > MAX_MSG_LEN) {
        ESP_LOGE(TAG, "Message too long");
        file.close();
        // This signals the message file can be deleted. If it is too long to send
        // there is no point trying again later.
        return true;

    }

    // Read and close the message file now in case there is a config script coming
    // that wants to use SPIFFS.
    file.readBytes(msg_buf, len);
    msg_buf[len] = 0;
    file.close();

    if (mqtt_status == MQTT_UNINITIALISED) {
        connect_to_internet();
        mqtt_status = mqtt_login() ? MQTT_LOGIN_OK : MQTT_LOGIN_FAILED;
        if (mqtt_status == MQTT_LOGIN_FAILED) {
            ESP_LOGI(TAG, "Not processing file, no MQTT connection");
            return false;
        }
    }

    // This is not always true - if this function is called after a failed login then
    // we want to skip publishing the message.
    if (mqtt_status == MQTT_LOGIN_OK) {
        return mqtt_publish(topic, msg_buf, len);
    }

    return false;
}

void send_messages(void) {
    if (SPIFFS.begin()) {
        File root = SPIFFS.open("/");
        if ( ! root) {
            ESP_LOGE(TAG, "Failed to open root directory of SPIFFS");
            return;
        }

        const char *msg_file_prefix = DeviceConfig::getMsgFilePrefix();
        const size_t msg_file_prefix_len = strlen(msg_file_prefix);

        uint16_t file_count = 0;
        uint16_t upload_errors = 0;
        File file = root.openNextFile();
        while (file) {
            bool remove_file = false;
            const char *filename = file.name();
            // Copy the filename for the delete operation because the above pointer
            // becomes invalid after the file is closed.
            strncpy(msg_filename, filename, MAX_FILENAME_LEN);

            if ( ! file.isDirectory()) {
                if ( ! strncmp(msg_filename, msg_file_prefix, msg_file_prefix_len)) {
                    if (file_count > 0) {
                        delay(250);
                    }

                    // NOTE: process_file is responsible for closing the file.
                    // This is due to weirdness around configs being downloaded and
                    // saved to SPIFFS. process_file must read the message file and
                    // close it before trying to log in to MQTT. Needs a redesign.
                    remove_file = process_file(file);
                    file_count++;
                    if ( ! remove_file) {
                        upload_errors++;
                    }
                }
            }

            if (remove_file) {
                snprintf(g_buffer, MAX_G_BUFFER, "/%s", msg_filename);
                SPIFFS.remove(g_buffer);
            }

            if (mqtt_status == MQTT_LOGIN_FAILED) {
                ESP_LOGW(TAG, "MQTT login failed, skipping any further messages");
                break;
            }

            file = root.openNextFile();
        }

        root.close();
        SPIFFS.end();

        ESP_LOGI(TAG, "Processed %u files with %u upload errors", file_count, upload_errors);
    }

    if (mqtt_status == MQTT_LOGIN_OK) {
        mqtt_logout();
    }

    mqtt_status = MQTT_UNINITIALISED;
}
