/**
 * @file mqtt_cli.cpp
 *
 * @brief MQTT setup and configuration through the CLI.
 *
 * @date January 2023
 */
#include <freertos/FreeRTOS.h>
#include <Stream.h>
#include <StreamString.h>

#include "cli/FreeRTOS_CLI.h"
#include "cli/CLI.h"
#include "cli/device_config/mqtt_cli.h"

#include <FS.h>
#include <Utils.h>

#include "mqtt_stack.h"
#include "globals.h"

//! ESP32 debug output tag
#define TAG "mqtt_cli"

//! MQTT CLI response buffer for output to the user
static StreamString response_buffer_;

/**
 * @brief Display current MQTT configuration.
 *
 * @param stream Output stream.
 */
void CLIMQTT::dump(Stream& stream) {
    stream.print("mqtt host ");
    stream.println(config.getMqttHost().c_str());
    stream.print("mqtt port ");
    stream.println(config.getMqttPort());
    stream.print("mqtt user ");
    stream.println(config.getMqttUser().c_str());
    stream.print("mqtt password ");
    stream.println(config.getMqttPassword().c_str());
    stream.print("mqtt topic ");
    stream.println(config.mqtt_topic_template);
}

/**
 * @brief Command-line interface command for setting MQTT parameters.
 *
 * This function provides a command-line interface for setting MQTT connection
 * parameters such as the host, port, user, and password. Commands include:
 *
 * - `list`: List MQTT configuration.
 * - `host`: MQTT broker host.
 * - `port`: MQTT broker port.
 * - `user`: MQTT broker username.
 * - `password`: MQTT broker password.
 *
 * @param pcWriteBuffer The buffer to write the command's output to.
 * @param xWriteBufferLen The length of the write buffer.
 * @param pcCommandString The command string to be parsed.
 * @return pdTRUE if there are more responses to come, pdFALSE if this is the final response.
 */
