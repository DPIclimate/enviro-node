/**
 * @file uart.h
 *
 * @brief BluetoothLE UART service setup and handling.
 *
 * @date December 2022
 */
#ifndef WOMBAT_UART_H
#define WOMBAT_UART_H

#include <Arduino.h>
#include <BLECharacteristic.h>
#include <BLEServer.h>
#include <BLE2902.h>

#include "audio-feedback/tones.h"
#include "bluetooth/server.h"

//! BLE UART service UUID
#define UART_SERVICE_UUID               "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
//! BLE UART RX (receive) UUID
#define UART_CHAR_RX_UUID               "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
//! BLE UART TX (transmit) UUID
#define UART_CHAR_TX_UUID               "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

/**
 * @brief BluetoothLE UART service.
 *
 * Setup BLE UART service to allow for the transfer of plain text message
 * between a BLE client and the BLE server running on this device.
 */
class BluetoothUartService {
public:
    explicit BluetoothUartService(BLEServer* server);
    ~BluetoothUartService() = default;
};

/**
 * @brief Handles RX and TX of BLEUart messages.
 *
 * BLEUart client messages are sanitised and passed through to the CLI. The
 * response from the CLI is then passed back to the BLE client.
 */
class BluetoothUartServiceCallbacks:
        public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* pCharacteristic) override;
};


#endif //WOMBAT_UART_H
