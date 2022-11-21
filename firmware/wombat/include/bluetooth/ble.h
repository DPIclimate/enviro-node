#ifndef WOMBAT_BLE_H
#define WOMBAT_BLE_H

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#include "bluetooth/ble_uart.h"

class Wombat_BluetoothLE {

public:
    Wombat_BluetoothLE() = default;
    ~Wombat_BluetoothLE() = default;
    void begin();

    inline static bool device_connected = false;
    inline static bool current_device = false;

    BLEServer *server; ///< BluetoothLE device as a server

    void read_write_blocking() const;

private:

};

class Wombat_BluetoothLE_Server_Callbacks: public BLEServerCallbacks,
                                           public Wombat_BluetoothLE {
    void onConnect(BLEServer* pServer) override {
        completed_tone();
        device_connected = true;
    }

    void onDisconnect(BLEServer* pServer) override {
        error_tone();
        device_connected = false;
    }
};

#endif //WOMBAT_BLE_H
