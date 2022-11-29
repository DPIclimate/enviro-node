#include "cli/peripherals/sdi12.h"

static StreamString response_buffer_;
static char response[CLISdi12::MAX_SDI12_RES_LEN];
static Stream* stream_;

extern SDI12 sdi12;
extern DPIClimate12 dpi12;
extern sensor_list sensors;

#define TAG "cli_sdi12"

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
        // But what to do about responses that don't have more than 1 line?
        if (len <= xWriteBufferLen) {
            pcWriteBuffer[len-1] = '\n';
        }

        return response_buffer_.available() > 0 ? pdTRUE : pdFALSE;
    }

    memset(pcWriteBuffer, 0, xWriteBufferLen);
    param = FreeRTOS_CLIGetParameter(pcCommandString, paramNum, &paramLen);
    if (param != nullptr && paramLen > 0) {
        if (!strncmp("scan", param, paramLen)) {
            sdi12.begin();

            char iCmd[] = { 0, 'I', '!', 0};
            for (char addr = '0'; addr <= '9'; addr++) {
                yield();

                iCmd[0] = addr;
                sdi12.sendCommand(iCmd);
                memset(response, 0, sizeof(response));
                size_t len = sdi12.readBytesUntil('\n', response, MAX_SDI12_RES_LEN);
                len = stripWS(response);
                if (len > 0) {
                    ESP_LOGI(TAG, "[%c] -> %s", addr, response);
                    if (! response_buffer_.isEmpty()) {
                        response_buffer_.print(',');
                    }

                    response_buffer_.print(response);
                } else {
                    ESP_LOGI(TAG, "[%c] -> No response", addr);
                }
            }

            // 113METER   TER12 201T12-00082937
            // 1+1807.82+21.0+1
            // 213METER   TER12 301T11-00019674
            // 2+1820.29+20.9

            // While we can't read real sensors
            response_buffer_.println("113METER   TER12 201T12-00082937");
            response_buffer_.println("213METER   TER11 301T11-00019674");

            sdi12.end();

            if (response_buffer_.isEmpty()) {
                strncpy(pcWriteBuffer, "X", xWriteBufferLen-1);
                return pdFALSE;
            }

            return pdTRUE;
        }

        if (!strncmp(">>", param, paramLen)) {
            const char* sdi12_cmd = FreeRTOS_CLIGetParameter(pcCommandString, 2, &paramLen);
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

    strncpy(pcWriteBuffer, "Syntax error\r\n", xWriteBufferLen - 1);
    return pdFALSE;


}
