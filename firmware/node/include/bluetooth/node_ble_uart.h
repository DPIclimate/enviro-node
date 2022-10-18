#ifndef NODE_BLE_UART_H
#define NODE_BLE_UART_H

#include <Arduino.h>
#include <BLECharacteristic.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include "node_config.h"
#include "node_ble_config.h"

class Node_BLE_UART_Service {
public:
    explicit Node_BLE_UART_Service(BLEServer* server);
    ~Node_BLE_UART_Service() = default;
};

class Node_BluetoothLE_UART_Callbacks:
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

#endif //NODE_BLE_UART_H
