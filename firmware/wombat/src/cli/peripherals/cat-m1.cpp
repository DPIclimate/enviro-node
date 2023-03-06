/**
 * @file cat-m1.cpp
 *
 * @brief CAT-M1 CLI command request and response handler.
 *
 * @date January 2023
 */
#include "cli/peripherals/cat-m1.h"
#include "CAT_M1.h"
#include "Utils.h"
#include "SparkFun_u-blox_SARA-R5_Arduino_Library.h"
#include "globals.h"

//! Sparkfun SARA-R5 library instance
extern SARA_R5 r5;
//! Holds responses from the modem
static StreamString response_buffer_;
//! Stream originating from the CLI
extern Stream *cliStream;

/**
 * @brief Get response from modem with a timeout.
 *
 * After a command has been sent to the modem this function handles the
 * response. -1 will be returned if the command times out, else the number of
 * characters in the 'g_buffer' will be returned.
 *
 * @param timeout Wait for response timeout.
 * @return Number of characters in response.
 */
static int get_response(uint32_t timeout = 500) {
    int ch = waitForChar(LTE_Serial, timeout);
    if (ch < 1) {
        return -1;
    }

    int i = 0;
    while (LTE_Serial.available()) {
        g_buffer[i] = LTE_Serial.read();
        i++;
        g_buffer[i] = 0;
    }

    return i;
}

/**
 * @brief Command line interface handler for a Cat M1 device.
 *
 * This function is a command line interface (CLI) handler for a Cat M1 device.
 * It processes commands entered by the user and takes the following actions:
 *
 * - `pt`: Enter Cat M1 passthrough mode. In this mode, all input from the user is
 *     forwarded to the Cat M1 device and all output from the Cat M1 device is
 *     displayed to the user. The mode is exited by pressing ctrl-D.
 * - `pwr`: Set the power state of the Cat M1 device, 1 on, 0 off.
 * - `on`: Turn the Cat M1 device on.
 * - `off`: Turn the Cat M1 device off.
 * - `restart`: Restart the Cat M1 device.
 * - `factory`: Reset the Cat M1 device to factory settings.
 * - `echo`: Enable or disable command echo.
 * - `cme`: Enable or disable the +CME error code feature.
 *
 * @param pcWriteBuffer A buffer where the function can write a response string
 *                      to be displayed to the user.
 * @param xWriteBufferLen The length of the pcWriteBuffer buffer.
 * @param pcCommandString A string containing the command entered by the user.
 * @return pdFALSE if there are no more responses to be displayed, or pdTRUE if
 *         there are more responses to be displayed in subsequent calls to this
 *         function.
 */BaseType_t CLICatM1::enter_cli(char *pcWriteBuffer, size_t xWriteBufferLen,
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
        if (len <= xWriteBufferLen) {
            pcWriteBuffer[len-1] = '\n';
        }

        return response_buffer_.available() > 0 ? pdTRUE : pdFALSE;
    }

    memset(pcWriteBuffer, 0, xWriteBufferLen);
    param = FreeRTOS_CLIGetParameter(pcCommandString, paramNum, &paramLen);
    if (param != nullptr && paramLen > 0) {
        if (!strncmp("pt", param, paramLen)) {
            if (cliStream == nullptr) {
                strncpy(pcWriteBuffer, "ERROR: input stream_ not set for Cat M1 passthrough\r\n", xWriteBufferLen - 1);
                return pdFALSE;
            }

            cliStream->println("Entering Cat M1 passthrough mode, press ctrl-D to exit");

            int ch;
            bool finish = false;
            while ( ! finish) {
                if (cliStream->available()) {
                    while (cliStream->available()) {
                        ch = cliStream->read();
                        if (ch == 0x04) {
                            finish = true;
                            break;
                        }
                        LTE_Serial.write(ch);
                    }
                    taskYIELD();
                }

                if (LTE_Serial.available()) {
                    while (LTE_Serial.available()) {
                        cliStream->write(LTE_Serial.read());
                    }
                }

                taskYIELD();
            }

            strncpy(pcWriteBuffer, "Exited Cat M1 passthrough mode\r\n", xWriteBufferLen - 1);
            return pdFALSE;
        }

        if (!strncmp("pwr", param, paramLen)) {
            const char* pwrState = FreeRTOS_CLIGetParameter(pcCommandString, 2, &paramLen);
            if(pwrState != nullptr && paramLen > 0) {
                cat_m1.power_supply(*pwrState == '1');
            }

            if (*pwrState == '0') {
                if (r5_ok) {
                    r5.modulePowerOff();
                }
            }

            snprintf(pcWriteBuffer, xWriteBufferLen-1, "\r\nOK\r\n");
            return pdFALSE;
        }

        if (!strncmp("on", param, paramLen)) {
            //cat_m1.device_on();
            r5.modulePowerOn();
            snprintf(pcWriteBuffer, xWriteBufferLen-1, "\r\nOK\r\n");
            return pdFALSE;
        }

        if (!strncmp("off", param, paramLen)) {
            //cat_m1.device_off();
            r5.modulePowerOff();
            snprintf(pcWriteBuffer, xWriteBufferLen-1, "\r\nOK\r\n");
            return pdFALSE;
        }

        if (!strncmp("restart", param, paramLen)) {
            cat_m1.restart();
            snprintf(pcWriteBuffer, xWriteBufferLen-1, "\r\nOK\r\n");
            return pdFALSE;
        }

        if (!strncmp("factory", param, paramLen)) {
            LTE_Serial.print("AT+CFUN=0\r");
            int len = get_response();
            if (len > 0) {
                response_buffer_.print(g_buffer);
            } else {
                response_buffer_.print("+CFUN=0 bad response");
            }
            LTE_Serial.print("AT+UFACTORY=2,2\r");
            len = get_response();
            if (len > 0) {
                response_buffer_.print(g_buffer);
            } else {
                response_buffer_.print("+UFACTORY=2,2 bad response");
            }
            LTE_Serial.print("AT+CPWROFF\r");
            len = get_response();
            if (len > 0) {
                response_buffer_.print(g_buffer);
            } else {
                response_buffer_.print("+CPRWOFF bad response");
            }

            snprintf(pcWriteBuffer, xWriteBufferLen-1, "\r\nOK\r\n");
            return pdFALSE;
        }

        if (!strncmp("ok", param, paramLen)) {
            LTE_Serial.print("ATI\r");
            int ch = waitForChar(LTE_Serial, 250);
            if (ch < 1) {
                response_buffer_.print("\r\nERROR: Timeout\r\n");
            } else {
                while (LTE_Serial.available()) {
                    response_buffer_.write(LTE_Serial.read());
                }
                response_buffer_.print("\r\nOK\r\n");
            }
            return pdTRUE;
        }

        if (!strncmp("cti", param, paramLen)) {
            bool rc = connect_to_internet();
            snprintf(pcWriteBuffer, xWriteBufferLen-1, "\r\n%s\r\n", rc ? "OK" : "ERROR");
            return pdFALSE;
        }
    }

    snprintf(pcWriteBuffer, xWriteBufferLen-1, "\r\nERROR: Invalid command\r\n");
    return pdFALSE;
}
