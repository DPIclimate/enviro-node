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

static char msg_buf[4096 + 1];

/**
 * Sends the given file via MQTT.
 *
 * @param filename the file to send, including the leading '/' required by SPIFFS.
 *
 * @return true if the message was sent ok, otherwise false.
 */
static bool process_file(const String& filename) {
    ESP_LOGI(TAG, "Processing message file [%s]", filename.c_str());
    log_to_sdcardf("Processing message file [%s]", filename.c_str());

    size_t msg_len;
    if (read_spiffs_file(filename.c_str(), msg_buf, sizeof(msg_buf), msg_len)) {
        return false;
    }

    if (mqtt_status == MQTT_UNINITIALISED) {
        if ( ! connect_to_internet()) {
            ESP_LOGE(TAG, "cti failed, not processing file");
            log_to_sdcard("[E] cti failed, not processing file");
            // Set mqtt_status to login failed so no more files will be attempted this run.
            mqtt_status = MQTT_LOGIN_FAILED;
            return false;
        }

        mqtt_status = mqtt_login() ? MQTT_LOGIN_OK : MQTT_LOGIN_FAILED;
        if (mqtt_status == MQTT_LOGIN_FAILED) {
            ESP_LOGE(TAG, "Not processing file, no MQTT connection");
            log_to_sdcard("[E] Not processing file, no MQTT connection");
            return false;
        }
    }

    // This is not always true - if this function is called after a failed login then
    // we want to skip publishing the message.
    if (mqtt_status == MQTT_LOGIN_OK) {
        if (msg_len < MAX_MQTT_DIRECT_MSG_LEN) {
            if (mqtt_publish(topic, msg_buf, msg_len)) {
                SPIFFS.remove(filename);
                return true;
            }
        } else {
            const String r5_fn("a.txt");

            r5.deleteFile(r5_fn);
            delay(500);
            r5.appendFileContents(r5_fn, msg_buf, msg_len);

            const auto a = std::string(msg_buf, msg_len);

            memset(msg_buf, 0, sizeof(msg_buf));
            delay(500);

            size_t bytes_read;
            SARA_R5_error_t r5_err;
            int x = read_r5_file(r5_fn, msg_buf, msg_len, bytes_read, r5_err);
            if (x == -1) {
                return false;
            }

            if (x == -3 || bytes_read != msg_len) {
                return false;
            }

            const char* const a_ptr = a.c_str();
            bool file_corrupt = false;
            for (size_t i = 0; i < msg_len; i++) {
                if (a_ptr[i] != msg_buf[i]) {
                    ESP_LOGE(TAG, "Mismatch at posn %lu, %c != %c", i, a_ptr[i], msg_buf[i]);
                    file_corrupt = true;
                    break;
                }
            }

            if (file_corrupt) {
                return false;
            }

            if ( ! mqtt_publish_file(topic, r5_fn)) {
                return false;
            }

            SPIFFS.remove(filename);
            return true;
        }
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
                log_to_sdcard("MQTT login failed, skipping any further messages");
                break;
            }

            filename = root.getNextFileName();
        }

        root.close();

        ESP_LOGI(TAG, "Processed %u files with %u upload errors", file_count, upload_errors);
    }

    if (mqtt_status == MQTT_UNINITIALISED) {
        ESP_LOGI(TAG, "No files caused an MQTT connection, trying now to look for waiting config scripts");
        log_to_sdcard("No files caused an MQTT connection, trying now to look for waiting config scripts");
        connect_to_internet();
        mqtt_status = mqtt_login() ? MQTT_LOGIN_OK : MQTT_LOGIN_FAILED;
        if (mqtt_status == MQTT_LOGIN_FAILED) {
            ESP_LOGE(TAG, "MQTT connection failed");
            log_to_sdcard("MQTT connection failed");
        }
    }

    if (mqtt_status == MQTT_LOGIN_OK) {
        mqtt_logout();
    }

    mqtt_status = MQTT_UNINITIALISED;
}
