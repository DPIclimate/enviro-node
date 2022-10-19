#ifndef NODE_BLUETOOTH_H
#define NODE_BLUETOOTH_H

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#include "node_config.h"
#include "node_ble_config.h"
#include "node_ble_uart.h"
#include "power_monitoring/node_ble_power.h"

class Node_BluetoothLE {

public:
    Node_BluetoothLE() = default;
    ~Node_BluetoothLE() = default;
    void begin();

    inline static bool device_connected = false;
    inline static bool current_device = false;

    BLEServer *server; ///< BluetoothLE device as a server

    [[noreturn]] void read_write_blocking() const;

private:

};

class Node_BluetoothLE_Server_Callbacks: public BLEServerCallbacks,
        public Node_BluetoothLE {
    void onConnect(BLEServer* pServer) override {
        #ifdef DEBUG
            log_msg("[DEBUG]: Device connected.");
        #endif
        device_connected = true;
    }

    void onDisconnect(BLEServer* pServer) override {
        #ifdef DEBUG
            log_msg("[DEBUG]: Device disconnected.");
        #endif
        device_connected = false;
    }
};

#endif // NODE_BLUETOOTH_H