#include "bluetooth/uart.h"

BluetoothUartService::BluetoothUartService(BLEServer *server) {
    BLEService* uart_service = server->createService(UART_SERVICE_UUID);

    // TX characteristic
    BLECharacteristic* uart_tx_char = uart_service->createCharacteristic(
            UART_CHAR_TX_UUID, BLECharacteristic::PROPERTY_NOTIFY);
    uart_tx_char->addDescriptor(new BLE2902());

    // RX characteristic
    BLECharacteristic* uart_rx_char = uart_service->createCharacteristic(
            UART_CHAR_RX_UUID, BLECharacteristic::PROPERTY_WRITE);

    // Set callbacks for characteristics
    uart_rx_char->setCallbacks(new BluetoothUartServiceCallbacks);
    // Start service
    uart_service->start();

    // Add UART service to advertising
    server->getAdvertising()->addServiceUUID(uart_service->getUUID());
}

void BluetoothUartServiceCallbacks::onWrite(BLECharacteristic* pCharacteristic){
    std::string rxValue = pCharacteristic->getValue();
    if(rxValue.length() > 0){
        for(const auto &i: rxValue){
            Serial.write(i);
        }
        Serial.println();
    }
    else{
        error_tone();
    }
}