#ifndef WOMBAT_CLI_POWER_H
#define WOMBAT_CLI_POWER_H

#include <freertos/FreeRTOS.h>
#include <Stream.h>
#include <StreamString.h>

#include "cli/FreeRTOS_CLI.h"
#include "CAT_M1.h"

/**
 * @brief Power related commands.
 */
class CLIPower {
public:
    //! Power related commands in the CLI begin with "pwr".
    inline static const std::string cmd = "pwr";

    static BaseType_t enter_cli(char *pcWriteBuffer, size_t xWriteBufferLen,
                                const char *pcCommandString);
};

#endif //WOMBAT_CLI_POWER_H
