/**
 * @file mqtt_cli.h
 *
 * @brief MQTT setup and configuration through the CLI.
 *
 * @date January 2023
 */
#ifndef WOMBAT_MQTT_CLI_H
#define WOMBAT_MQTT_CLI_H

#include "DeviceConfig.h"

/**
 * @brief CLI MQTT configuration.
 *
 * Provides an interface between the user and the MQTT settings stored on the
 * ESP32 in SPIFFS.
 */
class CLIMQTT {
    //! Get the current device configuration upon initialisation
    inline static DeviceConfig& config = DeviceConfig::get();

public:
    //! Prefix for all MQTT related configuration commands
    inline static const std::string cmd = "mqtt";

    static void dump(Stream& stream);

    static BaseType_t enter_cli(char *pcWriteBuffer, size_t xWriteBufferLen,
                                const char *pcCommandString);
};

#endif //WOMBAT_MQTT_CLI_H
