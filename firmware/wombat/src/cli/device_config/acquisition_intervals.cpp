#include "cli/device_config/acquisition_intervals.h"

static StreamString response_buffer_;

void CLIConfigIntervals::dump(Stream& stream) {
    stream.print("interval measure ");
    stream.println(config.getMeasureInterval());
    stream.print("interval uplink ");
    stream.println(config.getUplinkInterval());
}

// The interval command sets and gets the various interval values, such as the measurement interval and the
// uplink interval. Intervals are specified in seconds.
BaseType_t CLIConfigIntervals::enter_cli(char *pcWriteBuffer, size_t xWriteBufferLen,
                     const char *pcCommandString) {
    BaseType_t paramLen = 0;
    UBaseType_t paramNum = 1;
    const char *param;

    // More in the buffer?
    if (response_buffer_.available()) {
        memset(pcWriteBuffer, 0, xWriteBufferLen);
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

        if (!strncmp("measure", param, paramLen)) {
            uint16_t i = 0;
            paramNum++;
            param = FreeRTOS_CLIGetParameter(pcCommandString, paramNum, &paramLen);
            if (param != nullptr && paramLen > 0) {
                strncpy(pcWriteBuffer, param, paramLen);
                i = strtoul(pcWriteBuffer, nullptr, 10);
            }

            memset(pcWriteBuffer, 0, xWriteBufferLen);
            if (i < 1) {
                strncpy(pcWriteBuffer, "ERROR: Missing or invalid interval value\r\n", xWriteBufferLen - 1);
                return pdFALSE;
            }

            config.setMeasureInterval(i);
            if (i == config.getMeasureInterval()) {
                strncpy(pcWriteBuffer, "OK\r\n", xWriteBufferLen - 1);
            } else {
                strncpy(pcWriteBuffer, "ERROR: set measure interval failed\r\n", xWriteBufferLen - 1);
            }
            return pdFALSE;
        }

        if (!strncmp("uplink", param, paramLen)) {
            uint16_t i = 0;
            paramNum++;
            param = FreeRTOS_CLIGetParameter(pcCommandString, paramNum, &paramLen);
            if (param != nullptr && paramLen > 0) {
                strncpy(pcWriteBuffer, param, paramLen);
                i = strtoul(pcWriteBuffer, nullptr, 10);
            }

            memset(pcWriteBuffer, 0, xWriteBufferLen);
            if (i < 1) {
                strncpy(pcWriteBuffer, "ERROR: Missing or invalid interval value\r\n", xWriteBufferLen - 1);
                return pdFALSE;
            }

            config.setUplinkInterval(i);
            if (i == config.getUplinkInterval()) {
                strncpy(pcWriteBuffer, "OK\r\n", xWriteBufferLen - 1);
            } else {
                strncpy(pcWriteBuffer, "ERROR: set uplink interval failed\r\n", xWriteBufferLen - 1);
            }
            return pdFALSE;
        }
    }

    strncpy(pcWriteBuffer, "Syntax error\r\n", xWriteBufferLen - 1);
    return pdFALSE;
}
