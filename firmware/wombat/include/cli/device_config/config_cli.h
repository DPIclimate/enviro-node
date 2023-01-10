/**
 * @file config_cli.cpp
 *
 * @brief Command line interface node configuration commands.
 *
 * @date January 2023
 */
#ifndef WOMBAT_CONFIG_CLI_H
#define WOMBAT_CONFIG_CLI_H

#include "DeviceConfig.h"

/**
 * @brief CLI interface to get and set device configuration options.
 */
class CLIConfig {
    //! Current device configuration (called upon initialisation)
    inline static DeviceConfig& config = DeviceConfig::get();

public:
    //! CLI prefix for configuration commands
    inline static const std::string cmd = "config";

    static BaseType_t enter_cli(char *pcWriteBuffer, size_t xWriteBufferLen,
                                const char *pcCommandString);
};

#endif //WOMBAT_CONFIG_CLI_H
