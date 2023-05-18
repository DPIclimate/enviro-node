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

static bool process_file(File& file) {
    ESP_LOGI(TAG, "Processing message file [%s]", file.name());

    if (mqtt_status == MQTT_UNINITIALISED) {
        connect_to_internet();
        mqtt_status = mqtt_login() ? MQTT_LOGIN_OK : MQTT_LOGIN_FAILED;
        if (mqtt_status == MQTT_LOGIN_FAILED) {
            ESP_LOGI(TAG, "Not processing file, no MQTT connection");
            return false;
        }
    }

    size_t len = file.available();
    if (len > MAX_G_BUFFER) {
        ESP_LOGE(TAG, "Message too long");

        // This signals the message file can be deleted. If it is too long to send
        // there is no point trying again later.
        return true;
    }

    // g_buffer is longer than len, so it is safe to read all of len bytes and then zero-terminate.
    file.readBytes(g_buffer, len);
    g_buffer[len] = 0;

    return mqtt_publish(topic, g_buffer);
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
        File file = root.openNextFile();
        while (file) {
            bool remove_file = false;
            const char *filename = file.name();

            if ( ! file.isDirectory()) {
                if ( ! strncmp(filename, msg_file_prefix, msg_file_prefix_len)) {
                    if (file_count > 0) {
                        delay(250);
                    }
                    remove_file = process_file(file);
                    file_count++;
                    if ( ! remove_file) {
                        upload_errors++;
                    }
                }
            }

            file.close();

            if (remove_file) {
                snprintf(g_buffer, MAX_G_BUFFER, "/%s", filename);
                SPIFFS.remove(g_buffer);
            }

            file = root.openNextFile();
        }

        root.close();

        ESP_LOGI(TAG, "Processed %u files with %u upload errors", file_count, upload_errors);
    }

    if (mqtt_status == MQTT_LOGIN_OK) {
        mqtt_logout();
    }

    mqtt_status = MQTT_UNINITIALISED;
}
