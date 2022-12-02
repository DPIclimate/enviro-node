#ifndef WOMBAT_CONFIG_CLI_H
#define WOMBAT_CONFIG_CLI_H

#include "DeviceConfig.h"

class CLIConfig {
    inline static DeviceConfig& config = DeviceConfig::get();

public:
    inline static const std::string cmd = "config";

    static BaseType_t enter_cli(char *pcWriteBuffer, size_t xWriteBufferLen,
                                const char *pcCommandString);
};

#endif //WOMBAT_CONFIG_CLI_H
