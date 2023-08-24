/**
 * @file config_cli.cpp
 *
 * @brief Command line interface node configuration commands.
 *
 * @date January 2023
 */
#include <freertos/FreeRTOS.h>
#include <Stream.h>
#include <StreamString.h>

#include "cli/FreeRTOS_CLI.h"
#include "cli/device_config/config_cli.h"
#include "CAT_M1.h"
#include "Utils.h"
#include "ota_update.h"
#include "ftp_stack.h"
#include "globals.h"

//! ESP32 debugging output tag
#define TAG "config_cli"

//! Response buffer for configuration commands
static StreamString response_buffer_;


/**
 * @brief Enter command line interface (CLI) mode for the given command string.
 *
 * This function handles the following commands:
 * - `list`: Lists the current configuration settings to the response buffer.
 * - `load`: Loads the configuration from SPIFFS storage and outputs it to
 * the response buffer.
 * - `save`: Saves the current configuration to persistent storage (SPIFFS).
 *
 * If an invalid command or argument is given, an error message is added to the
 * response buffer.
 *
 * @param pcWriteBuffer Pointer to the buffer where the output string is to be
 * written.
 * @param xWriteBufferLen Maximum length of the output string, including the
 * null terminator.
 * @param pcCommandString Pointer to the command string input by the user.
 * @return pdTRUE if there is more data to be returned, pdFALSE otherwise.
 */
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

    memset(pcWriteBuffer, 0, xWriteBufferLen);
    param = FreeRTOS_CLIGetParameter(pcCommandString, paramNum, &paramLen);
    if (param != nullptr && paramLen > 0) {
        if (!strncmp("list", param, paramLen)) {
            response_buffer_.clear();
            config.dumpConfig(response_buffer_);
            response_buffer_.println("\r\nOK");
            return pdTRUE;
        }

        if (!strncmp("load", param, paramLen)) {
            config.load();
            config.dumpConfig(response_buffer_);
            response_buffer_.println("\r\nOK");
            return pdTRUE;
        }

        if (!strncmp("save", param, paramLen)) {
            config.save();
            strncpy(pcWriteBuffer, "Configuration saved\r\nOK\r\n", xWriteBufferLen - 1);
            return pdFALSE;
        }

        if (!strncmp("dto", param, 3)) {
            timeout_active = false;
            strncpy(pcWriteBuffer, "Timeout disabled\r\nOK\r\n", xWriteBufferLen - 1);
            return pdFALSE;
        }

        if (!strncmp("eto", param, 3)) {
            timeout_active = true;
            strncpy(pcWriteBuffer, "Timeout enabled\r\nOK\r\n", xWriteBufferLen - 1);
            return pdFALSE;
        }

        if (!strncmp("ota", param, paramLen)) {
            // If force is true the version number in wombat.sha1 is not checked.
            // Use "config ota 1" to force the update.
            bool force = false;

            paramNum++;
            const char* ch = FreeRTOS_CLIGetParameter(pcCommandString, paramNum, &paramLen);
            if (ch != nullptr && paramLen > 0) {
                force = (*ch == '1');
            }

            bool success = false;
            if (cat_m1.make_ready()) {
                if (connect_to_internet()) {
                    if (ftp_login()) {
                        ota_firmware_info_t ota_ctx;
                        int rc = ota_check_for_update(ota_ctx);
                        // force is only useful if we received the wombat.sha1 file, so check that first.
                        if (rc >= 0) {
                            // Do the update if the server has later firmware or force is true.
                            if (rc > 0 || force) {
                                if (rc == 0) {
                                    // If rc == 0 then we must be in here because force is true.
                                    ESP_LOGI(TAG, "Forcing OTA update");
                                }
                                success = ota_download_update(ota_ctx);
                            }
                        }

                        ftp_logout();
                    }
                }
            }

            if (success) {
                strncpy(pcWriteBuffer, "\r\nOK\r\n", xWriteBufferLen - 1);
            } else {
                strncpy(pcWriteBuffer, "\r\nERROR\r\n", xWriteBufferLen - 1);
            }
            return pdFALSE;
        }

        if (!strncmp("sdi12defn", param, paramLen)) {
            bool success = false;
            if (cat_m1.make_ready()) {
                if (connect_to_internet()) {
                    if (ftp_login()) {
                        success = ota_download_sdi12defn();
                        ftp_logout();
                    }
                }
            }

            if (success) {
                strncpy(pcWriteBuffer, "\r\nOK\r\n", xWriteBufferLen - 1);
            } else {
                strncpy(pcWriteBuffer, "\r\nERROR\r\n", xWriteBufferLen - 1);
            }
            return pdFALSE;
        }

        if (!strncmp("reboot", param, paramLen)) {
            strncpy(pcWriteBuffer, "Rebooting\r\nOK\r\n", xWriteBufferLen - 1);
            shutdown();
            esp_restart();
        }

        response_buffer_.clear();
//        response_buffer_.printf("ERROR: Invalid argument: [%s]", param);
        return pdTRUE;
    }

    strncpy(pcWriteBuffer, "ERROR: Invalid command\r\n", xWriteBufferLen - 1);
    return pdFALSE;
}
