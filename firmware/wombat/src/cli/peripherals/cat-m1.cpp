#include "cli/peripherals/cat-m1.h"

static StreamString response_buffer_;
extern Stream *cliStream;

BaseType_t CLICatM1::enter_cli(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString) {
    BaseType_t paramLen = 0;
    UBaseType_t paramNum = 1;
    const char *param;

    // More in the buffer?
    if (response_buffer_.available()) {
        memset(pcWriteBuffer, 0, xWriteBufferLen);
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
                }

                if (LTE_Serial.available()) {
                    while (LTE_Serial.available()) {
                        cliStream->write(LTE_Serial.read());
                    }
                }

                yield();
            }

            strncpy(pcWriteBuffer, "Exited Cat M1 passthrough mode\r\n", xWriteBufferLen - 1);
            return pdFALSE;
        }
    }

    return pdFALSE;
}
