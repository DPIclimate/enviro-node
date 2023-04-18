#ifndef WOMBAT_SD_CARD_H
#define WOMBAT_SD_CARD_H

#include <freertos/FreeRTOS.h>
#include <string>

/**
 * @brief SD card related commands.
 */
class CLISDCard {
public:
    //! SD card related commands in the CLI begin with "sd".
    inline static const std::string cmd = "sd";

    static BaseType_t enter_cli(char *pcWriteBuffer, size_t xWriteBufferLen,
                                const char *pcCommandString);
};

#endif //WOMBAT_SD_CARD_H
