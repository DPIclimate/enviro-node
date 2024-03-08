/**
 * @file cli_spiffs.cpp
 *
 * @brief SPIFFS related commands.
 */
#include <SPIFFS.h>

#include "cli/peripherals/sd_card.h"

#include <freertos/FreeRTOS.h>
#include <Stream.h>

#include "cli/FreeRTOS_CLI.h"

#include "globals.h"
#include "sd-card/interface.h"
#include "cli/CLI.h"
#include "cli/peripherals/cli_spiffs.h"
#include "Utils.h"

//! ESP32 debug output tag
#define TAG "cli_spiffs"

static StreamString response_buffer_;

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
BaseType_t CLISPIFFS::enter_cli(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString) {
    if ( ! spiffs_ok) {
        snprintf(pcWriteBuffer, xWriteBufferLen - 1, "ERROR: SPIFFS not initialised\r\n");
        return pdFALSE;
    }

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
        if (len <= xWriteBufferLen) {
            pcWriteBuffer[len-1] = '\n';
        }

        return response_buffer_.available() > 0 ? pdTRUE : pdFALSE;
    }

    BaseType_t paramLen = 0;
    UBaseType_t paramNum = 1;
    const char *param;

    static uint8_t CRLF[] = { 0x0d, 0x0a };

    memset(pcWriteBuffer, 0, xWriteBufferLen);
    param = FreeRTOS_CLIGetParameter(pcCommandString, paramNum, &paramLen);
    if (param != nullptr && paramLen > 0) {
        if (!strncmp("ls", param, paramLen)) {
            File root = SPIFFS.open("/");
            if ( ! root) {
                snprintf(pcWriteBuffer, xWriteBufferLen - 1, "ERROR: Failed to open root directory of SPIFFS\r\n");
                return pdFALSE;
            }

            String filename = root.getNextFileName();
            while (filename.length() > 0) {
                response_buffer_.write(reinterpret_cast<const uint8_t *>(filename.c_str()), filename.length());
                filename = root.getNextFileName();
                response_buffer_.write(CRLF, sizeof(CRLF));
            }

            root.close();
            return pdTRUE;
        }

        if (!strncmp("cat", param, paramLen)) {
            paramNum++;
            const char *filename = FreeRTOS_CLIGetParameter(pcCommandString, paramNum, &paramLen);
            if (filename != nullptr && paramLen > 0) {
                memset(g_buffer, 0, sizeof(g_buffer));
                size_t max_len = MAX_G_BUFFER;
                if (*filename != '/') {
                    g_buffer[0] = '/';
                    max_len--;
                }

                // filename is pointing to a character in pcCommandString but the argument may not be
                // null terminated if there is another parameter so only copy paramLen characters.
                if (paramLen < max_len) {
                    max_len = paramLen;
                }

                strncat(g_buffer, filename, max_len);

                File file = SPIFFS.open(g_buffer);
                // Just in case a directory shows up with a match on the filename pattern. May as well say it
                // was processed ok.
                if (file.isDirectory()) {
                    file.close();
                    snprintf(pcWriteBuffer, xWriteBufferLen - 1, "ERROR: %s is a directory\r\n", g_buffer);
                    return pdFALSE;
                }

                size_t len = file.available();
                if (len > MAX_G_BUFFER) {
                    file.close();
                    snprintf(pcWriteBuffer, xWriteBufferLen - 1, "ERROR: %s too long to print\r\n", g_buffer);
                    return pdFALSE;
                }

                memset(g_buffer, 0, sizeof(g_buffer));
                file.readBytes(g_buffer, len);
                file.close();

                response_buffer_.write(reinterpret_cast<const uint8_t *>(g_buffer), len);
                return pdTRUE;
            }

            snprintf(pcWriteBuffer, xWriteBufferLen - 1, "ERROR: filename required\r\n");
            return pdFALSE;
        }

        if (!strncmp("cp", param, paramLen)) {
            paramNum++;
            const char *filename = FreeRTOS_CLIGetParameter(pcCommandString, paramNum, &paramLen);
            if (filename != nullptr && paramLen > 0) {
                memset(g_buffer, 0, sizeof(g_buffer));
                size_t max_len = MAX_G_BUFFER;
                if (*filename != '/') {
                    g_buffer[0] = '/';
                    max_len--;
                }

                strncat(g_buffer, filename, max_len);

                paramNum++;
                const char *dest = FreeRTOS_CLIGetParameter(pcCommandString, paramNum, &paramLen);
                if (dest != nullptr && paramLen > 0) {
                    int fcd;
                    const char *dest_filename;
                    size_t dest_filename_len = 0;

                    if ( ! wombat::get_cp_destination(dest, paramLen, fcd, &dest_filename, dest_filename_len)) {
                        snprintf(pcWriteBuffer, xWriteBufferLen - 1, "ERROR: dest:filename required\r\n");
                        return pdFALSE;
                    }

                    if (fcd == 1 && *dest_filename == '/') {
                        dest_filename++;
                    }


                }
            }
        }

        if (!strncmp("rm", param, paramLen)) {
            paramNum++;
            const char *filename = FreeRTOS_CLIGetParameter(pcCommandString, paramNum, &paramLen);
            if (filename != nullptr && paramLen > 0) {
                if (paramLen < (xWriteBufferLen - 1)) {
                    snprintf(pcWriteBuffer, xWriteBufferLen, "/%s", filename);
                    //SDCardInterface::delete_file(pcWriteBuffer);
                    snprintf(pcWriteBuffer, xWriteBufferLen-1, OK_RESPONSE);
                    return pdFALSE;
                }

                snprintf(pcWriteBuffer, xWriteBufferLen-1, "\r\nERROR: filename too long\r\n");
                return pdFALSE;
            }

            snprintf(pcWriteBuffer, xWriteBufferLen-1, "\r\nERROR: missing filename\r\n");
            return pdFALSE;
        }
    }

    strncpy(pcWriteBuffer, INVALID_CMD_RESPONSE, xWriteBufferLen - 1);
    return pdFALSE;
}
