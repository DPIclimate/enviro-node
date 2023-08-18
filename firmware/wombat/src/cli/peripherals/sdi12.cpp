/**
 * @file sdi12.cpp
 *
 * @brief SDI-12 command line interface handler.
 *
 * @date January 2023
 */
#include "cli/peripherals/sdi12.h"
#include "SensorTask.h"
#include "cli/CLI.h"

//! Holds responses from a SDI-12 device
static StreamString response_buffer_;
static char response[CLISdi12::MAX_SDI12_RES_LEN];

//! Enviro-DIY SDI-12 instance.
extern SDI12 sdi12;
//! DPI12 instance
extern DPIClimate12 dpi12;
//!
extern sensor_list sensors;

#define TAG "cli_sdi12"

/**
 * @brief Command line interface handler for an SDI-12 sensor.
 *
 * This function is a command line interface (CLI) handler for an SDI-12 sensor.
 * It processes commands entered by the user and takes the appropriate action.
 *
 * The function first checks if there is a response stored in the
 * `response_buffer_` object. If so, it copies the response to the
 * `pcWriteBuffer` buffer and returns `pdFALSE` if there are no more responses
 * to be displayed or `pdTRUE` if there are more responses to be displayed in
 * subsequent calls to this function.
 *
 * If there are no responses stored in the `response_buffer_`, the function
 * checks the command entered by the user and preforms the following commands:
 *
 * scan: Scans the SDI-12 bus for sensors and stores the list of sensors in the
 * `response_buffer_` object.
 * m: reads the value of a sensor at a specified address and stores the result in
 * the `response_buffer_`.
 * ">>": It sends a command to the SDI-12 sensor and stores the response in
 * the `response_buffer_`.
 *
 * @todo `cmd` in this function shadows the declaration in the header file
 *
 * @param pcWriteBuffer A buffer where the function can write a response string
 * to display to the user.
 * @param xWriteBufferLen The length of the pcWriteBuffer buffer.
 * @param pcCommandString A string containing the command entered by the user.
 * @return Returns pdFALSE if there are no more responses to be displayed,
 * pdTRUE if there are more responses to be displayed in subsequent calls to
 * this function.
 */
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
            if (CLI::cliInput == nullptr || CLI::cliOutput == nullptr) {
                strncpy(pcWriteBuffer, "ERROR: input stream not set for SDI-12 passthrough mode\r\n", xWriteBufferLen - 1);
                return pdFALSE;
            }

            CLI::cliOutput->println("Entering SDI-12 passthrough mode, press ctrl-D to exit");
            sdi12.begin();

            char cmd[8];

            int ch;
            while (true) {
                if (CLI::cliInput->available()) {
                    ch = CLI::cliInput->peek();
                    if (ch == 0x04) {
                        break;
                    }

                    memset(cmd, 0, sizeof(cmd));
                    readFromStreamUntil(*CLI::cliInput, '\n', cmd, sizeof(cmd));
                    stripWS(cmd);
                    if (strlen(cmd) > 0) {
                        sdi12.sendCommand(cmd);
                    }
                }

                if (sdi12.available()) {
                    ch = sdi12.read();
                    char cch = ch & 0xFF;
                    if (cch >= ' ') {
                        CLI::cliOutput->write(ch);
                    } else {
//                        CLI::cliOutput->printf(" 0x%X ", cch);
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
