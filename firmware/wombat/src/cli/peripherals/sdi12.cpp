#include "cli/peripherals/sdi12.h"
#include "SensorTask.h"

static StreamString response_buffer_;
static char response[CLISdi12::MAX_SDI12_RES_LEN];
extern Stream *cliStream;

extern SDI12 sdi12;
extern DPIClimate12 dpi12;
extern sensor_list sensors;

#define TAG "cli_sdi12"

BaseType_t CLISdi12::enter_cli(char *pcWriteBuffer, size_t xWriteBufferLen,
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

        size_t len = response_buffer_.readBytesUntil('\n', pcWriteBuffer, xWriteBufferLen-1);
        // readBytesUntil strips the delimiter, so put the '\n' back in.
        // So this assumes all responses end in \n.
        if (len < xWriteBufferLen) {
            pcWriteBuffer[len] = '\n';
        }

        return response_buffer_.available() > 0 ? pdTRUE : pdFALSE;
    }

    memset(pcWriteBuffer, 0, xWriteBufferLen);
    param = FreeRTOS_CLIGetParameter(pcCommandString, paramNum, &paramLen);
    if (param != nullptr && paramLen > 0) {
        if (!strncmp("scan", param, paramLen)) {
            sdi12.begin();

            dpi12.scan_bus(sensors);
            for (size_t i = 0; i < sensors.count; i++) {
                response_buffer_.print((char*)&sensors.sensors[i]);
                response_buffer_.print("\r\n");
            }

            response_buffer_.print("OK\r\n");

            sdi12.end();
            return pdTRUE;
        }

        if (!strncmp("m", param, paramLen)) {
            enable12V();
            sdi12.begin();

            char addr = 0;
            paramNum++;
            param = FreeRTOS_CLIGetParameter(pcCommandString, paramNum, &paramLen);
            if (param == nullptr || *param == 0) {
                strncpy(pcWriteBuffer, "ERROR: missing SDI-12 address\r\n", xWriteBufferLen - 1);
                return pdFALSE;
            }

            addr = *param;

            DynamicJsonDocument msg(2048);
            JsonArray timeseries_array = msg.createNestedArray("timeseries");
            if ( ! read_sensor(addr, timeseries_array)) {
                strncpy(pcWriteBuffer, "ERROR: failed to read SDI-12 sensor\r\n", xWriteBufferLen - 1);
                return pdFALSE;
            }

            String str;
            serializeJson(msg, response_buffer_);
            response_buffer_.print("\r\nOK\r\n");

            sdi12.end();

            // SDI-12 power not disabled here. It is disabled when the CLI is exited.

            return pdTRUE;
        }

        if (!strncmp(">>", param, paramLen)) {
            const char* sdi12_cmd = FreeRTOS_CLIGetParameter(pcCommandString, 2, &paramLen);
            if(sdi12_cmd != nullptr && paramLen > 0) {
                sdi12.begin();

                ESP_LOGI(TAG, "SDI-12 CMD received: [%s]", sdi12_cmd);

                sdi12.clearBuffer();
                response_buffer_.clear();
                sdi12.sendCommand(sdi12_cmd);

                int ch = -1;
                if (waitForChar(sdi12, 750) != -1) {
                    while (true) {
                        while (sdi12.available()) {
                            ch = sdi12.read();
                            response_buffer_.write(ch);
                        }

                        if (ch == '\n' || waitForChar(sdi12, 50) < 0) {
                            break;
                        }
                    }
                }

                delay(10);
                sdi12.end();

                bool ok = ! response_buffer_.isEmpty();
                if (ch != '\n') {
                    response_buffer_.print("\r\n");
                }

                if (ok) {
                    response_buffer_.print("OK\r\n");
                } else {
                    response_buffer_.print("ERROR: No response\r\n");
                }

                return pdTRUE;
            }
        }

        if (!strncmp("pt", param, paramLen)) {
            if (cliStream == nullptr) {
                strncpy(pcWriteBuffer, "ERROR: input stream not set for SDI-12 passthrough mode\r\n", xWriteBufferLen - 1);
                return pdFALSE;
            }

            cliStream->println("Entering SDI-12 passthrough mode, press ctrl-D to exit");
            sdi12.begin();

            char cmd[8];

            int ch;
            while (true) {
                if (cliStream->available()) {
                    ch = cliStream->peek();
                    if (ch == 0x04) {
                        break;
                    }

                    memset(cmd, 0, sizeof(cmd));
                    readFromStreamUntil(*cliStream, '\n', cmd, sizeof(cmd));
                    stripWS(cmd);
                    if (strlen(cmd) > 0) {
                        sdi12.sendCommand(cmd);
                    }
                }

                if (sdi12.available()) {
                    ch = sdi12.read();
                    char cch = ch & 0xFF;
                    if (cch >= ' ') {
                        cliStream->write(ch);
                    } else {
                        cliStream->printf(" 0x%X ", cch);
                    }
                }

                yield();
            }

            sdi12.end();

            strncpy(pcWriteBuffer, "Exited SDI-12 passthrough mode\r\nOK\r\n", xWriteBufferLen - 1);
            return pdFALSE;
        }
    }

    strncpy(pcWriteBuffer, "ERROR\r\n", xWriteBufferLen - 1);
    return pdFALSE;
}
