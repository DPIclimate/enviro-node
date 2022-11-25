#include "bluetooth/uart.h"

char bluetooth_write_buf[1024];

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
        BaseType_t rc = pdTRUE;
        while (rc != pdFALSE) {
            // Send command to CLI
            rc = FreeRTOS_CLIProcessCommand(rxValue.c_str(), bluetooth_write_buf,
                                            sizeof(bluetooth_write_buf));

            // Notify device with CLI response
            BluetoothServer::notify_device(bluetooth_write_buf);
            Serial.print(bluetooth_write_buf);

            // Reset the bluetooth TX buffer
            memset(bluetooth_write_buf, 0, sizeof(bluetooth_write_buf));
        }
        Serial.print("$");
    } else{
        error_tone();
    }
}