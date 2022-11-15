#ifndef BLE_H
#define BLE_H

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#include "ble_uart.h"
#include "audio_feedback.h"

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
        completed_tone();
        device_connected = true;
    }

    void onDisconnect(BLEServer* pServer) override {
        error_tone();
        device_connected = false;
    }
};


#endif //BLE_H
