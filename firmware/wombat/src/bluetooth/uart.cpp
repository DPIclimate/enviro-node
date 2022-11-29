#include "bluetooth/uart.h"

#define TAG "btuart"

char rx_buf[256];
constexpr size_t BT_WRITE_BUF_LEN = 1024;
char bluetooth_write_buf[BT_WRITE_BUF_LEN+1];

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

    if (rxValue.length() > 0) {
        memset(rx_buf, 0, sizeof(rx_buf));
        size_t mcl = sizeof(rx_buf);
        if (rxValue.length() < mcl) {
            mcl = rxValue.length();
        }

        memcpy(rx_buf, rxValue.c_str(), mcl);
        stripWS(rx_buf);
        ESP_LOGI(TAG, "recv: [%s], len = %d", rx_buf, strlen(rx_buf));

        BaseType_t rc = pdTRUE;
        while (rc != pdFALSE) {
            // Send command to CLI
            rc = FreeRTOS_CLIProcessCommand(rx_buf, bluetooth_write_buf, BT_WRITE_BUF_LEN);

            // Notify device with CLI response
            BluetoothServer::notify_device(bluetooth_write_buf);

            // Reset the bluetooth TX buffer
            memset(bluetooth_write_buf, 0, sizeof(bluetooth_write_buf));
        }
    } else {
        error_tone();
    }
}
