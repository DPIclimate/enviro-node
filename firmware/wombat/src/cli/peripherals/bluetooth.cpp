#include "cli/peripherals/bluetooth.h"

BaseType_t CLIBluetooth::enter_cli(char *pcWriteBuffer, size_t xWriteBufferLen,
                                   const char *pcCommandString) {
    BluetoothServer::begin();
    return pdFALSE;
}
