#include <Arduino.h>
#include "CLI.h"

#include <freertos/FreeRTOS.h>
#include "FreeRTOS_CLI.h"
#include "Config.h"
#include "Utils.h"
#include "dpiclimate-12.h"
#include "CAT_M1.h"
#include "bluetooth/ble.h"
#include <esp_log.h>
#include <StreamString.h>

#define TAG "cli"

static BaseType_t doInterval(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString);
static BaseType_t doSDI12(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString);
static BaseType_t doCatM1(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString);
static BaseType_t doBtUart(char* pcWriteBuffer, size_t xWriteBufferLen, const char* pcCommandString);

static const CLI_Command_Definition_t intervalCmd = { "interval", "interval:\r\n Configure interval settings\r\n", doInterval, -1 };
static const CLI_Command_Definition_t sdi12Cmd = { "sdi12", "sdi12:\r\n Work with SDI-12 sensors\r\n", doSDI12, -1 };
static const CLI_Command_Definition_t catM1Cmd = { "catm1", "catm1:\r\n Work with the Cat M1 modem\r\n", doCatM1, -1 };
static const CLI_Command_Definition_t btCmd= { "bt", "bt:\r\n Communicate over Bluetooth UART\r\n", doBtUart, -1 };

static Config& config = Config::get();

static Stream* stream = nullptr;


void cliInitialise(void) {
    ESP_LOGI(TAG, "Registering CLI commands");
    FreeRTOS_CLIRegisterCommand(&intervalCmd);
    FreeRTOS_CLIRegisterCommand(&sdi12Cmd);
    FreeRTOS_CLIRegisterCommand(&catM1Cmd);
    FreeRTOS_CLIRegisterCommand(&btCmd);
}

// Commands returning multiple lines should write their output to this, and then
// call returnNextResponseLine each time they are called and responseBuffer.available() > 0.
static StreamString responseBuffer;

static BaseType_t returnNextResponseLine(char *pcWriteBuffer, size_t xWriteBufferLen) {
    if (responseBuffer.available()) {
        memset(pcWriteBuffer, 0, xWriteBufferLen);
        size_t len = responseBuffer.readBytesUntil('\n', pcWriteBuffer, xWriteBufferLen-1);

        // readBytesUntil strips the delimiter, so put the '\n' back in.
        if (len <= xWriteBufferLen) {
            pcWriteBuffer[len-1] = '\n';
        }

        return responseBuffer.available() > 0 ? pdTRUE : pdFALSE;
    }

    return pdFALSE;
}

// INTERVAL
//
// The interval command sets and gets the various interval values, such as the measurement interval and the
// uplink interval. Intervals are specified in seconds.
BaseType_t doInterval(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString) {
    BaseType_t paramLen = 0;
    UBaseType_t paramNum = 1;
    const char* param;

    if (responseBuffer.available()) {
        return returnNextResponseLine(pcWriteBuffer, xWriteBufferLen);
    }

    memset(pcWriteBuffer, 0, xWriteBufferLen);
    param = FreeRTOS_CLIGetParameter(pcCommandString, paramNum, &paramLen);
    if (param != nullptr && paramLen > 0) {
        if (!strncmp("list", param, paramLen)) {
            responseBuffer.clear();
            responseBuffer.print("interval measure ");
            responseBuffer.println(config.getMeasureInterval());
            responseBuffer.print("interval uplink ");
            responseBuffer.println(config.getUplinkInterval());
            memset(pcWriteBuffer, 0, xWriteBufferLen);
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
                strncpy(pcWriteBuffer, "ERROR: Missing or invalid interval value\r\n", xWriteBufferLen - 1);
                return pdFALSE;
            }

            config.setMeasureInterval(i);
            if (i == config.getMeasureInterval()) {
                strncpy(pcWriteBuffer, "OK\r\n", xWriteBufferLen - 1);
            } else {
                strncpy(pcWriteBuffer, "ERROR: set measure interval failed\r\n", xWriteBufferLen - 1);
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
                strncpy(pcWriteBuffer, "ERROR: Missing or invalid interval value\r\n", xWriteBufferLen - 1);
                return pdFALSE;
            }

            config.setUplinkInterval(i);
            if (i == config.getUplinkInterval()) {
                strncpy(pcWriteBuffer, "OK\r\n", xWriteBufferLen - 1);
            } else {
                strncpy(pcWriteBuffer, "ERROR: set uplink interval failed\r\n", xWriteBufferLen - 1);
            }
            return pdFALSE;
        }
    }

    strncpy(pcWriteBuffer, "Syntax error\r\n", xWriteBufferLen - 1);
    return pdFALSE;
}

extern SDI12 sdi12;
extern DPIClimate12 dpi12;

#define MAX_CMD 48
static char cmd[MAX_CMD+1];

#define MAX_MSG 256
static char msg[MAX_MSG+1];

