/**
 * @file acquisition_intervals.h
 *
 * @brief Handles the time between measurements and uplinks by the device over
 * LTE.
 *
 * @date December 2022
 */
#ifndef WOMBAT_CLI_ACQUISITION_INTERVALS_H
#define WOMBAT_CLI_ACQUISITION_INTERVALS_H

#include <freertos/FreeRTOS.h>
#include <Stream.h>
#include <StreamString.h>

#include "cli/FreeRTOS_CLI.h"
#include "DeviceConfig.h"

/**
 * @brief Setup acquisition intervals based on device configuration stored in
 * SPIFFS memory or via the CLI.
 */
class CLIConfigIntervals {
    //! Get the device configuration from SPIFFS memory on initialisation
    inline static DeviceConfig& config = DeviceConfig::get();

public:
    //! Command to change measurement and/or uplink interval
    inline static const std::string cmd = "interval";

    static BaseType_t enter_cli(char *pcWriteBuffer, size_t xWriteBufferLen,
                                const char *pcCommandString);

    static void dump(Stream& stream);
};

#endif //WOMBAT_CLI_ACQUISITION_INTERVALS_H
