/**
 * @file CLI.h
 *
 * @brief FreeRTOS CLI handler.
 *
 * @date January 2023
 */
#ifndef WOMBAT_CLI_H
#define WOMBAT_CLI_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>

#include "cli/FreeRTOS_CLI.h"
#include "cli/peripherals/cat-m1.h"
#include "cli/peripherals/sdi12.h"
#include "cli/peripherals/cli_power.h"
#include "cli/peripherals/sd_card.h"
#include "bluetooth/server.h"
#include "cli/device_config/acquisition_intervals.h"
#include "cli/device_config/mqtt_cli.h"
#include "cli/device_config/config_cli.h"

#define CLI_TAG "cli"

/**
 * @brief FreeRTOS command line interface.
 *
 * Provides callbacks to various CLI components such as CAT-M1, configuration,
 * bluetooth and sdi-12 sensor configuration.
 */
class CLI {
public:
    static Stream *cliStream;
    //! Maximum command (input) length
    static const uint8_t MAX_CLI_CMD_LEN = 48;
    //! Maximum command (output) length
    static const uint8_t MAX_CLI_MSG_LEN = 255;

    static void init();
    static void repl(Stream& io);
};

#endif //WOMBAT_CLI_H
