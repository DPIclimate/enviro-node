#include <freertos/FreeRTOS.h>
#include <Stream.h>
#include <StreamString.h>

#include "cli/FreeRTOS_CLI.h"
#include "cli/device_config/config_cli.h"

#define TAG "config_cli"

static StreamString response_buffer_;

// The interval command sets and gets the various interval values, such as the measurement interval and the
// uplink interval. Intervals are specified in seconds.
BaseType_t CLIConfig::enter_cli(char *pcWriteBuffer, size_t xWriteBufferLen,
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

    DeviceConfig& config = DeviceConfig::get();

    memset(pcWriteBuffer, 0, xWriteBufferLen);
    param = FreeRTOS_CLIGetParameter(pcCommandString, paramNum, &paramLen);
    if (param != nullptr && paramLen > 0) {
        if (!strncmp("list", param, paramLen)) {
            response_buffer_.clear();
            config.dumpConfig(response_buffer_);
            return pdTRUE;
        }
    }

    if (param != nullptr && paramLen > 0) {
        if (!strncmp("load", param, paramLen)) {
            config.load();
            config.dumpConfig(response_buffer_);
            return pdTRUE;
        }
    }

    if (param != nullptr && paramLen > 0) {
        if (!strncmp("save", param, paramLen)) {
            config.save();
            strncpy(pcWriteBuffer, "Configuration saved\r\n", xWriteBufferLen - 1);
            return pdFALSE;
        }
    }

    strncpy(pcWriteBuffer, "ERROR: Invalid command\r\n", xWriteBufferLen - 1);
    return pdFALSE;
}
