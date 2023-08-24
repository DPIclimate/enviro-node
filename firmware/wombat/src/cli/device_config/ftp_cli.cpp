/**
 * @file ftp_cli.cpp
 *
 * @brief FTP setup and configuration through the CLI.
 *
 * @date January 2023
 */
#include <freertos/FreeRTOS.h>
#include <Stream.h>
#include <StreamString.h>

#include "cli/FreeRTOS_CLI.h"
#include "cli/device_config/ftp_cli.h"
#include "ftp_stack.h"

//! ESP32 debug output tag
#define TAG "ftp_cli"

//! FTP CLI response buffer for output to the user
static StreamString response_buffer_;

/**
 * @brief Display current FTP configuration.
 *
 * @param stream Output stream.
 */
void CLIFTP::dump(Stream& stream) {
    stream.print("ftp host ");
    stream.println(config.getFtpHost().c_str());
    stream.print("ftp user ");
    stream.println(config.getFtpUser().c_str());
    stream.print("ftp password ");
    stream.println(config.getFtpPassword().c_str());
}

/**
 * @brief Command-line interface command for setting FTP parameters.
 *
 * This function provides a command-line interface for setting FTP connection
 * parameters such as the host, port, user, and password. Commands include:
 *
 * - `list`: List FTP configuration.
 * - `host`: FTP server hostname.
 * - `user`: FTP  username.
 * - `password`: FTP  password.
 *
 * @param pcWriteBuffer The buffer to write the command's output to.
 * @param xWriteBufferLen The length of the write buffer.
 * @param pcCommandString The command string to be parsed.
 * @return pdTRUE if there are more responses to come, pdFALSE if this is the final response.
 */
BaseType_t CLIFTP::enter_cli(char *pcWriteBuffer, size_t xWriteBufferLen,
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
            dump(response_buffer_);
            return pdTRUE;
        }

        if (!strncmp("host", param, paramLen)) {
            paramNum++;
            param = FreeRTOS_CLIGetParameter(pcCommandString, paramNum, &paramLen);
            if (param != nullptr && paramLen > 0) {
                strncpy(pcWriteBuffer, param, paramLen);
                pcWriteBuffer[paramLen] = 0;
                config.setFtpHost(pcWriteBuffer);
                strncpy(pcWriteBuffer, "OK\r\n", xWriteBufferLen - 1);
                return pdFALSE;
            }

            memset(pcWriteBuffer, 0, xWriteBufferLen);
            strncpy(pcWriteBuffer, "ERROR: Missing FTP host name\r\n", xWriteBufferLen - 1);
            return pdFALSE;
        }

        if (!strncmp("user", param, paramLen)) {
            paramNum++;
            param = FreeRTOS_CLIGetParameter(pcCommandString, paramNum, &paramLen);
            if (param != nullptr && paramLen > 0) {
                strncpy(pcWriteBuffer, param, paramLen);
                pcWriteBuffer[paramLen] = 0;
                config.setFtpUser(pcWriteBuffer);
                strncpy(pcWriteBuffer, "OK\r\n", xWriteBufferLen - 1);
                return pdFALSE;
            }

            memset(pcWriteBuffer, 0, xWriteBufferLen);
            strncpy(pcWriteBuffer, "ERROR: Missing FTP user name\r\n", xWriteBufferLen - 1);
            return pdFALSE;
        }

        if (!strncmp("password", param, paramLen)) {
            paramNum++;
            param = FreeRTOS_CLIGetParameter(pcCommandString, paramNum, &paramLen);
            if (param != nullptr && paramLen > 0) {
                strncpy(pcWriteBuffer, param, paramLen);
                pcWriteBuffer[paramLen] = 0;
                config.setFtpPassword(pcWriteBuffer);
                strncpy(pcWriteBuffer, "OK\r\n", xWriteBufferLen - 1);
                return pdFALSE;
            }

            memset(pcWriteBuffer, 0, xWriteBufferLen);
            strncpy(pcWriteBuffer, "ERROR: Missing FTP password\r\n", xWriteBufferLen - 1);
            return pdFALSE;
        }

        if (!strncmp("login", param, paramLen)) {
            bool rc = ftp_login();
            snprintf(pcWriteBuffer, xWriteBufferLen - 1, "\r\n%s\r\n", rc ? "OK" : "ERROR");
            return pdFALSE;
        }

        if (!strncmp("logout", param, paramLen)) {
            bool rc = ftp_logout();
            snprintf(pcWriteBuffer, xWriteBufferLen - 1, "\r\n%s\r\n", rc ? "OK" : "ERROR");
            return pdFALSE;
        }

        if (!strncmp("get", param, paramLen)) {
            paramNum++;
            param = FreeRTOS_CLIGetParameter(pcCommandString, paramNum, &paramLen);
            if (param != nullptr && paramLen > 0) {
                strncpy(pcWriteBuffer, param, paramLen);
                pcWriteBuffer[paramLen] = 0;

                bool rc = ftp_get(pcWriteBuffer);
                snprintf(pcWriteBuffer, xWriteBufferLen - 1, "\r\n%s\r\n", rc ? "OK" : "XERROR");
                return pdFALSE;
            }

            memset(pcWriteBuffer, 0, xWriteBufferLen);
            strncpy(pcWriteBuffer, "ERROR: Missing FTP filename\r\n", xWriteBufferLen - 1);
            return pdFALSE;
        }

        if (!strncmp("upload", param, paramLen)){
            paramNum++;
            param = FreeRTOS_CLIGetParameter(pcCommandString, paramNum, &paramLen);
            if (param != nullptr && paramLen > 0) {
                strncpy(pcWriteBuffer, param, paramLen);
                pcWriteBuffer[paramLen] = 0;

                bool rc = ftp_upload_file(pcWriteBuffer);
                snprintf(pcWriteBuffer, xWriteBufferLen - 1, "\r\n%s\r\n", rc ? "OK" : "ERROR");
                return pdFALSE;
            }

            memset(pcWriteBuffer, 0, xWriteBufferLen);
            strncpy(pcWriteBuffer, "ERROR: Missing filename\r\n", xWriteBufferLen - 1);
            return pdFALSE;
        }

    }
    strncpy(pcWriteBuffer, "ERROR: Invalid command\r\n", xWriteBufferLen - 1);
    return pdFALSE;
}
