#ifndef WOMBAT_CLI_SPIFFS_H
#define WOMBAT_CLI_SPIFFS_H

#include <freertos/FreeRTOS.h>
#include <string>

/**
 * @brief SPIFFS related commands.
 */
class CLISPIFFS {
public:
    //! SPIFFS card related commands in the CLI begin with "spiffs".
    inline static const std::string cmd = "spiffs";

    static BaseType_t enter_cli(char *pcWriteBuffer, size_t xWriteBufferLen,
                                const char *pcCommandString);
};

#endif //WOMBAT_CLI_SPIFFS_H
