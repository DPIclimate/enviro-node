/**
 * @file cli_power.cpp
 *
 * @brief Power related commands.
 */
#include "cli/peripherals/sd_card.h"

#include <freertos/FreeRTOS.h>
#include <Stream.h>

#include "cli/FreeRTOS_CLI.h"

#include "globals.h"
#include "sd-card/interface.h"
#include "cli/CLI.h"

//! ESP32 debug output tag
#define TAG "cli_sd"

/**
 * @brief Command-line interface command for working with the power buses.
 *
 *
 * @note Intervals are specified in seconds.
 *
 * @param pcWriteBuffer The buffer to write the command's output to.
 * @param xWriteBufferLen The length of the write buffer.
 * @param pcCommandString The command string to be parsed.
 * @return pdTRUE if there are more responses to come, pdFALSE if this is the final response.
 */
BaseType_t CLISDCard::enter_cli(char *pcWriteBuffer, size_t xWriteBufferLen,
                               const char *pcCommandString) {

    if (!SDCardInterface::is_ready()) {
        snprintf(pcWriteBuffer, xWriteBufferLen - 1, "ERROR: SD card not found\r\n");
        return pdFALSE;
    }

    BaseType_t paramLen = 0;
    UBaseType_t paramNum = 1;
    const char *param;

    static unsigned int step = 0;

    memset(pcWriteBuffer, 0, xWriteBufferLen);
    param = FreeRTOS_CLIGetParameter(pcCommandString, paramNum, &paramLen);
    if (param != nullptr && paramLen > 0) {
        if (!strncmp("rm", param, paramLen)) {
            const char *filename = FreeRTOS_CLIGetParameter(pcCommandString, 2, &paramLen);
            if (filename != nullptr && paramLen > 0) {
                if (paramLen < (xWriteBufferLen - 1)) {
                    snprintf(pcWriteBuffer, xWriteBufferLen, "/%s", filename);
                    SDCardInterface::delete_file(pcWriteBuffer);
                    snprintf(pcWriteBuffer, xWriteBufferLen-1, OK_RESPONSE);
                    return pdFALSE;
                }

                snprintf(pcWriteBuffer, xWriteBufferLen-1, "\r\nERROR: filename too long\r\n");
                return pdFALSE;
            }

            snprintf(pcWriteBuffer, xWriteBufferLen-1, "\r\nERROR: missing filename\r\n");
            return pdFALSE;
        }

        if (!strncmp("data", param, paramLen)) {
            if (step < 1) {
                step++;
                snprintf(pcWriteBuffer, xWriteBufferLen - 1, "[\r\n");
                return pdTRUE;
            } else if (step < 2) {
                step++;
                SDCardInterface::read_file(sd_card_datafile_name, *CLI::cliOutput);
                return pdTRUE;
            } else {
                step = 0;
                snprintf(pcWriteBuffer, xWriteBufferLen - 1, "]\r\n");
                return pdFALSE;
            }
        }

        if (!strncmp("log", param, paramLen)) {
            SDCardInterface::read_file(sd_card_logfile_name, *CLI::cliOutput);
            return pdFALSE;
        }
    }

    strncpy(pcWriteBuffer, INVALID_CMD_RESPONSE, xWriteBufferLen - 1);
    return pdFALSE;
}
