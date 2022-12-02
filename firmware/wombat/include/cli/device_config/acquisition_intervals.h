#ifndef WOMBAT_CLI_INTERVALS_H
#define WOMBAT_CLI_INTERVALS_H

#include <freertos/FreeRTOS.h>
#include <Stream.h>
#include <StreamString.h>

#include "cli/FreeRTOS_CLI.h"
#include "DeviceConfig.h"

class CLIConfigIntervals {
    inline static DeviceConfig& config = DeviceConfig::get();

public:
    inline static const std::string cmd = "interval";

    static BaseType_t enter_cli(char *pcWriteBuffer, size_t xWriteBufferLen,
                                const char *pcCommandString);

    static void dump(Stream& stream);
};

#endif //WOMBAT_CLI_INTERVALS_H
