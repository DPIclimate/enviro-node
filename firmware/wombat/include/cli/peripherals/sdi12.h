/**
 * @file sdi12.h
 *
 * @brief SDI-12 command line interface handler.
 *
 * @date January 2023
 */
#ifndef WOMBAT_CLI_SDI12_H
#define WOMBAT_CLI_SDI12_H

#include <freertos/FreeRTOS.h>
#include <Stream.h>
#include <StreamString.h>
#include "SDI12.h"
#include "dpiclimate-12.h"

#include "cli/FreeRTOS_CLI.h"
#include "Utils.h"

/**
 * @brief SDI-12 sensor CLI.
 *
 * Commands can be used to control SDI-12 sensors on the bus. The user must
 * provide the prefix `sdi12` before sending a command which will be handled
 * in enter_cli(...).
 */
class CLISdi12 {
public:
    //! CLI SDI-12 reference, to send commands use "sdi12" followed by the cmd
    inline static const std::string cmd = "sdi12";

    //! Maximum SDI-12 response length
    static const uint8_t MAX_SDI12_RES_LEN = 255;

    static BaseType_t enter_cli(char *pcWriteBuffer, size_t xWriteBufferLen,
                                const char *pcCommandString);
};

#endif //WOMBAT_CLI_SDI12_H
