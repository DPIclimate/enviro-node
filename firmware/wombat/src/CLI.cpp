#include <Arduino.h>
#include "CLI.h"

#include <freertos/FreeRTOS.h>
#include "FreeRTOS_CLI.h"
#include "Config.h"

#include <esp_log.h>
#include <StreamString.h>

#define TAG "cli"

static BaseType_t doInterval(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString);

static const CLI_Command_Definition_t intervalCmd = { "interval", "interval:\r\n Configure interval settings\r\n", doInterval, -1 };

void cliInitialise(void) {
    ESP_LOGI(TAG, "Registering CLI commands");
    FreeRTOS_CLIRegisterCommand(&intervalCmd);
}

// Commands returning multiple lines should write their output to this, and then
// call returnNextLine each time they are called and responseBuffer.available() > 0.
static StreamString responseBuffer;

static BaseType_t returnNextLine(char *pcWriteBuffer, size_t xWriteBufferLen) {
    if (responseBuffer.available()) {
        memset(pcWriteBuffer, 0, xWriteBufferLen);
        size_t len = responseBuffer.readBytesUntil('\n', pcWriteBuffer, xWriteBufferLen-1);

        // readBytesUntil strips the delimiter, so put the '\n' back in.
        if (len <= xWriteBufferLen) {
            pcWriteBuffer[len-1] = '\n';
        }

        return responseBuffer.available() > 0 ? pdTRUE : pdFALSE;
    }

    return pdFALSE;
}

// INTERVAL
//
// The interval command sets and gets the various interval values, such as the measurement interval and the
// uplink interval. Intervals are specified in seconds.
BaseType_t doInterval(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString) {
    static Config& config = Config::get();
    BaseType_t paramLen = 0;
    UBaseType_t paramNum = 1;
    const char* param;

    if (responseBuffer.available()) {
        return returnNextLine(pcWriteBuffer, xWriteBufferLen);
    }

    do {
        param = FreeRTOS_CLIGetParameter(pcCommandString, paramNum, &paramLen);
        if (param != nullptr && paramLen > 0) {
            //memset(pcWriteBuffer, 0, xWriteBufferLen);
            strncpy(pcWriteBuffer, param, paramLen);
            if (!strncmp("list", pcWriteBuffer, sizeof("list"))) {
                responseBuffer.clear();
                responseBuffer.print("interval measure ");
                responseBuffer.println(config.getMeasureInterval());
                responseBuffer.print("interval uplink ");
                responseBuffer.println(config.getUplinkInterval());
                memset(pcWriteBuffer, 0, xWriteBufferLen);
                return pdTRUE;
            }

            if (!strncmp("measure", pcWriteBuffer, sizeof("measure"))) {
                responseBuffer.clear();

                uint16_t i = 0;
                paramNum++;
                param = FreeRTOS_CLIGetParameter(pcCommandString, paramNum, &paramLen);
                if (param != nullptr && paramLen > 0) {
                    strncpy(pcWriteBuffer, param, paramLen);
                    i = strtoul(pcWriteBuffer, nullptr, 10);
                }

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

            if (!strncmp("uplink", pcWriteBuffer, sizeof("uplink"))) {
                responseBuffer.clear();

                uint16_t i = 0;
                paramNum++;
                param = FreeRTOS_CLIGetParameter(pcCommandString, paramNum, &paramLen);
                if (param != nullptr && paramLen > 0) {
                    strncpy(pcWriteBuffer, param, paramLen);
                    i = strtoul(pcWriteBuffer, nullptr, 10);
                }

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

        paramNum++;
    } while (param != nullptr);

    return pdFALSE;
}
