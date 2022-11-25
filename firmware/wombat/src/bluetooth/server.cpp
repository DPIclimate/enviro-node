#include "bluetooth/server.h"

static BLEServer* server;
static char message_buffer[UINT8_MAX];

void BluetoothServer::begin(){
    #if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
        #error Bluetooth is not enabled.
    #endif

    // Initialise device
    BLEDevice::init("ESP32 Wombat");

    // Construct server on device
    server = BLEDevice::createServer();
    // Set callbacks for server
    server->setCallbacks(new BluetoothServerCallbacks);

    // Services and characteristics (callbacks incorporated)
    (BluetoothUartService(server));

    // Start server advertising
    server->getAdvertising()->start();

    device_connected = false;
    current_device = false;
}

bool BluetoothServer::is_device_connected(){
    return device_connected;
}

void BluetoothServer::notify_device(const char* message){
    server->getServiceByUUID(UART_SERVICE_UUID)->getCharacteristic(
            UART_CHAR_TX_UUID)->setValue(message);
    server->getServiceByUUID(UART_SERVICE_UUID)->getCharacteristic(
            UART_CHAR_TX_UUID)->notify();
}

void BluetoothServer::read_write_blocking(){
    while(true){
        if(device_connected){
            if(Serial.available()){

                Serial.readBytes(message_buffer, sizeof(message_buffer));

                if(strcmp(message_buffer, "exit") == 0) {
                    device_connected = false;
                    memset(message_buffer, 0, sizeof(message_buffer));
                    return;
                }

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
