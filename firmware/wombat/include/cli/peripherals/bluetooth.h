#ifndef WOMBAT_CLI_BLUETOOTH_H
#define WOMBAT_CLI_BLUETOOTH_H

#include <freertos/FreeRTOS.h>
#include <Stream.h>
#include <StreamString.h>

#include "bluetooth/server.h"

class CLIBluetooth{
public:
    static BaseType_t enter_cli(char *pcWriteBuffer, size_t xWriteBufferLen,
                                const char *pcCommandString);
};

#endif //WOMBAT_CLI_BLUETOOTH_H
