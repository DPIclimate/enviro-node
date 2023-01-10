/**
 * @file acquisition_intervals.cpp
 *
 * @brief Handles the time between measurements and uplinks by the device over
 * LTE.
 *
 * @date December 2022
 */
#include "cli/device_config/acquisition_intervals.h"

static StreamString response_buffer;

/**
 * @brief Print out interval information into a provided Stream.
 *
 * A debug method to print out the available information about current
 * acquisition intervals which have either been setup through device memory
 * or through the CLI.
 *
 * @param stream Output stream to write to.
 */
void CLIConfigIntervals::dump(Stream& stream) {
    stream.print("interval measure ");
    stream.println(config.getMeasureInterval());
    stream.print("interval uplink ");
    stream.println(config.getUplinkInterval());
}

/**
 * @brief CLI entrypoint for measurement and uplink interval setup.
 *
 * The function first checks if there is any data left in the response_buffer
 * and, if so, copies it to the write buffer and returns. If no data is left in
 * the response_buffer, the function retrieves the first parameter from the
 * command and checks if it is a "list", "measure", or "uplink" command.
 *
 * If the list command is entered, the function clears the response_buffer and
 * calls the dump() function to print a list of the current measurement and
 * uplink intervals to the buffer.
 *
 * If the measure command is entered, the function retrieves the next parameter
 * from the command string and converts it to an integer. If the parameter is
 * valid, the function sets the measurement interval to the specified value and
 * writes an "OK" message to the write buffer, or an "ERROR" message if the
 * interval was not set.
 *
 * If the uplink command is entered, the function retrieves the next parameter
 * from the command string and converts it to an integer. If the parameter is
 * valid, the function sets the uplink interval to the specified value and
 * writes an "OK" message to the write buffer, or an "ERROR" message if the
 * interval was not set.
 *
 * If none of the above conditions are met, the function writes a "Syntax error"
 * message to the write buffer and returns.
 *
 * @note Intervals are specified in seconds.
 *
 * @param pcWriteBuffer A buffer for storing the response to the command. The
 * response will be displayed to the user.
 * @param xWriteBufferLen The length of the write buffer, in bytes.
 * @param pcCommandString The command entered by the user.
 * @return pdFALSE if the buffer is full and there is more output to be written,
 * otherwise pdTRUE.
 */
BaseType_t CLIConfigIntervals::enter_cli(char *pcWriteBuffer,
                                         size_t xWriteBufferLen,
                                         const char *pcCommandString) {
    BaseType_t paramLen = 0;
    UBaseType_t paramNum = 1;
    const char *param;

    // More in the buffer?
    if (response_buffer.available()) {
        memset(pcWriteBuffer, 0, xWriteBufferLen);
        if (response_buffer.length() < xWriteBufferLen) {
            strncpy(pcWriteBuffer, response_buffer.c_str(),
                    response_buffer.length());
            response_buffer.clear();
            return pdFALSE;
        }

        size_t len = response_buffer.readBytesUntil('\n', pcWriteBuffer,
                                                    xWriteBufferLen - 1);

        // readBytesUntil strips the delimiter, so put the '\n' back in.
        if (len <= xWriteBufferLen) {
            pcWriteBuffer[len - 1] = '\n';
        }

        return response_buffer.available() > 0 ? pdTRUE : pdFALSE;
    }

    memset(pcWriteBuffer, 0, xWriteBufferLen);
    param = FreeRTOS_CLIGetParameter(pcCommandString, paramNum, &paramLen);
    if (param != nullptr && paramLen > 0) {
        if (!strncmp("list", param, paramLen)) {
            response_buffer.clear();
            dump(response_buffer);
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
                strncpy(pcWriteBuffer, "ERROR: Missing or invalid interval "
                                       "value\r\n", xWriteBufferLen - 1);
                return pdFALSE;
            }

            config.setMeasureInterval(i);
            if (i == config.getMeasureInterval()) {
                strncpy(pcWriteBuffer, "OK\r\n", xWriteBufferLen - 1);
            } else {
                strncpy(pcWriteBuffer, "ERROR: set measure interval "
                                       "failed\r\n", xWriteBufferLen - 1);
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
                strncpy(pcWriteBuffer, "ERROR: Missing or invalid interval "
                                       "value\r\n", xWriteBufferLen - 1);
                return pdFALSE;
            }

            config.setUplinkInterval(i);
            if (i == config.getUplinkInterval()) {
                strncpy(pcWriteBuffer, "OK\r\n", xWriteBufferLen - 1);
            } else {
                strncpy(pcWriteBuffer, "ERROR: set uplink interval "
                                       "failed\r\n", xWriteBufferLen - 1);
            }
            return pdFALSE;
        }
    }

    strncpy(pcWriteBuffer, "Syntax error\r\n", xWriteBufferLen - 1);
    return pdFALSE;
}
