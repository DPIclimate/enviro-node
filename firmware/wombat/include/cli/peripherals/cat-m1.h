#ifndef WOMBAT_CLI_CAT_M1_H
#define WOMBAT_CLI_CAT_M1_H

#include <freertos/FreeRTOS.h>
#include <Stream.h>
#include <StreamString.h>

#include "cli/FreeRTOS_CLI.h"
#include "CAT_M1.h"

class CLICatM1 {
public:
    inline static const std::string cmd = "c1";

    static BaseType_t enter_cli(char *pcWriteBuffer, size_t xWriteBufferLen,
                         const char *pcCommandString);
};


#endif //WOMBAT_CLI_CAT_M1_H
