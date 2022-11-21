#ifndef WOMBAT_BLE_UART_H
#define WOMBAT_BLE_UART_H

#include <Arduino.h>
#include <BLECharacteristic.h>
#include <BLEServer.h>
#include <BLE2902.h>

#include "audio_feedback.h"

// UART UUID's
#define UART_SERVICE_UUID               "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define UART_CHAR_RX_UUID               "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define UART_CHAR_TX_UUID               "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

class Wombat_BLE_UART_Service {
public:
    explicit Wombat_BLE_UART_Service(BLEServer* server);
    ~Wombat_BLE_UART_Service() = default;
};

class Node_BluetoothLE_UART_Callbacks:
        public BLECharacteristicCallbacks {

    void onWrite(BLECharacteristic* pCharacteristic) override {
        std::string rxValue = pCharacteristic->getValue();
        if(rxValue.length() > 0){
            Serial.print("[ESP32 Wombat]: ");
            for(const auto &i: rxValue){
                Serial.print(i);
            }
        } else {
            error_tone();
            Serial.println("Bluetooth RX error");
        }
    }
};


#endif //WOMBAT_BLE_UART_H
