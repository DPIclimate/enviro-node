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
#include "cli/device_config/mqtt_cli.h"

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
 * - `port`: MQTT broker port.
 * - `user`: MQTT broker username.
 * - `password`: MQTT broker password.
 *
 * @note Intervals are specified in seconds.
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
                strncpy(pcWriteBuffer, "OK\r\n", xWriteBufferLen - 1);
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
                    strncpy(pcWriteBuffer, "OK\r\n", xWriteBufferLen - 1);
                } else {
                    strncpy(pcWriteBuffer, "ERROR: set MQTT port failed\r\n", xWriteBufferLen - 1);
                }
            }
            return pdFALSE;
        }
    }

    if (!strncmp("user", param, paramLen)) {
        paramNum++;
        param = FreeRTOS_CLIGetParameter(pcCommandString, paramNum, &paramLen);
        if (param != nullptr && paramLen > 0) {
            strncpy(pcWriteBuffer, param, paramLen);
            pcWriteBuffer[paramLen] = 0;
            config.setMqttUser(pcWriteBuffer);
            strncpy(pcWriteBuffer, "OK\r\n", xWriteBufferLen - 1);
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
            strncpy(pcWriteBuffer, "OK\r\n", xWriteBufferLen - 1);
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
            strncpy(config.mqtt_topic_template, param, paramLen);
            strncpy(pcWriteBuffer, "OK\r\n", xWriteBufferLen - 1);
            return pdFALSE;
        }

        memset(pcWriteBuffer, 0, xWriteBufferLen);
        strncpy(pcWriteBuffer, "ERROR: Missing MQTT user name\r\n", xWriteBufferLen - 1);
        return pdFALSE;
    }

    strncpy(pcWriteBuffer, "ERROR: Invalid command\r\n", xWriteBufferLen - 1);
    return pdFALSE;
}
