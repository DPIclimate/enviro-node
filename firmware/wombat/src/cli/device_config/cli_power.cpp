/**
 * @file cli_power.cpp
 *
 * @brief Power related commands.
 */
#include "cli/peripherals/cli_power.h"

#include <freertos/FreeRTOS.h>
#include <Stream.h>
#include <StreamString.h>

#include "cli/FreeRTOS_CLI.h"

#include "globals.h"

//! ESP32 debug output tag
#define TAG "cli_power"

//! MQTT CLI response buffer for output to the user
static StreamString response_buffer_;

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
BaseType_t CLIPower::enter_cli(char *pcWriteBuffer, size_t xWriteBufferLen,
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
        if (!strncmp("show", param, paramLen)) {
            response_buffer_.clear();
            float bv = battery_monitor.get_voltage();
            float bi = battery_monitor.get_current();

            float sv = solar_monitor.get_voltage();
            float si = solar_monitor.get_current();

            snprintf(pcWriteBuffer, xWriteBufferLen - 1, "Battery: %.2fv %.2fA, solar: %.2fv %.2fA\r\n", bv, bi, sv, si);
            return pdFALSE;
        }
    }

    strncpy(pcWriteBuffer, "ERROR: Invalid command\r\n", xWriteBufferLen - 1);
    return pdFALSE;
}