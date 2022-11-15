#include "ble.h"

static char message_buffer[UINT8_MAX];

void Node_BluetoothLE::begin(){
    #if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
        #error Bluetooth is not enabled.
    #endif

    // Initialise device
    BLEDevice::init("ESP32 Wombat");

    // Construct server on device
    server = BLEDevice::createServer();
    // Set callbacks for server
    server->setCallbacks(new Node_BluetoothLE_Server_Callbacks);

    // Services and characteristics (callbacks incorporated)
    (Node_BLE_UART_Service(server));

    // Start server advertising
    server->getAdvertising()->start();

    device_connected = false;
    current_device = false;
}


[[noreturn]] void Node_BluetoothLE::read_write_blocking() const{
    while(true){

        if(device_connected){
            if(Serial.available()){

                Serial.readBytes(message_buffer, sizeof(message_buffer));

                size_t msg_len = strlen(message_buffer);
                if(msg_len < UINT8_MAX){
                    message_buffer[msg_len] = '\n';
                    message_buffer[msg_len + 1] = '\0';
                }

                server->getServiceByUUID(UART_SERVICE_UUID)->getCharacteristic(
                        UART_CHAR_TX_UUID)->setValue(message_buffer);
                server->getServiceByUUID(UART_SERVICE_UUID)->getCharacteristic(
                        UART_CHAR_TX_UUID)->notify();

                message_tone();
                Serial.print("[Host]: ");
                Serial.print(message_buffer);
                memset(message_buffer, 0, sizeof(message_buffer));
            }
        }

        if(!device_connected && current_device){
            server->startAdvertising();
            current_device = device_connected;
        }

        if(device_connected && !current_device){
            current_device = device_connected;
        }
    }
}