BaseType_t CLIMQTT::enter_cli(char *pcWriteBuffer, size_t xWriteBufferLen,
                                         const char *pcCommandString) {
    BaseType_t paramLen = 0;
    UBaseType_t paramNum = 1;
    const char *param;

    // More in the buffer?
    if (response_buffer_.available()) {
        memset(pcWriteBuffer, 0, xWriteBufferLen);
        if (response_buffer_.length() < xWriteBufferLen) {
            strncpy(pcWriteBuffer, response_buffer_.c_str(), response_buffer_.length());
            response_buffer_.clear();
            return pdFALSE;
        }

        size_t len = response_buffer_.readBytesUntil('\n', pcWriteBuffer, xWriteBufferLen - 1);

        // readBytesUntil strips the delimiter, so put the '\n' back in.
        if (len <= xWriteBufferLen) {
            pcWriteBuffer[len - 1] = '\n';
        }

        return response_buffer_.available() > 0 ? pdTRUE : pdFALSE;
    }

    memset(pcWriteBuffer, 0, xWriteBufferLen);
    param = FreeRTOS_CLIGetParameter(pcCommandString, paramNum, &paramLen);
    if (param != nullptr && paramLen > 0) {
        if (!strncmp("list", param, paramLen)) {
            response_buffer_.clear();
            dump(response_buffer_);
            return pdTRUE;
        }

        if (!strncmp("host", param, paramLen)) {
            paramNum++;
            param = FreeRTOS_CLIGetParameter(pcCommandString, paramNum, &paramLen);
            if (param != nullptr && paramLen > 0) {
                strncpy(pcWriteBuffer, param, paramLen);
                pcWriteBuffer[paramLen] = 0;
                config.setMqttHost(pcWriteBuffer);
                strncpy(pcWriteBuffer, OK_RESPONSE, xWriteBufferLen - 1);
                return pdFALSE;
            }

            memset(pcWriteBuffer, 0, xWriteBufferLen);
            strncpy(pcWriteBuffer, "ERROR: Missing MQTT host name\r\n", xWriteBufferLen - 1);
            return pdFALSE;
        }

        if (!strncmp("port", param, paramLen)) {
            uint16_t i = 0;
            paramNum++;
            param = FreeRTOS_CLIGetParameter(pcCommandString, paramNum, &paramLen);
            // Checking if the first character of param is '-' because strtoul will convert
            // -1 to 65565, for example.
            if (param != nullptr && paramLen > 0 && param[0] != '-') {
                strncpy(pcWriteBuffer, param, paramLen);
                i = strtoul(pcWriteBuffer, nullptr, 10);
            }

            memset(pcWriteBuffer, 0, xWriteBufferLen);
            if (i < 1) {
                strncpy(pcWriteBuffer, "ERROR: Missing or invalid MQTT port number\r\n", xWriteBufferLen - 1);
            } else {
                config.setMqttPort(i);
                if (i == config.getMqttPort()) {
                    strncpy(pcWriteBuffer, OK_RESPONSE, xWriteBufferLen - 1);
                } else {
                    strncpy(pcWriteBuffer, "ERROR: set MQTT port failed\r\n", xWriteBufferLen - 1);
                }
            }
            return pdFALSE;
        }

        if (!strncmp("user", param, paramLen)) {
            paramNum++;
            param = FreeRTOS_CLIGetParameter(pcCommandString, paramNum, &paramLen);
            if (param != nullptr && paramLen > 0) {
                strncpy(pcWriteBuffer, param, paramLen);
                pcWriteBuffer[paramLen] = 0;
                config.setMqttUser(pcWriteBuffer);
                strncpy(pcWriteBuffer, OK_RESPONSE, xWriteBufferLen - 1);
                return pdFALSE;
            }

            memset(pcWriteBuffer, 0, xWriteBufferLen);
            strncpy(pcWriteBuffer, "ERROR: Missing MQTT user name\r\n", xWriteBufferLen - 1);
            return pdFALSE;
        }

        if (!strncmp("password", param, paramLen)) {
            paramNum++;
            param = FreeRTOS_CLIGetParameter(pcCommandString, paramNum, &paramLen);
            if (param != nullptr && paramLen > 0) {
                strncpy(pcWriteBuffer, param, paramLen);
                pcWriteBuffer[paramLen] = 0;
                config.setMqttPassword(pcWriteBuffer);
                strncpy(pcWriteBuffer, OK_RESPONSE, xWriteBufferLen - 1);
                return pdFALSE;
            }

            memset(pcWriteBuffer, 0, xWriteBufferLen);
            strncpy(pcWriteBuffer, "ERROR: Missing MQTT password\r\n", xWriteBufferLen - 1);
            return pdFALSE;
        }

        if (!strncmp("topic", param, paramLen)) {
            paramNum++;
            param = FreeRTOS_CLIGetParameter(pcCommandString, paramNum, &paramLen);
            if (param != nullptr && paramLen > 0) {
                if (paramLen > DeviceConfig::MAX_CONFIG_STR) {
                    strncpy(pcWriteBuffer, "ERROR: Topic name too long\r\n", xWriteBufferLen - 1);
                    return pdFALSE;
                }

                strncpy(config.mqtt_topic_template, param, paramLen);
                config.mqtt_topic_template[paramLen] = 0;
                strncpy(pcWriteBuffer, OK_RESPONSE, xWriteBufferLen - 1);
                return pdFALSE;
            }

            memset(pcWriteBuffer, 0, xWriteBufferLen);
            strncpy(pcWriteBuffer, "ERROR: Missing MQTT user name\r\n", xWriteBufferLen - 1);
            return pdFALSE;
        }

        if (!strncmp("login", param, paramLen)) {
            bool rc = mqtt_login();
            snprintf(pcWriteBuffer, xWriteBufferLen-1, "%s", rc ? OK_RESPONSE : ERROR_RESPONSE);
            return pdFALSE;
        }

        if (!strncmp("logout", param, paramLen)) {
            bool rc = mqtt_logout();
            snprintf(pcWriteBuffer, xWriteBufferLen-1, "%s", rc ? OK_RESPONSE : ERROR_RESPONSE);
            return pdFALSE;
        }

        if (!strncmp("pubfile", param, paramLen)) {
            const String topic(config.mqtt_topic_template);
            paramNum++;
            param = FreeRTOS_CLIGetParameter(pcCommandString, paramNum, &paramLen);
            bool rc = false;
            if (param != nullptr && paramLen > 0) {
                ESP_LOGI(TAG, "publishing file [%s] to topic [%s]", param, topic.c_str());

                size_t file_size;
                int x = read_spiffs_file(param, g_buffer, MAX_G_BUFFER, file_size);
                if (x == -1) {
                    snprintf(pcWriteBuffer, xWriteBufferLen-1, "\r\nERROR: %s is a directory", param);
                    ESP_LOGE(TAG, "%s", pcWriteBuffer);
                    return pdFALSE;
                }
                if (x == -2) {
                    snprintf(pcWriteBuffer, xWriteBufferLen-1, "\r\nERROR: %s: file too long", param);
                    ESP_LOGE(TAG, "%s", pcWriteBuffer);
                    return pdFALSE;
                }
                if (x == -3) {
                    snprintf(pcWriteBuffer, xWriteBufferLen-1, "\r\nERROR: %s: short read", param);
                    ESP_LOGE(TAG, "%s", pcWriteBuffer);
                    return pdFALSE;
                }

                g_buffer[file_size] = 0;
                ESP_LOGI(TAG, "read: %lu bytes\r\n%s", file_size, g_buffer);

                const String r5_fn("a.txt");

                r5.deleteFile(r5_fn);
                delay(500);
                r5.appendFileContents(r5_fn, g_buffer, file_size);

                const auto a = std::string(g_buffer);

                memset(g_buffer, 0, sizeof(g_buffer));
                delay(1000);

                size_t bytes_read;
                SARA_R5_error_t r5_err;
                x = read_r5_file(r5_fn, g_buffer, file_size, bytes_read, r5_err);
                if (x == -1) {
                    snprintf(pcWriteBuffer, xWriteBufferLen-1, "ERROR: r5.getFileBlock returned %d", r5_err);
                    ESP_LOGE(TAG, "%s", pcWriteBuffer);
                    return pdFALSE;
                }

                if (x == -3 || bytes_read != file_size) {
                    snprintf(pcWriteBuffer, xWriteBufferLen-1, "ERROR: expected %lu bytes, received %lu", file_size, bytes_read);
                    ESP_LOGE(TAG, "%s", pcWriteBuffer);
                    return pdFALSE;
                }

                g_buffer[file_size] = 0;
                ESP_LOGI(TAG, "File from R5:\n\r%s", g_buffer);

                const char* const a_ptr = a.c_str();
                bool file_corrupt = false;
                for (size_t i = 0; i < file_size; i++) {
                    const char c1 = a_ptr[i];
                    const char c2 = g_buffer[i];
                    if (c1 != c2) {
                        ESP_LOGE(TAG, "Mismatch at posn %lu, %c != %c", i, a_ptr[i], g_buffer[i]);
                        file_corrupt = true;
                        break;
                    }
                }

                if (file_corrupt) {
                    snprintf(pcWriteBuffer, xWriteBufferLen-1, "%s", ERROR_RESPONSE);
                    return pdFALSE;
                }

                r5_err = r5.mqttPublishFromFile(topic, r5_fn, 1);
                if (r5_err) {
                    snprintf(pcWriteBuffer, xWriteBufferLen-1, "ERROR: publish failed, error code = %d", r5_err);
                    ESP_LOGE(TAG, "%s", pcWriteBuffer);
                    return pdFALSE;
                }

                rc = true;
            }

            snprintf(pcWriteBuffer, xWriteBufferLen-1, "%s", rc ? OK_RESPONSE : ERROR_RESPONSE);
            return pdFALSE;
        }

        if (!strncmp("publish", param, paramLen)) {
            String topic(config.mqtt_topic_template);
            bool rc = mqtt_publish(topic, "ABCDEF", 6);
            snprintf(pcWriteBuffer, xWriteBufferLen-1, "%s", rc ? OK_RESPONSE : ERROR_RESPONSE);
            return pdFALSE;
        }
    }

    strncpy(pcWriteBuffer, INVALID_CMD_RESPONSE, xWriteBufferLen - 1);
    return pdFALSE;
}