BaseType_t doSDI12(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString) {
    BaseType_t paramLen = 0;
    UBaseType_t paramNum = 1;
    const char *param;

    if (responseBuffer.available()) {
        return returnNextResponseLine(pcWriteBuffer, xWriteBufferLen);
    }

    memset(pcWriteBuffer, 0, xWriteBufferLen);
    param = FreeRTOS_CLIGetParameter(pcCommandString, paramNum, &paramLen);
    if (param != nullptr && paramLen > 0) {
        if (!strncmp("scan", param, paramLen)) {
            if (stream == nullptr) {
                strncpy(pcWriteBuffer, "ERROR: input stream not set for SDI-12 scan\r\n", xWriteBufferLen - 1);
                return pdFALSE;
            }

            sdi12.begin();
            stream->println("Scanning addresses 0 - 9");

            char iCmd[] = { 0, 'I', '!', 0};
            for (char addr = '0'; addr <= '9'; addr++) {
                iCmd[0] = addr;
                sdi12.sendCommand(iCmd);
                memset(msg, 0, sizeof(msg));
                size_t len = sdi12.readBytesUntil('\n', msg, MAX_MSG);
                if (len > 0) {
                    stripWS(msg);
                    if (msg[0] != 0) {
                        stream->println(msg);
                    }
                }
            }

            sdi12.end();
            strncpy(pcWriteBuffer, "SDI-12 scan complete\r\n", xWriteBufferLen - 1);
            return pdFALSE;
        }

        if (!strncmp("pt", param, paramLen)) {
            bool state = sdi12_pt(stream);
            if(state){
                return pdTRUE;
            } else {
                return pdFALSE;
            }
        }
    }

    strncpy(pcWriteBuffer, "Syntax error\r\n", xWriteBufferLen - 1);
    return pdFALSE;
}

bool sdi12_pt(Stream* sdi12_stream){

    sdi12_stream->println("Entering SDI-12 passthrough mode, press ctrl-D to exit");
    sdi12.begin();

    int ch;
    while (true) {
        if (sdi12_stream->available()) {
            ch = sdi12_stream->peek();
            if (ch == 0x04) {
                break;
            }

            memset(cmd, 0, sizeof(cmd));
            readFromStreamUntil(*sdi12_stream, '\n', cmd, sizeof(cmd));
            Serial.println(cmd);
            stripWS(cmd);
            if (strlen(cmd) > 0) {
                sdi12.sendCommand(cmd);
            }
        }

        if (sdi12.available()) {
            ch = sdi12.read();
            sdi12_stream->write(ch);
        }

        yield();
    }

    sdi12.end();
    return true;
}

BaseType_t doCatM1(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString) {
    BaseType_t paramLen = 0;
    UBaseType_t paramNum = 1;
    const char *param;

    if (responseBuffer.available()) {
        return returnNextResponseLine(pcWriteBuffer, xWriteBufferLen);
    }

    memset(pcWriteBuffer, 0, xWriteBufferLen);
    param = FreeRTOS_CLIGetParameter(pcCommandString, paramNum, &paramLen);
    if (param != nullptr && paramLen > 0) {
        if (!strncmp("pt", param, paramLen)) {
            if (stream == nullptr) {
                strncpy(pcWriteBuffer, "ERROR: input stream not set for Cat M1 passthrough\r\n", xWriteBufferLen - 1);
                return pdFALSE;
            }

            stream->println("Entering Cat M1 passthrough mode, press ctrl-D to exit");

            int ch;
            bool finish = false;
            while ( ! finish) {
                if (stream->available()) {
                    while (stream->available()) {
                        ch = stream->read();
                        if (ch == 0x04) {
                            finish = true;
                            break;
                        }
                        LTE_Serial.write(ch);
                    }
                }

                if (LTE_Serial.available()) {
                    while (LTE_Serial.available()) {
                        stream->write(LTE_Serial.read());
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


static Wombat_BluetoothLE bt;

BaseType_t doBtUart(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString) {

    bt.begin();
    bt.read_write_blocking();

    return pdFALSE;
}


void repl(Stream& io) {
    // stream is used by the SDI-12 & Cat M1 passthrough modes, so must be set from here before either
    // mode is used.
    stream = &io;
    io.println("Entering repl");

    while (true) {
        io.print("$ ");
        while (!io.available()) {
            yield();
        }

        size_t len = readFromStreamUntil(io, '\n', cmd, MAX_CMD);
        if (len > 1) {
            cmd[len] = 0;
            stripWS(cmd);
            if (!strcmp("exit", cmd)) {
                io.println("Exiting repl");
                return;
            }

            BaseType_t rc = pdTRUE;
            while (rc != pdFALSE) {
                rc = FreeRTOS_CLIProcessCommand(cmd, msg, MAX_MSG);
                Serial.print(msg);
            }
        }
    }
}
