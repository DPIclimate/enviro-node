#ifndef WOMBAT_UART_H
#define WOMBAT_UART_H

#include <Arduino.h>
#include <BLECharacteristic.h>
#include <BLEServer.h>
#include <BLE2902.h>


#include "audio-feedback/tones.h"
#include "bluetooth/server.h"

// UART UUID's
#define UART_SERVICE_UUID               "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define UART_CHAR_RX_UUID               "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define UART_CHAR_TX_UUID               "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

class BluetoothUartService {
public:
    explicit BluetoothUartService(BLEServer* server);
    ~BluetoothUartService() = default;
};

class BluetoothUartServiceCallbacks:
        public BLECharacteristicCallbacks {

    void onWrite(BLECharacteristic* pCharacteristic) override;
};


#endif //WOMBAT_UART_H
