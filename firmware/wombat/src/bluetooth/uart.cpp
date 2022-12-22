/**
 * @file uart.cpp
 *
 * @brief BluetoothLE UART service setup and handling.
 *
 * @date December 2022
 */
#include "bluetooth/uart.h"

//! ESP32 log tag for BLEUart debug output
#define TAG "btuart"

//! Receive buffer for message from a BLE client
char rx_buf[256];
//! Write buffer size for message to a BLE client
constexpr size_t BT_WRITE_BUF_LEN = 512;
//! Write buffer for messages being sent to a BLE client
char bluetooth_write_buf[BT_WRITE_BUF_LEN+1];

/**
 * @brief Setup BLEUart service.
 *
 * A BLEUart service is created and attached to the BLE server. Two
 * characteristics (RX and TX) are contained within this service. Callbacks
 * are used to notify the server when a message is received from a BLE client
 * and to notify the client when a message is available.
 *
 * @param server Pointer to the BLE server.
 */
BluetoothUartService::BluetoothUartService(BLEServer *server) {
    BLEService* uart_service = server->createService(UART_SERVICE_UUID);

    // TX characteristic
    BLECharacteristic* uart_tx_char = uart_service->createCharacteristic(
            UART_CHAR_TX_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
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

/**
 * @brief Callback function for messages received from a BLE client.
 *
 * When a BLE client sends a message to the BLE server running on this device
 * this method will be called. The message is extracted and check to ensure
 * data is available (if not the function returns with an error tone). The
 * message is then allocated to a buffer (rx_buf) which has its white space
 * removed. This buffer is then passed through to the CLI which responds
 * accordingly. The response from the CLI is held in another buffer which is
 * then passed back to the BLE client through a notify method.
 *
 * @param pCharacteristic Pointer to the UART characteristic.
 */
void BluetoothUartServiceCallbacks::onWrite(BLECharacteristic* pCharacteristic){

    // Extract message from characteristic
    std::string rxValue = pCharacteristic->getValue();

    // Check valid message
    if(rxValue.length() < 1){
        error_tone();
        return;
    }

    // Assign message to RX buffer
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
}
