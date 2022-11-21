#include "bluetooth/ble_uart.h"

Wombat_BLE_UART_Service::Wombat_BLE_UART_Service(BLEServer *server) {
    BLEService* uart_service = server->createService(UART_SERVICE_UUID);

    // TX characteristic
    BLECharacteristic* uart_tx_char = uart_service->createCharacteristic(
            UART_CHAR_TX_UUID, BLECharacteristic::PROPERTY_NOTIFY);
    uart_tx_char->addDescriptor(new BLE2902());

    // RX characteristic
    BLECharacteristic* uart_rx_char = uart_service->createCharacteristic(
            UART_CHAR_RX_UUID, BLECharacteristic::PROPERTY_WRITE);

    // Set callbacks for characteristics
    uart_rx_char->setCallbacks(new Node_BluetoothLE_UART_Callbacks);
    // Start service
    uart_service->start();

    server->getAdvertising()->addServiceUUID(uart_service->getUUID());


}

void Node_BluetoothLE_UART_Callbacks::onWrite(BLECharacteristic* pCharacteristic){

    std::string rxValue = pCharacteristic->getValue();

    if(rxValue.length() > 0){
        Serial.print("[ESP32 Wombat]: ");
        for(const auto &i: rxValue){
            Serial.print(i);
        }
        if(strncmp(rxValue.c_str(), "sdi12 pt", rxValue.length()) == 0){
            disableCore1WDT();
            disableCore0WDT();
            sdi12_pt(&Serial);
            enableCore0WDT();
            enableCore1WDT();
        }
    }
    else{
            error_tone();
            Serial.println("Bluetooth RX error");
    }
}