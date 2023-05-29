/**
 * @file ftp_cli.h
 *
 * @brief FTP setup and configuration through the CLI.
 */
#ifndef WOMBAT_FTP_CLI_H
#define WOMBAT_FTP_CLI_H

#include "DeviceConfig.h"

/**
 * @brief CLI FTP configuration.
 *
 * Provides an interface between the user and the FTP settings stored on the
 * ESP32 in SPIFFS.
 */
class CLIFTP {
    //! Get the current device configuration upon initialisation
    inline static DeviceConfig& config = DeviceConfig::get();

public:
    //! Prefix for all FTP related configuration commands
    inline static const std::string cmd = "ftp";

    static void dump(Stream& stream);

    static BaseType_t enter_cli(char *pcWriteBuffer, size_t xWriteBufferLen,
                                const char *pcCommandString);
};

#endif //WOMBAT_FTP_CLI_H
