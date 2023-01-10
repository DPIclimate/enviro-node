/**
 * @file cat-m1.h
 *
 * @brief CAT-M1 CLI command request and response handler.
 *
 * @date January 2023
 */
#ifndef WOMBAT_CLI_CAT_M1_H
#define WOMBAT_CLI_CAT_M1_H

#include <freertos/FreeRTOS.h>
#include <Stream.h>
#include <StreamString.h>

#include "cli/FreeRTOS_CLI.h"
#include "CAT_M1.h"

/**
 * @brief CLI for CAT-M1 commands.
 *
 * Features a pass through mode where commands are sent directly to the modem.
 * Several other commands are inbuilt to allow for communications to and from
 * the modem without typing several AT-commands.
 */
class CLICatM1 {
public:
    //! CLI CAT-M1 reference, to send commands use "c1" followed by the command
    inline static const std::string cmd = "c1";

    static BaseType_t enter_cli(char *pcWriteBuffer, size_t xWriteBufferLen,
                         const char *pcCommandString);
};


#endif //WOMBAT_CLI_CAT_M1_H
