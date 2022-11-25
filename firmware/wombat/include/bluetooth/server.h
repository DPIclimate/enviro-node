#ifndef WOMBAT_BLUETOOTH_SERVER_H
#define WOMBAT_BLUETOOTH_SERVER_H

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#include "bluetooth/uart.h"
#include "cli/CLI.h"

class BluetoothServer {

public:
    BluetoothServer() = default;
    ~BluetoothServer() = default;
    static void begin();

    inline static bool device_connected = false;
    inline static bool current_device = false;

    static void read_write_blocking();
    static bool is_device_connected();
    static void notify_device(const char* message);

};

class BluetoothServerCallbacks: public BLEServerCallbacks,
                                public BluetoothServer {
    void onConnect(BLEServer* pServer) override {
        completed_tone();
        device_connected = true;
    }

    void onDisconnect(BLEServer* pServer) override {
        error_tone();
        device_connected = false;
    }
};

#endif //WOMBAT_BLUETOOTH_SERVER_H
