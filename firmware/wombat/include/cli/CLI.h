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
#include "cli/peripherals/cat-m1.h"

#define CLI_TAG "cli"

const char OK_RESPONSE[] = "\r\nOK\r\n";
const char ERROR_RESPONSE[] = "\r\nERROR\r\n";
const char INVALID_CMD_RESPONSE[] = "\r\nERROR: Invalid command\r\n";

/**
 * @brief FreeRTOS command line interface.
 *
 * Provides callbacks to various CLI components such as CAT-M1, configuration,
 * bluetooth and sdi-12 sensor configuration.
 */
class CLI {
public:
    static Stream *cliInput;
    static Stream *cliOutput;
    //! Maximum command (input) length
    static const size_t MAX_CLI_CMD_LEN = 256;
    //! Maximum command (output) length
    static const size_t MAX_CLI_MSG_LEN = 256;

    static void init();
    static void repl(Stream& input, Stream& output);
};

#endif //WOMBAT_CLI_H
