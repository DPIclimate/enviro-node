#ifndef NODE_BLUETOOTH_H
#define NODE_BLUETOOTH_H

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "node_config.h"

#define SERVICE_UUID           "6385d4ce-5b69-4f4e-b08c-738867fd15d1"
#define CHARACTERISTIC_UUID_RX "6385d4ce-5b69-4f4e-b08c-738867fd15d2"
#define CHARACTERISTIC_UUID_TX "6385d4ce-5b69-4f4e-b08c-738867fd15d3"

class Node_BluetoothLE {

public:
    Node_BluetoothLE() = default;
    ~Node_BluetoothLE() = default;
    void begin();

    inline static bool device_connected = false;
    inline static bool current_device = false;

    /**
    * Bluetooth handler.
    *
    * Allows for the setup of bluetooth specific services.
    * - OTA firmware updates
    * - UART over bluetooth
    * - Device information
    * - Battery status
    *
    * Also provided are options for TX power (may need to be adjusted) and
    * the maximum number for concurrent connections over bluetooth.
    */
    struct Handler {
        BLEDevice *device; ///< BluetoothLE device
        BLEServer *server; ///< BluetoothLE device as a server
        BLEService *service; ///< Over the air firmware update
        BLECharacteristic *TxCharacteristic;
        BLECharacteristic *RxCharacteristic;
    } bt_handler;

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

class Node_BluetoothLE_Characteristic_Callbacks:
        public BLECharacteristicCallbacks {

    void onWrite(BLECharacteristic* pCharacteristic) override{
        std::string rxValue = pCharacteristic->getValue();
        if(rxValue.length() > 0){
            Serial.print("Received: ");
            for(const auto &i: rxValue){
                Serial.print(i);
            }
        }
    }

};

#endif // NODE_BLUETOOTH_H
