#include "cli/peripherals/sdi12.h"

static SDI12 sdi12(4);
static StreamString response_buffer_;
static char response[CLISdi12::MAX_SDI12_RES_LEN];
static Stream* stream_;

BaseType_t CLISdi12::enter_cli(char *pcWriteBuffer, size_t xWriteBufferLen,
                               const char *pcCommandString) {
    BaseType_t paramLen = 0;
    UBaseType_t paramNum = 1;
    const char *param;

    // More in the buffer?
    if (response_buffer_.available()) {
        memset(pcWriteBuffer, 0, xWriteBufferLen);
        size_t len = response_buffer_.readBytesUntil('\n', pcWriteBuffer,
                                                     xWriteBufferLen-1);

        // readBytesUntil strips the delimiter, so put the '\n' back in.
        if (len <= xWriteBufferLen) {
            pcWriteBuffer[len-1] = '\n';
        }

        return response_buffer_.available() > 0 ? pdTRUE : pdFALSE;
    }

    memset(pcWriteBuffer, 0, xWriteBufferLen);
    param = FreeRTOS_CLIGetParameter(pcCommandString, paramNum, &paramLen);
    sdi12.begin();
    if (param != nullptr && paramLen > 0) {
        if (!strncmp("scan", param, paramLen)) {
            if (stream_ == nullptr) {
                strncpy(pcWriteBuffer,
                        "ERROR: input stream not set for SDI-12 scan\r\n",
                        xWriteBufferLen - 1);
                return pdFALSE;
            }

            stream_->println("Scanning addresses 0 - 9");
            char iCmd[] = { 0, 'I', '!', 0};
            for (char addr = '0'; addr <= '9'; addr++) {
                iCmd[0] = addr;
                sdi12.sendCommand(iCmd);
                memset(response, 0, sizeof(response));
                size_t len = sdi12.readBytesUntil('\n', response,
                                                  MAX_SDI12_RES_LEN);
                if (len > 0) {
                    stripWS(response);
                    if (response[0] != 0) {
                        stream_->println(response);
                    }
                }
            }

            strncpy(pcWriteBuffer, "SDI-12 scan complete\r\n",
                    xWriteBufferLen - 1);
            return pdFALSE;
        }

        if (!strncmp(">>", param, paramLen)) {
            const char* sdi12_cmd = FreeRTOS_CLIGetParameter(pcCommandString,
                                                             2,
                                                             &paramLen);
            if(sdi12_cmd != nullptr && paramLen > 0){
                stream_->print("SDI-12 CMD received: ");
                stream_->println(sdi12_cmd);

                sdi12.sendCommand(sdi12_cmd);

                int ch;
                while (sdi12.available()) {
                    ch = sdi12.read();
                    stream_->write(ch);
                }
            }
        }
    }

    sdi12.end();
    strncpy(pcWriteBuffer, "Syntax error\r\n", xWriteBufferLen - 1);
    return pdFALSE;


}
