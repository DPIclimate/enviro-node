#ifndef WOMBAT_CLI_SDI12_H
#define WOMBAT_CLI_SDI12_H

#include <freertos/FreeRTOS.h>
#include <Stream.h>
#include <StreamString.h>
#include "SDI12.h"
#include "dpiclimate-12.h"

#include "cli/FreeRTOS_CLI.h"
#include "Utils.h"

class CLISdi12 {
public:
    inline static const std::string cmd = "sdi12";

    static const uint8_t MAX_SDI12_RES_LEN = 255;

    static BaseType_t enter_cli(char *pcWriteBuffer, size_t xWriteBufferLen,
                                const char *pcCommandString);
};

#endif //WOMBAT_CLI_SDI12_H
