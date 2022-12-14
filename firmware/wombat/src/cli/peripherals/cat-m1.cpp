#include "cli/peripherals/cat-m1.h"
#include "CAT_M1.h"
#include "Utils.h"
#include "SparkFun_u-blox_SARA-R5_Arduino_Library.h"
#include "globals.h"

extern SARA_R5 r5;

static StreamString response_buffer_;
extern Stream *cliStream;

static int get_response(uint32_t timout = 500) {
    int ch = waitForChar(LTE_Serial, timout);
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

BaseType_t CLICatM1::enter_cli(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString) {
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
    }

    snprintf(pcWriteBuffer, xWriteBufferLen-1, "\r\nERROR: Invalid command\r\n");
    return pdFALSE;
}
